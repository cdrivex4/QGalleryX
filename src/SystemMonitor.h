#ifndef SYSTEMMONITOR_H
#define SYSTEMMONITOR_H

#include <QObject>
#include <QTimer>

class SystemMonitor : public QObject {
  Q_OBJECT
  Q_PROPERTY(double cpuUsage READ cpuUsage NOTIFY cpuUsageChanged)
  Q_PROPERTY(double systemCpuUsage READ systemCpuUsage NOTIFY systemCpuUsageChanged)
  Q_PROPERTY(double memoryUsageMB READ memoryUsageMB NOTIFY memoryUsageChanged)
  Q_PROPERTY(double totalSystemMemoryMB READ totalSystemMemoryMB NOTIFY systemMemoryChanged)
  Q_PROPERTY(double availableSystemMemoryMB READ availableSystemMemoryMB NOTIFY systemMemoryChanged)
  Q_PROPERTY(double gpuUsage READ gpuUsage NOTIFY gpuUsageChanged)
  Q_PROPERTY(
      double gpuVramUsedMB READ gpuVramUsedMB NOTIFY gpuVramUsedMBChanged)
  Q_PROPERTY(
      double gpuVramTotalMB READ gpuVramTotalMB NOTIFY gpuVramTotalMBChanged)
  Q_PROPERTY(QString gpuName READ gpuName CONSTANT)

public:
  explicit SystemMonitor(QObject *parent = nullptr);
  static SystemMonitor* instance();

  // Property getters
  double cpuUsage() const { return m_cpuUsage; }
  double systemCpuUsage() const { return m_systemCpuUsage; }
  double memoryUsageMB() const { return m_memoryUsageMB; }
  double totalSystemMemoryMB() const { return m_totalSystemMemoryMB; }
  double availableSystemMemoryMB() const { return m_availableSystemMemoryMB; }
  double gpuUsage() const { return m_gpuUsage; }
  double gpuVramUsedMB() const { return m_gpuVramUsedMB; }
  double gpuVramTotalMB() const { return m_gpuVramTotalMB; }
  QString gpuName() const { return m_gpuName; }

  // Invokable methods for one-time queries
  double getCpuUsage();
  double getSystemCpuUsage();
  double getMemoryUsageMB();
  Q_INVOKABLE double getGpuUsage();
  Q_INVOKABLE QString getGpuName();
  Q_INVOKABLE void logEnvironmentSnapshot();

  // Control monitoring
  Q_INVOKABLE void startMonitoring(int intervalMs = 1000);
  Q_INVOKABLE void stopMonitoring();

signals:
  void cpuUsageChanged();
  void systemCpuUsageChanged();
  void memoryUsageChanged();
  void systemMemoryChanged();
  void gpuUsageChanged();
  void gpuVramUsedMBChanged();
  void gpuVramTotalMBChanged();

private slots:
  void updateStats();

private:
  // Current values
  double m_cpuUsage;
  double m_systemCpuUsage;
  double m_memoryUsageMB;
  double m_totalSystemMemoryMB = 0.0;
  double m_availableSystemMemoryMB = 0.0;
  double m_gpuUsage;
  double m_gpuVramUsedMB;
  double m_gpuVramTotalMB;
  QString m_gpuName;

  // Timer for automatic updates
  QTimer *m_updateTimer;

  // Logging Thresholds
  double m_lastLogMemoryMB = 0.0;
  double m_lastLogVramMB = 0.0;

#ifdef Q_OS_WIN
  // Windows-specific: CPU tracking
  unsigned long long m_lastSystemTime;
  unsigned long long m_lastProcessTime;
  
  // System-wide CPU tracking
  unsigned long long m_lastIdleTime = 0;
  unsigned long long m_lastKernelTime = 0;
  unsigned long long m_lastUserTime = 0;

  // PDH for GPU Load
  void *m_pdhQuery;   // HQUERY
  void *m_pdhCounter; // HCOUNTER
#endif
};

#endif // SYSTEMMONITOR_H
