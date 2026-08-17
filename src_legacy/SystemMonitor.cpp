#include "SystemMonitor.h"
#include <QDebug>
#include <QString>
#include <cmath>
#include <pdh.h>
#include <pdhmsg.h>
#include "TaskScheduler.h"


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

#ifdef Q_OS_WIN
  // PDH Init for GPU Load
  m_pdhQuery = nullptr;
  m_pdhCounter = nullptr;

  if (PdhOpenQueryA(NULL, 0, (HQUERY *)&m_pdhQuery) == ERROR_SUCCESS) {
    // Add wildcard counter for all GPU engines
    PdhAddEnglishCounterA((HQUERY)m_pdhQuery,
                          "\\GPU Engine(*)\\Utilization Percentage", 0,
                          (HCOUNTER *)&m_pdhCounter);
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

  // Periodic Queue Logging (Every 2 seconds)
  static int tick = 0;
  if (++tick >= 2) {
      int q0 = TaskScheduler::instance().getQueueSize(TaskScheduler::Immediate);
      int q1 = TaskScheduler::instance().getQueueSize(TaskScheduler::High);
      int q2 = TaskScheduler::instance().getQueueSize(TaskScheduler::Normal);
      if (q0 > 0 || q1 > 0 || q2 > 0) {
          qDebug() << "[TaskScheduler] Pending Image Tasks -> Immediate:" << q0 
                   << "| Lookahead:" << q1 << "| Precache:" << q2;
      }
      tick = 0;
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
  // 1. Dynamically enumerate all DXGI adapters to query active VRAM
  ComPtr<IDXGIFactory4> factory;
  if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
    UINT adapterIndex = 0;
    ComPtr<IDXGIAdapter> adapter;
    SIZE_T maxBudget = 0;
    double usedMB = 0.0;
    double totalMB = 0.0;
    QString currentBestName;

    while (factory->EnumAdapters(adapterIndex, adapter.GetAddressOf()) != DXGI_ERROR_NOT_FOUND) {
      ComPtr<IDXGIAdapter3> adapter3;
      if (SUCCEEDED(adapter.As(&adapter3))) {
        DXGI_ADAPTER_DESC1 desc;
        adapter3->GetDesc1(&desc);
        bool isSoftware = (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) || 
                          QString::fromWCharArray(desc.Description).contains("Basic Render", Qt::CaseInsensitive);

        DXGI_QUERY_VIDEO_MEMORY_INFO videoMemoryInfo;
        if (SUCCEEDED(adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &videoMemoryInfo))) {
          if (!isSoftware && (videoMemoryInfo.Budget > maxBudget || maxBudget == 0)) {
            maxBudget = videoMemoryInfo.Budget;
            usedMB = videoMemoryInfo.CurrentUsage / (1024.0 * 1024.0);
            totalMB = videoMemoryInfo.Budget / (1024.0 * 1024.0);
            currentBestName = QString::fromWCharArray(desc.Description);
          }
        }
      }
      adapter.Reset();
      adapterIndex++;
    }

    if (totalMB > 0) {
      m_gpuVramUsedMB = usedMB;
      m_gpuVramTotalMB = totalMB;
      if (!currentBestName.isEmpty() && currentBestName != m_gpuName) {
        m_gpuName = currentBestName;
        emit gpuNameChanged();
      }
    }
  }

  // 2. Update GPU Load (PDH)
  double maxUsage = 0.0;
  if (m_pdhQuery && m_pdhCounter) {
    PdhCollectQueryData((HQUERY)m_pdhQuery);

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
              if (pItems[i].FmtValue.doubleValue > maxUsage) {
                maxUsage = pItems[i].FmtValue.doubleValue;
              }
            }
          }
        }
        free(pItems);
      }
    }
  }

  return maxUsage;
#else
  return 0.0;
#endif
}

bool SystemMonitor::isSlowMedia(const QString &path) {
  QString type = getStorageType(path);
  return (type == "SD_CARD" || type == "NETWORK_SHARE");
}

QString SystemMonitor::getStorageType(const QString &path) {
#ifdef Q_OS_WIN
  QString cleanPath = path;
  if (cleanPath.startsWith("file:///", Qt::CaseInsensitive)) {
    cleanPath = cleanPath.mid(8);
  }
  
  if (cleanPath.startsWith("//") || cleanPath.startsWith("\\\\")) {
    return "NETWORK_SHARE";
  }

  wchar_t root[4] = {L'C', L':', L'\\', L'\0'};
  if (cleanPath.length() >= 2 && cleanPath[1] == L':') {
    root[0] = cleanPath[0].toUpper().toLatin1();
  }

  UINT driveType = GetDriveTypeW(root);
  switch (driveType) {
    case DRIVE_REMOVABLE:
      return "SD_CARD";
    case DRIVE_REMOTE:
      return "NETWORK_SHARE";
    case DRIVE_FIXED:
      return "NVME_SATA_SSD";
    default:
      return "UNKNOWN";
  }
#else
  Q_UNUSED(path);
  return "NVME_SATA_SSD";
#endif
}

QString SystemMonitor::getGpuName() {
#ifdef Q_OS_WIN
  ComPtr<IDXGIFactory1> factory;
  if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
    return "DXGI Error";
  }

  QString bestName = "Unknown GPU";
  SIZE_T maxDedicatedVram = 0;
  UINT adapterIndex = 0;
  ComPtr<IDXGIAdapter1> adapter;

  while (factory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND) {
    DXGI_ADAPTER_DESC1 desc;
    if (SUCCEEDED(adapter->GetDesc1(&desc))) {
      // Skip software rasterizers unless no physical adapter is found
      bool isSoftware = (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) || 
                        QString::fromWCharArray(desc.Description).contains("Basic Render", Qt::CaseInsensitive);
      
      if (!isSoftware && desc.DedicatedVideoMemory >= maxDedicatedVram) {
        maxDedicatedVram = desc.DedicatedVideoMemory;
        bestName = QString::fromWCharArray(desc.Description);
        m_gpuVramTotalMB = desc.DedicatedVideoMemory / (1024.0 * 1024.0);
      } else if (bestName == "Unknown GPU") {
        bestName = QString::fromWCharArray(desc.Description);
        m_gpuVramTotalMB = desc.DedicatedVideoMemory / (1024.0 * 1024.0);
      }
    }
    adapter.Reset();
    adapterIndex++;
  }

  return bestName;
#else
  return "Platform Not Supported";
#endif
}

