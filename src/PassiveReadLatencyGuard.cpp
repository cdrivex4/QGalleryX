#include "PassiveReadLatencyGuard.h"
#include "DesktopHelper.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>
#include <QDebug>
#include <cmath>
#include <thread>

PassiveReadLatencyGuard& PassiveReadLatencyGuard::instance() {
    static PassiveReadLatencyGuard inst;
    return inst;
}

PassiveReadLatencyGuard::PassiveReadLatencyGuard(QObject *parent) : QObject(parent) {}

PassiveReadLatencyGuard::ReadScope PassiveReadLatencyGuard::startRead(const QString &filePath, qint64 fileSize) {
    ReadScope scope;
    if (filePath.isEmpty() || filePath.startsWith("synthetic:")) {
        return scope;
    }

    // Skip network drives
    if (DesktopHelper::staticIsNetworkPath(filePath)) {
        return scope;
    }

    scope.filePath = filePath;
    scope.fileSize = fileSize;
    
    QString clean = QDir::fromNativeSeparators(filePath);
    if (clean.length() >= 2 && clean[1] == ':') {
        scope.driveRoot = clean.left(3).toUpper();
    } else {
        scope.driveRoot = "LOCAL";
    }

    scope.active = true;
    scope.timer.start();
    return scope;
}

void PassiveReadLatencyGuard::endRead(ReadScope &scope) {
    if (!scope.active) return;
    scope.active = false;

    qint64 elapsedMs = scope.timer.elapsed();

    QMutexLocker lock(&m_mutex);
    DriveLatencyStats &stats = m_driveStats[scope.driveRoot];
    stats.sampleCount++;

    // Welford's algorithm for running mean and variance
    double delta = elapsedMs - stats.avgMs;
    stats.avgMs += delta / stats.sampleCount;
    double delta2 = elapsedMs - stats.avgMs;
    stats.m2 += delta * delta2;

    double stdDev = 0.0;
    if (stats.sampleCount > 1) {
        stdDev = std::sqrt(stats.m2 / (stats.sampleCount - 1));
    }

    // Trigger anomaly only on genuine drive I/O stalls (>= 1000ms or 3x stddev above baseline)
    bool isAnomaly = false;
    if (stats.sampleCount >= 10 && elapsedMs > 500 && elapsedMs > (stats.avgMs + 3 * stdDev)) {
        isAnomaly = true;
    } else if (elapsedMs >= 1000) {
        isAnomaly = true;
    }

    if (isAnomaly) {
        stats.anomalyCount++;
        qDebug() << "[LatencyGuard] SPIKE DETECTED on" << scope.filePath << "Latency:" << elapsedMs << "ms (Avg:" << stats.avgMs << "ms)";
        logAnomaly(scope.filePath, scope.driveRoot, scope.fileSize, elapsedMs, stats.avgMs);

        // Soft back-off: Concurrency floor is 2, brief 20ms breather
        int hwMax = std::max(2, (int)std::thread::hardware_concurrency());
        int current = m_recommendedConcurrency.load(std::memory_order_relaxed);
        int reduced = std::max(2, current / 2);
        m_recommendedConcurrency.store(reduced, std::memory_order_relaxed);
        m_throttleDelayMs.store(20, std::memory_order_relaxed);
        m_consecutiveFastReads.store(0, std::memory_order_relaxed);

        QString fileName = QFileInfo(scope.filePath).fileName();
        emit singleLatencySpike(fileName, elapsedMs);
        emit concurrencyChanged(reduced);

        if (stats.anomalyCount == 10 || stats.anomalyCount == 30 || stats.anomalyCount == 60) {
            qDebug() << "[LatencyGuard] DRIVE WARNING on" << scope.driveRoot << "Count:" << stats.anomalyCount;
            emit driveLatencyWarning(scope.driveRoot, stats.anomalyCount);
        }
    } else {
        // Clean Fast Read: Additive Increase up to hardware concurrency
        int hwMax = std::max(4, (int)std::thread::hardware_concurrency());
        int fast = m_consecutiveFastReads.fetch_add(1, std::memory_order_relaxed) + 1;
        if (fast >= 3) { // Ramp up quickly every 3 clean reads
            m_consecutiveFastReads.store(0, std::memory_order_relaxed);
            int current = m_recommendedConcurrency.load(std::memory_order_relaxed);
            if (current < hwMax) {
                m_recommendedConcurrency.store(current + 1, std::memory_order_relaxed);
                emit concurrencyChanged(current + 1);
            }
            m_throttleDelayMs.store(0, std::memory_order_relaxed);
        }
    }
}

int PassiveReadLatencyGuard::recommendedConcurrency() const {
    return m_recommendedConcurrency.load(std::memory_order_relaxed);
}

int PassiveReadLatencyGuard::throttleDelayMs() const {
    return m_throttleDelayMs.load(std::memory_order_relaxed);
}

bool PassiveReadLatencyGuard::isCongested() const {
    return m_recommendedConcurrency.load(std::memory_order_relaxed) <= 1;
}

void PassiveReadLatencyGuard::logAnomaly(const QString &filePath, const QString &driveRoot, qint64 fileSize, qint64 latencyMs, double avgMs) {
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/antigravity";
    QDir().mkpath(logDir);
    QString logFile = logDir + "/disk_latency_audit.log";

    QFile file(logFile);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
        double sizeMB = fileSize / (1024.0 * 1024.0);
        out << "[" << timeStr << "] [Drive " << driveRoot << "] [Latency: " << latencyMs << "ms] [Avg: " << qRound(avgMs) << "ms] [Size: " << QString::number(sizeMB, 'f', 2) << "MB] Path: \"" << filePath << "\"\n";
    }
}
