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

signals:
    void singleLatencySpike(const QString &fileName, qint64 latencyMs);
    void driveLatencyWarning(const QString &driveRoot, int spikeCount);

private:
    explicit PassiveReadLatencyGuard(QObject *parent = nullptr);
    void logAnomaly(const QString &filePath, const QString &driveRoot, qint64 fileSize, qint64 latencyMs, double avgMs);

    QMutex m_mutex;
    QMap<QString, DriveLatencyStats> m_driveStats;
};

#endif // PASSIVEREADLATENCYGUARD_H
