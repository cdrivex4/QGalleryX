#include "TelemetryMonitor.h"
#include <QDateTime>
#include <QDebug>
#include <QtGlobal>

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

TelemetryMonitor::TelemetryMonitor(QObject *parent) : QObject(parent) {
  m_frameTimer.start();
  m_frameTimes.reserve(60);
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

void TelemetryMonitor::recordFrame() {
  qint64 frameTime = m_frameTimer.restart();

  // Store last 60 frame times
  if (m_frameTimes.size() >= 60) {
    m_frameTimes.removeFirst();
  }
  m_frameTimes.append(frameTime);

  m_frameCount++;

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
#ifdef Q_OS_WIN
  // Use dynamic loading to avoid MinGW psapi.h issues
  typedef BOOL(WINAPI * PGetProcessMemoryInfo)(HANDLE, PPROCESS_MEMORY_COUNTERS,
                                               DWORD);
  static PGetProcessMemoryInfo pGetProcessMemoryInfo = nullptr;

  if (!pGetProcessMemoryInfo) {
    HMODULE hPsapi = LoadLibraryA("psapi.dll");
    if (hPsapi) {
      pGetProcessMemoryInfo =
          (PGetProcessMemoryInfo)GetProcAddress(hPsapi, "GetProcessMemoryInfo");
    }
  }

  if (pGetProcessMemoryInfo) {
    PROCESS_MEMORY_COUNTERS pmc;
    pmc.cb = sizeof(pmc);
    if (pGetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
      qint64 memoryMB = pmc.WorkingSetSize / (1024 * 1024);
      if (m_memoryUsageMB != memoryMB) {
        m_memoryUsageMB = memoryMB;
        emit memoryUsageMBChanged();
      }
    }
  }
#else
  // Placeholder for other platforms
  m_memoryUsageMB = 0;
#endif
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
