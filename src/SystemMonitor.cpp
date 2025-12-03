#include "SystemMonitor.h"
#include <QDebug>
#include <QString>

#ifdef Q_OS_WIN
#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <windows.h>
#include <wrl/client.h>

using namespace Microsoft::WRL;

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
      m_gpuVramUsedMB(0.0), m_gpuVramTotalMB(0.0), m_gpuName("Unknown"),
      m_updateTimer(new QTimer(this))
#ifdef Q_OS_WIN
      ,
      m_lastSystemTime(0), m_lastProcessTime(0)
#endif
{
  connect(m_updateTimer, &QTimer::timeout, this, &SystemMonitor::updateStats);

  // Initial GPU detection
  m_gpuName = getGpuName();
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
  double oldVramUsed = m_gpuVramUsedMB;
  double oldVramTotal = m_gpuVramTotalMB;

  m_cpuUsage = getCpuUsage();
  m_memoryUsageMB = getMemoryUsageMB();
  m_gpuUsage =
      getGpuUsage(); // This updates m_gpuVramUsedMB and m_gpuVramTotalMB

  // Emit signals only if values changed
  if (m_cpuUsage != oldCpu)
    emit cpuUsageChanged();
  if (m_memoryUsageMB != oldMem)
    emit memoryUsageChanged();
  if (m_gpuUsage != oldGpu)
    emit gpuUsageChanged();
  if (m_gpuVramUsedMB != oldVramUsed)
    emit gpuVramUsedMBChanged();
  if (m_gpuVramTotalMB != oldVramTotal)
    emit gpuVramTotalMBChanged();
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
#ifdef Q_OS_WIN
  // Use DXGI 1.4 to get video memory budget and usage
  ComPtr<IDXGIFactory4> factory;
  if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
    return 0.0;
  }

  ComPtr<IDXGIAdapter3> adapter;
  if (FAILED(factory->EnumAdapters(
          0, reinterpret_cast<IDXGIAdapter **>(adapter.GetAddressOf())))) {
    // Fallback to older DXGI if 1.4 isn't available (e.g. Windows 7)
    // We can't easily get "usage" on Win7 without D3D counters, so just return
    // 0
    return 0.0;
  }

  DXGI_QUERY_VIDEO_MEMORY_INFO videoMemoryInfo;
  if (SUCCEEDED(adapter->QueryVideoMemoryInfo(
          0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &videoMemoryInfo))) {
    m_gpuVramUsedMB = videoMemoryInfo.CurrentUsage / (1024.0 * 1024.0);
    m_gpuVramTotalMB = videoMemoryInfo.Budget / (1024.0 * 1024.0);

    // Return percentage of budget used
    if (videoMemoryInfo.Budget > 0) {
      return (double)videoMemoryInfo.CurrentUsage /
             (double)videoMemoryInfo.Budget * 100.0;
    }
  }
#endif
  return 0.0;
}

QString SystemMonitor::getGpuName() {
#ifdef Q_OS_WIN
  ComPtr<IDXGIFactory1> factory;
  if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
    return "DXGI Error";
  }

  ComPtr<IDXGIAdapter1> adapter;
  if (SUCCEEDED(factory->EnumAdapters1(0, &adapter))) {
    DXGI_ADAPTER_DESC1 desc;
    if (SUCCEEDED(adapter->GetDesc1(&desc))) {
      // Store total VRAM from description as fallback/baseline
      m_gpuVramTotalMB = desc.DedicatedVideoMemory / (1024.0 * 1024.0);
      return QString::fromWCharArray(desc.Description);
    }
  }
  return "Unknown GPU";
#else
  return "Platform Not Supported";
#endif
}
