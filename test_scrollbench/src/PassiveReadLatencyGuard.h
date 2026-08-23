#ifndef PASSIVEREADLATENCYGUARD_H
#define PASSIVEREADLATENCYGUARD_H

#include <QElapsedTimer>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QMap>
#include <QDateTime>

struct DriveLatencyStats {
    int sampleCount = 0;
    double totalMs = 0.0;
    double avgMs = 0.0;
    double m2 = 0.0;
    int anomalyCount = 0;
};

class PassiveReadLatencyGuard : public QObject {
    Q_OBJECT
public:
    static PassiveReadLatencyGuard& instance();

    struct ReadScope {
        QString filePath;
        QString driveRoot;
        qint64 fileSize = 0;
        QElapsedTimer timer;
        bool active = false;
    };

    ReadScope startRead(const QString &filePath, qint64 fileSize);
    void endRead(ReadScope &scope);

    Q_INVOKABLE int recommendedConcurrency() const;
    Q_INVOKABLE int throttleDelayMs() const;
    Q_INVOKABLE bool isCongested() const;

signals:
    void singleLatencySpike(const QString &fileName, qint64 latencyMs);
    void driveLatencyWarning(const QString &driveRoot, int spikeCount);
    void concurrencyChanged(int newConcurrency);

private:
    explicit PassiveReadLatencyGuard(QObject *parent = nullptr);
    void logAnomaly(const QString &filePath, const QString &driveRoot, qint64 fileSize, qint64 latencyMs, double avgMs);

    QMutex m_mutex;
    QMap<QString, DriveLatencyStats> m_driveStats;
    std::atomic<int> m_recommendedConcurrency{4};
    std::atomic<int> m_throttleDelayMs{0};
    std::atomic<int> m_consecutiveFastReads{0};
};

#endif // PASSIVEREADLATENCYGUARD_H
