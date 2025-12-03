#ifndef SYSTEMMONITOR_H
#define SYSTEMMONITOR_H

#include <QObject>
#include <QTimer>

class SystemMonitor : public QObject {
  Q_OBJECT
  Q_PROPERTY(double cpuUsage READ cpuUsage NOTIFY cpuUsageChanged)
  Q_PROPERTY(double memoryUsageMB READ memoryUsageMB NOTIFY memoryUsageChanged)
  Q_PROPERTY(double gpuUsage READ gpuUsage NOTIFY gpuUsageChanged)
  Q_PROPERTY(
      double gpuVramUsedMB READ gpuVramUsedMB NOTIFY gpuVramUsedMBChanged)
  Q_PROPERTY(
      double gpuVramTotalMB READ gpuVramTotalMB NOTIFY gpuVramTotalMBChanged)
  Q_PROPERTY(QString gpuName READ gpuName CONSTANT)

public:
  explicit SystemMonitor(QObject *parent = nullptr);

  // Property getters
  double cpuUsage() const { return m_cpuUsage; }
  double memoryUsageMB() const { return m_memoryUsageMB; }
  double gpuUsage() const { return m_gpuUsage; }
  double gpuVramUsedMB() const { return m_gpuVramUsedMB; }
  double gpuVramTotalMB() const { return m_gpuVramTotalMB; }
  QString gpuName() const { return m_gpuName; }

  // Invokable methods for one-time queries
  Q_INVOKABLE double getCpuUsage();
  Q_INVOKABLE double getMemoryUsageMB();
  Q_INVOKABLE double getGpuUsage();
  Q_INVOKABLE QString getGpuName();

  // Control monitoring
  Q_INVOKABLE void startMonitoring(int intervalMs = 1000);
  Q_INVOKABLE void stopMonitoring();

signals:
  void cpuUsageChanged();
  void memoryUsageChanged();
  void gpuUsageChanged();
  void gpuVramUsedMBChanged();
  void gpuVramTotalMBChanged();

private slots:
  void updateStats();

private:
  // Current values
  double m_cpuUsage;
  double m_memoryUsageMB;
  double m_gpuUsage;
  double m_gpuVramUsedMB;
  double m_gpuVramTotalMB;
  QString m_gpuName;

  // Timer for automatic updates
  QTimer *m_updateTimer;

#ifdef Q_OS_WIN
  // Windows-specific: CPU tracking
  unsigned long long m_lastSystemTime;
  unsigned long long m_lastProcessTime;
#endif
};

#endif // SYSTEMMONITOR_H
