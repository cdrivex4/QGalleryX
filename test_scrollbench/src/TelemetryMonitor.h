#ifndef TELEMETRYMONITOR_H
#define TELEMETRYMONITOR_H

#include <QElapsedTimer>
#include <QObject>
#include <QVariant>
#include <QVector>

class TelemetryMonitor : public QObject {
  Q_OBJECT
  Q_PROPERTY(int fps READ fps NOTIFY fpsChanged)
  Q_PROPERTY(int averageFps READ averageFps NOTIFY averageFpsChanged)
  Q_PROPERTY(double cpuUsage READ cpuUsage NOTIFY historyChanged)
  Q_PROPERTY(int pendingDecodes READ pendingDecodes WRITE setPendingDecodes
                 NOTIFY pendingDecodesChanged)
  Q_PROPERTY(qreal cacheHitRate READ cacheHitRate NOTIFY cacheHitRateChanged)
  Q_PROPERTY(int delegateCount READ delegateCount WRITE setDelegateCount NOTIFY
                 delegateCountChanged)
  Q_PROPERTY(
      qint64 memoryUsageMB READ memoryUsageMB NOTIFY memoryUsageMBChanged)
  Q_PROPERTY(int completionsThisFrame READ completionsThisFrame WRITE
                 setCompletionsThisFrame NOTIFY completionsThisFrameChanged)
  Q_PROPERTY(int lastLoadTime READ lastLoadTime NOTIFY lastLoadTimeChanged)
  Q_PROPERTY(int lastWorkDuration READ lastWorkDuration NOTIFY lastWorkDurationChanged)
  Q_PROPERTY(int minLoadTime READ minLoadTime NOTIFY lastLoadTimeChanged)
  Q_PROPERTY(int maxLoadTime READ maxLoadTime NOTIFY lastLoadTimeChanged)
  Q_PROPERTY(int averageLoadTime READ averageLoadTime NOTIFY lastLoadTimeChanged)
  
  Q_PROPERTY(QVariantList cpuHistory READ cpuHistory NOTIFY historyChanged)
  Q_PROPERTY(QVariantList ramHistory READ ramHistory NOTIFY historyChanged)
  Q_PROPERTY(QVariantList gpuHistory READ gpuHistory NOTIFY historyChanged)

public:
  explicit TelemetryMonitor(QObject *parent = nullptr);

  int fps() const { return m_currentFps; }
  int averageFps() const;
  double cpuUsage() const { return m_cpuHistory.isEmpty() ? 0.0 : m_cpuHistory.last().toDouble(); }
  int pendingDecodes() const { return m_pendingDecodes; }
  qreal cacheHitRate() const { return m_cacheHitRate; }
  int delegateCount() const { return m_delegateCount; }
  qint64 memoryUsageMB() const { return m_memoryUsageMB; }
  int completionsThisFrame() const { return m_completionsThisFrame; }
  int lastLoadTime() const { return m_lastLoadTime; }
  int lastWorkDuration() const { return m_lastWorkDuration; }
  int minLoadTime() const { return m_minLoadTime; }
  int maxLoadTime() const { return m_maxLoadTime; }
  int averageLoadTime() const { return m_loadCount > 0 ? static_cast<int>(m_totalLoadTime / m_loadCount) : 0; } // This line was modified in the original replace, but it should be kept as is.

  QVariantList cpuHistory() const { return m_cpuHistory; }
  QVariantList ramHistory() const { return m_ramHistory; }
  QVariantList gpuHistory() const { return m_gpuHistory; }

  void setPendingDecodes(int count);
  void setDelegateCount(int count);
  void setCompletionsThisFrame(int count);
  
  Q_INVOKABLE void reportLoadTime(int ms);
  Q_INVOKABLE void reportWorkDuration(int ms);
  Q_INVOKABLE void resetStats();

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
  void completionsThisFrameChanged();
  void lastLoadTimeChanged();
  void lastWorkDurationChanged();
  void historyChanged();

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
  int m_completionsThisFrame = 0;
  
  int m_lastLoadTime = 0;
  int m_lastWorkDuration = 0;
  int m_minLoadTime = 999999;
  int m_maxLoadTime = 0;
  qint64 m_totalLoadTime = 0;
  int m_loadCount = 0;

  QVariantList m_cpuHistory;
  QVariantList m_ramHistory;
  QVariantList m_gpuHistory;
};

#endif // TELEMETRYMONITOR_H
