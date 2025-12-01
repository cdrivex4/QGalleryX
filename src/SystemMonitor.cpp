#include "SystemMonitor.h"
#include <QDebug>
#include <QString>

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

SystemMonitor::SystemMonitor(QObject *parent)
    : QObject(parent), m_cpuUsage(0.0), m_memoryUsageMB(0.0), m_gpuUsage(0.0),
      m_updateTimer(new QTimer(this))
#ifdef Q_OS_WIN
      ,
      m_lastSystemTime(0), m_lastProcessTime(0)
#endif
{
  connect(m_updateTimer, &QTimer::timeout, this, &SystemMonitor::updateStats);
}

void SystemMonitor::startMonitoring(int intervalMs) {
  if (!m_updateTimer->isActive()) {
    qDebug() << "SystemMonitor: Starting monitoring with interval" << intervalMs
             << "ms";
    m_updateTimer->start(intervalMs);
    updateStats(); // Get initial values immediately
  }
}

void SystemMonitor::stopMonitoring() {
  if (!m_updateTimer->isActive()) {
    qDebug() << "SystemMonitor: Stopping monitoring";
    m_updateTimer->stop();
  }
}

void SystemMonitor::updateStats() {
  double oldCpu = m_cpuUsage;
  double oldMem = m_memoryUsageMB;
  double oldGpu = m_gpuUsage;

  m_cpuUsage = getCpuUsage();
  m_memoryUsageMB = getMemoryUsageMB();
  m_gpuUsage = getGpuUsage();

  // Emit signals only if values changed
  if (m_cpuUsage != oldCpu)
    emit cpuUsageChanged();
  if (m_memoryUsageMB != oldMem)
    emit memoryUsageChanged();
  if (m_gpuUsage != oldGpu)
    emit gpuUsageChanged();
}

// Helper to convert FILETIME to unsigned long long
#ifdef Q_OS_WIN
static unsigned long long fileTimeToInt64(const FILETIME &ft) {
  return (((unsigned long long)(ft.dwHighDateTime)) << 32) |
         ((unsigned long long)ft.dwLowDateTime);
}
#endif

double SystemMonitor::getCpuUsage() {
#ifdef Q_OS_WIN
  FILETIME sysIdle, sysKernel, sysUser;
  FILETIME procCreation, procExit, procKernel, procUser;

  if (!GetSystemTimes(&sysIdle, &sysKernel, &sysUser) ||
      !GetProcessTimes(GetCurrentProcess(), &procCreation, &procExit,
                       &procKernel, &procUser)) {
    return 0.0;
  }

  unsigned long long sysTime =
      fileTimeToInt64(sysKernel) + fileTimeToInt64(sysUser);
  unsigned long long procTime =
      fileTimeToInt64(procKernel) + fileTimeToInt64(procUser);

  // First call - just store values
  if (m_lastSystemTime == 0) {
    m_lastSystemTime = sysTime;
    m_lastProcessTime = procTime;
    return 0.0;
  }

  unsigned long long sysDelta = sysTime - m_lastSystemTime;
  unsigned long long procDelta = procTime - m_lastProcessTime;

  m_lastSystemTime = sysTime;
  m_lastProcessTime = procTime;

  if (sysDelta == 0)
    return 0.0;

  // Return CPU usage as percentage (0-100)
  return (double)procDelta / (double)sysDelta * 100.0;
#else
  return 0.0; // Not implemented for other platforms
#endif
}

double SystemMonitor::getMemoryUsageMB() {
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
      return (double)pmc.WorkingSetSize / (1024.0 * 1024.0);
    }
  }
#endif
  return 0.0;
}

double SystemMonitor::getGpuUsage() {
  // TODO: Implement GPU usage monitoring
  // This is complex and requires vendor-specific APIs:
  // - NVIDIA: NVML (NVIDIA Management Library)
  // - AMD: ADL (AMD Display Library)
  // - Intel: Intel Media SDK
  // For now, return placeholder
  return 0.0;
}

QString SystemMonitor::getGpuName() {
#ifdef Q_OS_WIN
  // TODO: Query GPU name from Windows API or vendor libraries
  // For now, return a placeholder
  return "GPU Detection Not Implemented";
#else
  return "Platform Not Supported";
#endif
}
