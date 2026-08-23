#include "TelemetryMonitor.h"
#include "SystemMonitor.h"
#include "TaskScheduler.h"
#include <QDateTime>
#include <QDebug>
#include <QTimer>
#include <QtGlobal>
#include "../../src/AsyncImageProvider.h"

#ifdef Q_OS_WIN
#include <windows.h>

// Manually define PROCESS_MEMORY_COUNTERS to avoid MinGW psapi.h issues
typedef struct _PROCESS_MEMORY_COUNTERS {
  DWORD cb;
  DWORD PageFaultCount;
  SIZE_T PeakWorkingSetSize;
  SIZE_T WorkingSetSize;
  SIZE_T QuotaPeakPagedPoolUsage;
  SIZE_T QuotaPagedPoolUsage;
  SIZE_T QuotaPeakNonPagedPoolUsage;
  SIZE_T QuotaNonPagedPoolUsage;
  SIZE_T PagefileUsage;
  SIZE_T PeakPagefileUsage;
} PROCESS_MEMORY_COUNTERS;
typedef PROCESS_MEMORY_COUNTERS *PPROCESS_MEMORY_COUNTERS;
#endif

#include <QDir>
#include <QFile>
#include <QTextStream>

TelemetryMonitor::TelemetryMonitor(QObject *parent) : QObject(parent) {
  m_frameTimer.start();
  m_frameTimes.reserve(60);

  // Initialize histories with zeros so graphs aren't empty
  for (int i = 0; i < 50; ++i) {
    m_cpuHistory.append(0.0);
    m_gpuHistory.append(0.0);
    m_ramHistory.append(0.0);
  }

  QTimer *timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, [this]() {
    SystemMonitor sys;
    double cpu = sys.getCpuUsage();
    m_cpuHistory.append(cpu);
    if (m_cpuHistory.size() > 50)
      m_cpuHistory.removeFirst();

    double gpu = sys.getGpuUsage();
    m_gpuHistory.append(gpu);
    if (m_gpuHistory.size() > 50)
      m_gpuHistory.removeFirst();

    updateMemoryUsage();
    emit historyChanged();
  });
  timer->start(1000);
}

TelemetryMonitor::~TelemetryMonitor() { stopBenchmarking(); }

void TelemetryMonitor::setCompletionsThisFrame(int count) {
  if (m_completionsThisFrame != count) {
    m_completionsThisFrame = count;
    emit completionsThisFrameChanged();
  }
}

int TelemetryMonitor::averageFps() const {
  if (m_frameTimes.isEmpty()) {
    return 0;
  }

  qint64 totalTime = 0;
  for (qint64 time : m_frameTimes) {
    totalTime += time;
  }

  qint64 avgFrameTime = totalTime / m_frameTimes.size();
  return avgFrameTime > 0 ? static_cast<int>(1000.0 / avgFrameTime) : 0;
}

void TelemetryMonitor::setPendingDecodes(int count) {
  if (m_pendingDecodes != count) {
    m_pendingDecodes = count;
    emit pendingDecodesChanged();
  }
}

void TelemetryMonitor::setDelegateCount(int count) {
  if (m_delegateCount != count) {
    m_delegateCount = count;
    emit delegateCountChanged();
  }
}

static QList<int> s_loadHistory;

void TelemetryMonitor::reportLoadTime(int ms) {
  m_lastLoadTime = ms;

  // Moving average (last 20 loads)
  static const int MAX_LOAD_HISTORY = 20;
  s_loadHistory.append(ms);
  if (s_loadHistory.size() > MAX_LOAD_HISTORY)
    s_loadHistory.removeFirst();

  qint64 sum = 0;
  for (int val : s_loadHistory)
    sum += val;
  m_totalLoadTime = sum; // Overloading member for moving sum
  m_loadCount = s_loadHistory.size();

  if (ms < m_minLoadTime)
    m_minLoadTime = ms;
  if (ms > m_maxLoadTime)
    m_maxLoadTime = ms;
  emit lastLoadTimeChanged();
}

void TelemetryMonitor::resetStats() {
  s_loadHistory.clear();
  m_lastLoadTime = 0;
  m_minLoadTime = 999999;
  m_maxLoadTime = 0;
  m_totalLoadTime = 0;
  m_loadCount = 0;
  emit lastLoadTimeChanged();
}

void TelemetryMonitor::reportWorkDuration(int ms) {
  if (m_lastWorkDuration != ms) {
    m_lastWorkDuration = ms;
    emit lastWorkDurationChanged();
  }

  if (ms > 100) {
    qWarning() << "[Telemetry] High Work Duration detected:" << ms << "ms";
  }
}

void TelemetryMonitor::recordFrame() {
  qint64 frameTime = m_frameTimer.restart();

  // Store last 60 frame times
  if (m_frameTimes.size() >= 60) {
    m_frameTimes.removeFirst();
  }
  m_frameTimes.append(frameTime);

  m_frameCount++;
  if (m_frameCount % 300 == 0) {
    qDebug() << "TelemetryMonitor: Processed 300 frames. Current FPS:"
             << m_currentFps;
  }

  // Update FPS every 500ms
  qint64 now = QDateTime::currentMSecsSinceEpoch();
  if (now - m_lastFpsUpdate >= 500) {
    updateFps();
    m_lastFpsUpdate = now;
  }
}

void TelemetryMonitor::recordCacheHit() {
  m_cacheHits++;
  updateCacheHitRate();
}

void TelemetryMonitor::recordCacheMiss() {
  m_cacheMisses++;
  updateCacheHitRate();
}

void TelemetryMonitor::updateMemoryUsage() {
  double processMB = SystemMonitor::getMemoryUsageMB();
  if (m_memoryUsageMB != static_cast<qint64>(processMB)) {
    m_memoryUsageMB = static_cast<qint64>(processMB);
    emit memoryUsageMBChanged();
  }

  m_ramHistory.append(processMB);
  if (m_ramHistory.size() > 50)
    m_ramHistory.removeFirst();

  emit historyChanged();
}

void TelemetryMonitor::updateFps() {
  if (!m_frameTimes.isEmpty()) {
    int newFps = averageFps();
    if (m_currentFps != newFps) {
      m_currentFps = newFps;
      emit fpsChanged();
      emit averageFpsChanged();
    }
  }
}

void TelemetryMonitor::startBenchmarking(const QString &sessionName) {
  if (m_isBenchmarking)
    return;

  QDir().mkpath("logs");
  m_benchmarkFile =
      QString("logs/perf_%1_%2.csv")
          .arg(sessionName)
          .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));

  QFile file(m_benchmarkFile);
  if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream out(&file);
    out << "Timestamp,FPS,AvgFPS,CPU,RAM_MB,GPU,VRAM_MB,CacheHitRate,"
           "PendingDecodes,LastLoadMs,WorkDurationMs,TotalTasks\n";
    file.close();
  }

  m_isBenchmarking = true;
  emit isBenchmarkingChanged();

  if (!m_logTimer) {
    m_logTimer = new QTimer(this);
    connect(m_logTimer, &QTimer::timeout, this, &TelemetryMonitor::logStats);
  }
  m_logTimer->start(1000); // Log every second

  qDebug() << "Benchmarking started:" << m_benchmarkFile;
}

void TelemetryMonitor::stopBenchmarking() {
  if (!m_isBenchmarking)
    return;

  if (m_logTimer)
    m_logTimer->stop();
  m_isBenchmarking = false;
  emit isBenchmarkingChanged();
  qDebug() << "Benchmarking stopped.";
}

void TelemetryMonitor::logStats() {
  if (!m_isBenchmarking)
    return;

  QFile file(m_benchmarkFile);
  if (file.open(QIODevice::Append | QIODevice::Text)) {
    QTextStream out(&file);
    SystemMonitor sys;

    out << QDateTime::currentDateTime().toString("HH:mm:ss") << ","
        << m_currentFps << "," << averageFps() << ","
        << sys.cpuUsage() << ","
        << sys.memoryUsageMB() << ","
        << sys.gpuUsage() << ","
        << sys.gpuVramUsedMB() << "," << m_cacheHitRate << ","
        << m_pendingDecodes << "," << m_lastLoadTime << ","
        << m_lastWorkDuration << ","
        << (TaskScheduler::instance().getQueueSize(TaskScheduler::Immediate)) << "\n";

    file.close();
  }
}

void TelemetryMonitor::updateCacheHitRate() {
  int total = m_cacheHits + m_cacheMisses;
  if (total > 0) {
    qreal newRate = static_cast<qreal>(m_cacheHits) / total * 100.0;
    if (qAbs(m_cacheHitRate - newRate) > 0.1) {
      m_cacheHitRate = newRate;
      emit cacheHitRateChanged();
    }
  }
}
