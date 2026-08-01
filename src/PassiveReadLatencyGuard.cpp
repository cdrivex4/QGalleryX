#include "PassiveReadLatencyGuard.h"
#include "DesktopHelper.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>
#include <QDebug>
#include <cmath>

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

    // Trigger anomaly if:
    // 1. We have at least 5 baseline samples and latency > baseline + 2 * stdDev (and > 80ms)
    // OR single latency > 200ms
    bool isAnomaly = false;
    if (stats.sampleCount >= 5 && elapsedMs > 80 && elapsedMs > (stats.avgMs + 2 * stdDev)) {
        isAnomaly = true;
    } else if (elapsedMs >= 200) {
        isAnomaly = true;
    }

    if (isAnomaly) {
        stats.anomalyCount++;
        qDebug() << "[LatencyGuard] SPIKE DETECTED on" << scope.filePath << "Latency:" << elapsedMs << "ms (Avg:" << stats.avgMs << "ms)";
        logAnomaly(scope.filePath, scope.driveRoot, scope.fileSize, elapsedMs, stats.avgMs);

        QString fileName = QFileInfo(scope.filePath).fileName();
        emit singleLatencySpike(fileName, elapsedMs);

        if (stats.anomalyCount == 5 || stats.anomalyCount == 15 || stats.anomalyCount == 30) {
            qDebug() << "[LatencyGuard] DRIVE WARNING on" << scope.driveRoot << "Count:" << stats.anomalyCount;
            emit driveLatencyWarning(scope.driveRoot, stats.anomalyCount);
        }
    }
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
