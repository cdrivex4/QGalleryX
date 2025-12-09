#ifndef TELEMETRYMONITOR_H
#define TELEMETRYMONITOR_H

#include <QElapsedTimer>
#include <QObject>
#include <QVector>


class TelemetryMonitor : public QObject {
  Q_OBJECT
  Q_PROPERTY(int fps READ fps NOTIFY fpsChanged)
  Q_PROPERTY(int averageFps READ averageFps NOTIFY averageFpsChanged)
  Q_PROPERTY(int pendingDecodes READ pendingDecodes WRITE setPendingDecodes
                 NOTIFY pendingDecodesChanged)
  Q_PROPERTY(qreal cacheHitRate READ cacheHitRate NOTIFY cacheHitRateChanged)
  Q_PROPERTY(int delegateCount READ delegateCount WRITE setDelegateCount NOTIFY
                 delegateCountChanged)
  Q_PROPERTY(
      qint64 memoryUsageMB READ memoryUsageMB NOTIFY memoryUsageMBChanged)

public:
  explicit TelemetryMonitor(QObject *parent = nullptr);

  int fps() const { return m_currentFps; }
  int averageFps() const;
  int pendingDecodes() const { return m_pendingDecodes; }
  qreal cacheHitRate() const { return m_cacheHitRate; }
  int delegateCount() const { return m_delegateCount; }
  qint64 memoryUsageMB() const { return m_memoryUsageMB; }

  void setPendingDecodes(int count);
  void setDelegateCount(int count);

  Q_INVOKABLE void recordFrame();
  Q_INVOKABLE void recordCacheHit();
  Q_INVOKABLE void recordCacheMiss();
  Q_INVOKABLE void updateMemoryUsage();

signals:
  void fpsChanged();
  void averageFpsChanged();
  void pendingDecodesChanged();
  void cacheHitRateChanged();
  void delegateCountChanged();
  void memoryUsageMBChanged();

private:
  void updateFps();
  void updateCacheHitRate();

  QElapsedTimer m_frameTimer;
  QVector<qint64> m_frameTimes; // Last 60 frame durations
  int m_currentFps = 0;
  int m_frameCount = 0;
  qint64 m_lastFpsUpdate = 0;

  int m_pendingDecodes = 0;
  int m_delegateCount = 0;

  int m_cacheHits = 0;
  int m_cacheMisses = 0;
  qreal m_cacheHitRate = 0.0;

  qint64 m_memoryUsageMB = 0;
};

#endif // TELEMETRYMONITOR_H
