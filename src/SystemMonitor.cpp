#include "SystemMonitor.h"
#include <QDebug>
#include <QString>
#include <QDateTime>
#include <cmath>
#include <pdh.h>
#include <pdhmsg.h>
#include <QSysInfo>
#include <QThread>


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

static SystemMonitor* s_instance = nullptr;

SystemMonitor* SystemMonitor::instance() { return s_instance; }

SystemMonitor::SystemMonitor(QObject *parent)
    : QObject(parent), m_cpuUsage(0.0), m_memoryUsageMB(0.0), m_gpuUsage(0.0),
      m_gpuVramUsedMB(0.0), m_gpuVramTotalMB(0.0), m_gpuName("Unknown"),
      m_updateTimer(new QTimer(this))
#ifdef Q_OS_WIN
      ,
      m_lastSystemTime(0), m_lastProcessTime(0),
      m_lastIdleTime(0), m_lastKernelTime(0), m_lastUserTime(0)
#endif
{
  s_instance = this;
  connect(m_updateTimer, &QTimer::timeout, this, &SystemMonitor::updateStats);

  // Initial GPU detection
  m_gpuName = getGpuName();

  logEnvironmentSnapshot();

#ifdef Q_OS_WIN
  // PDH Init for GPU Load
  m_pdhQuery = nullptr;
  m_pdhCounter = nullptr;

  if (PdhOpenQueryA(NULL, 0, (HQUERY *)&m_pdhQuery) == ERROR_SUCCESS) {
    PdhAddEnglishCounterA((HQUERY)m_pdhQuery,
                          "\\GPU Engine(*)\\Utilization Percentage", 0,
                          (HCOUNTER *)&m_pdhCounter);
    // Mandatory double-collect for PDH to establish baseline
    PdhCollectQueryData((HQUERY)m_pdhQuery);
    Sleep(10); 
    PdhCollectQueryData((HQUERY)m_pdhQuery);
  }
#endif
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

void SystemMonitor::logEnvironmentSnapshot() {
    getMemoryUsageMB(); // Populates m_totalSystemMemoryMB

    QString os = QSysInfo::prettyProductName();
    QString cpuArch = QSysInfo::currentCpuArchitecture();
    int threads = QThread::idealThreadCount();

    qInfo() << "================ ENVIRONMENT SNAPSHOT ================";
    qInfo() << "OS              :" << os;
    qInfo() << "CPU Arch        :" << cpuArch << "(" << threads << "logical threads)";
    qInfo() << "Total System RAM:" << m_totalSystemMemoryMB << "MB";
    qInfo() << "GPU detected    :" << m_gpuName << "(" << m_gpuVramTotalMB << "MB VRAM)";
    qInfo() << "======================================================";
}

void SystemMonitor::updateStats() {
  double oldCpu = m_cpuUsage;
  double oldSysCpu = m_systemCpuUsage;
  double oldMem = m_memoryUsageMB;
  double oldTotalMem = m_totalSystemMemoryMB;
  double oldAvailMem = m_availableSystemMemoryMB;
  double oldGpu = m_gpuUsage;
  double oldVramUsed = m_gpuVramUsedMB;
  double oldVramTotal = m_gpuVramTotalMB;

  m_cpuUsage = getCpuUsage();
  m_systemCpuUsage = getSystemCpuUsage();
  m_memoryUsageMB = getMemoryUsageMB();
  m_gpuUsage =
      getGpuUsage(); // This updates m_gpuVramUsedMB and m_gpuVramTotalMB

  QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
  qDebug() << "[" << timeStr << "][SystemMonitor] Stats Update:"
           << "CPU App:" << m_cpuUsage << "% | Sys:" << m_systemCpuUsage << "%"
           << "| RAM App:" << m_memoryUsageMB << "MB"
           << "| Sys Avail:" << m_availableSystemMemoryMB << "MB /" << m_totalSystemMemoryMB << "MB";

  // Emit signals only if values changed
  if (m_cpuUsage != oldCpu)
    emit cpuUsageChanged();
  if (m_systemCpuUsage != oldSysCpu)
    emit systemCpuUsageChanged();
  if (m_memoryUsageMB != oldMem)
    emit memoryUsageChanged();
  
  if (m_totalSystemMemoryMB != oldTotalMem || m_availableSystemMemoryMB != oldAvailMem)
      emit systemMemoryChanged();

  if (m_gpuUsage != oldGpu)
    emit gpuUsageChanged();
  if (m_gpuVramUsedMB != oldVramUsed)
    emit gpuVramUsedMBChanged();
  if (m_gpuVramTotalMB != oldVramTotal)
    emit gpuVramTotalMBChanged();

  // Logging Analysis (Granularity Check)
  bool memChanged = std::abs(m_memoryUsageMB - m_lastLogMemoryMB) > 50.0;
  bool vramChanged = std::abs(m_gpuVramUsedMB - m_lastLogVramMB) > 50.0;

  if (memChanged || vramChanged) {
    if (m_memoryUsageMB > 1.0) { // Avoid initial zeros
      qDebug() << "[SystemMonitor] Resource Update:"
               << "RAM:" << m_memoryUsageMB << "MB"
               << "(Delta:" << (m_memoryUsageMB - m_lastLogMemoryMB) << ")"
               << "| VRAM:" << m_gpuVramUsedMB << "MB"
               << "(Delta:" << (m_gpuVramUsedMB - m_lastLogVramMB) << ")";
      m_lastLogMemoryMB = m_memoryUsageMB;
      m_lastLogVramMB = m_gpuVramUsedMB;
    }
  }
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

  double usage = (double)procDelta / (double)sysDelta * 100.0;
  
  // TRACE: App CPU logic
  if (usage > 0.1) {
      // qDebug() << "[SystemMonitor] App CPU Delta:" << procDelta << "/" << sysDelta << "=" << usage << "%";
  }
  
  return usage;
#else
  return 0.0;
#endif
}

double SystemMonitor::getSystemCpuUsage() {
#ifdef Q_OS_WIN
    FILETIME idleTime, kernelTime, userTime;
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        unsigned long long idle = fileTimeToInt64(idleTime);
        unsigned long long kernel = fileTimeToInt64(kernelTime);
        unsigned long long user = fileTimeToInt64(userTime);
        
        if (m_lastIdleTime == 0) {
            m_lastIdleTime = idle; m_lastKernelTime = kernel; m_lastUserTime = user;
            return 0.0;
        }
        
        unsigned long long idleDelta = idle - m_lastIdleTime;
        unsigned long long kernelDelta = kernel - m_lastKernelTime;
        unsigned long long userDelta = user - m_lastUserTime;
        
        m_lastIdleTime = idle; m_lastKernelTime = kernel; m_lastUserTime = user;
        
        // Kernel includes Idle
        unsigned long long totalDelta = kernelDelta + userDelta;
        if (totalDelta == 0) return 0.0;
        
        double usage = (double)(totalDelta - idleDelta) / (double)totalDelta * 100.0;
        return std::max(0.0, std::min(100.0, usage));
    }
#endif
    return 0.0;
}

double SystemMonitor::getMemoryUsageMB() {
#ifdef Q_OS_WIN
  // 1. Process Memory
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
      m_memoryUsageMB = (double)pmc.WorkingSetSize / (1024.0 * 1024.0);
    }
  }

  // 2. System Memory
  MEMORYSTATUSEX memInfo;
  memInfo.dwLength = sizeof(MEMORYSTATUSEX);
  if (GlobalMemoryStatusEx(&memInfo)) {
      m_totalSystemMemoryMB = (double)memInfo.ullTotalPhys / (1024.0 * 1024.0);
      m_availableSystemMemoryMB = (double)memInfo.ullAvailPhys / (1024.0 * 1024.0);
      
      if (m_totalSystemMemoryMB < 1.0) {
          qWarning() << "[SystemMonitor] GlobalMemoryStatusEx returned suspicious total memory:" << memInfo.ullTotalPhys;
      }
  } else {
      qWarning() << "[SystemMonitor] GlobalMemoryStatusEx failed. Error:" << GetLastError();
  }

  return m_memoryUsageMB;
#endif
  return 0.0;
}

double SystemMonitor::getGpuUsage() {
#ifdef Q_OS_WIN
  // 1. Update VRAM (DXGI)
  ComPtr<IDXGIFactory4> factory;
  if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
    ComPtr<IDXGIAdapter3> adapter;
    if (SUCCEEDED(factory->EnumAdapters(
            0, reinterpret_cast<IDXGIAdapter **>(adapter.GetAddressOf())))) {
      DXGI_QUERY_VIDEO_MEMORY_INFO videoMemoryInfo;
      if (SUCCEEDED(adapter->QueryVideoMemoryInfo(
              0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &videoMemoryInfo))) {
        m_gpuVramUsedMB = videoMemoryInfo.CurrentUsage / (1024.0 * 1024.0);
        m_gpuVramTotalMB = videoMemoryInfo.Budget / (1024.0 * 1024.0);
      }
    }
  }

  // 2. Update GPU Load (PDH)
  double totalUsage = 0.0;
  if (m_pdhQuery && m_pdhCounter) {
    PDH_STATUS status = PdhCollectQueryData((HQUERY)m_pdhQuery);
    if (status != ERROR_SUCCESS) {
        // PdhCollectQueryData failed
    }

    PDH_FMT_COUNTERVALUE_ITEM_A *pItems = NULL;
    DWORD dwBufferSize = 0;
    DWORD dwItemCount = 0;

    PdhGetFormattedCounterArrayA((HCOUNTER)m_pdhCounter, PDH_FMT_DOUBLE,
                                 &dwBufferSize, &dwItemCount, NULL);

    if (dwBufferSize > 0) {
      pItems = (PDH_FMT_COUNTERVALUE_ITEM_A *)malloc(dwBufferSize);
      if (pItems) {
        if (PdhGetFormattedCounterArrayA((HCOUNTER)m_pdhCounter, PDH_FMT_DOUBLE,
                                         &dwBufferSize, &dwItemCount,
                                         pItems) == ERROR_SUCCESS) {
          for (DWORD i = 0; i < dwItemCount; i++) {
            if (pItems[i].FmtValue.CStatus == ERROR_SUCCESS) {
              if (pItems[i].FmtValue.doubleValue > totalUsage) {
                totalUsage = pItems[i].FmtValue.doubleValue;
              }
            }
          }
        }
        free(pItems);
      }
    }
  }
  return std::min(100.0, totalUsage);
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
