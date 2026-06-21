#include "HardwareAccelerationManager.h"
#include <QDateTime>
#include <QDebug>
#include <QMutexLocker>
#include <QSet>
#include <QStringList>

#ifdef _MSC_VER
#include <intrin.h>
#elif defined(__GNUC__) || defined(__clang__)
#include <cpuid.h>
#endif

// Comprehensive runtime feature detection
static QString detectCPUFeatures() {
  QStringList features;
  int cpuInfo[4] = {0, 0, 0, 0};

#if defined(_MSC_VER)
  __cpuid(cpuInfo, 1);
#elif defined(__GNUC__) || defined(__clang__)
  __cpuid(1, cpuInfo[0], cpuInfo[1], cpuInfo[2], cpuInfo[3]);
#endif

  if (cpuInfo[3] & (1 << 25)) features << "SSE";
  if (cpuInfo[3] & (1 << 26)) features << "SSE2";
  if (cpuInfo[2] & (1 << 0))  features << "SSE3";
  if (cpuInfo[2] & (1 << 9))  features << "SSSE3";
  if (cpuInfo[2] & (1 << 19)) features << "SSE4.1";
  if (cpuInfo[2] & (1 << 20)) features << "SSE4.2";
  if (cpuInfo[2] & (1 << 28)) features << "AVX";

#if defined(_MSC_VER)
  __cpuid(cpuInfo, 7);
#elif defined(__GNUC__) || defined(__clang__)
  __cpuid_count(7, 0, cpuInfo[0], cpuInfo[1], cpuInfo[2], cpuInfo[3]);
#endif

  if (cpuInfo[1] & (1 << 5))  features << "AVX2";
  if (cpuInfo[1] & (1 << 16)) features << "AVX512F";
  if (cpuInfo[1] & (1 << 30)) features << "AVX512BW";
  if (cpuInfo[1] & (1 << 17)) features << "AVX512DQ";
  if (cpuInfo[1] & (1 << 31)) features << "AVX512VL";

  // Combine with available GPU media engines from FFmpeg
  QStringList gpuFeatures;
  AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
  while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE) {
    const char *name = av_hwdevice_get_type_name(type);
    if (name) {
      gpuFeatures << QString::fromUtf8(name).toUpper();
    }
  }

  if (!gpuFeatures.isEmpty()) {
    features << gpuFeatures;
  }

  return features.isEmpty() ? "None detected" : features.join(", ");
}

HardwareAccelerationManager &HardwareAccelerationManager::instance() {
  static HardwareAccelerationManager instance;
  return instance;
}

HardwareAccelerationManager::HardwareAccelerationManager(QObject *parent)
    : QObject(parent) {}

HardwareAccelerationManager::~HardwareAccelerationManager() { cleanup(); }

void HardwareAccelerationManager::cleanup() {
  QMutexLocker locker(&m_mutex);
  if (m_deviceCtx) {
    av_buffer_unref(&m_deviceCtx);
    m_deviceCtx = nullptr;
  }
  m_pixFmt = AV_PIX_FMT_NONE;
}

bool HardwareAccelerationManager::tryInitialize(SettingsHelper::HWAccel mode,
                                                const QString &timeStr) {
  const char *typeName = nullptr;
  AVPixelFormat targetPixFmt = AV_PIX_FMT_NONE;

  switch (mode) {
  case SettingsHelper::CUDA:
    typeName = "cuda";
    targetPixFmt = AV_PIX_FMT_CUDA;
    break;
  case SettingsHelper::QSV:
    typeName = "qsv";
    targetPixFmt = AV_PIX_FMT_QSV;
    break;
  case SettingsHelper::D3D11VA:
    typeName = "d3d11va";
    targetPixFmt = AV_PIX_FMT_D3D11;
    break;
  case SettingsHelper::DXVA2:
    typeName = "dxva2";
    targetPixFmt = AV_PIX_FMT_DXVA2_VLD;
    break;
  case SettingsHelper::OpenCL:
    typeName = "opencl";
    targetPixFmt = AV_PIX_FMT_OPENCL;
    break;
  case SettingsHelper::Auto:
  case SettingsHelper::None:
  default:
    return false; // Can't "initialize" None or Auto directly
  }

  AVHWDeviceType type = av_hwdevice_find_type_by_name(typeName);
  if (type == AV_HWDEVICE_TYPE_NONE) {
    qDebug() << "[" << timeStr << "][HWAccel]" << typeName
             << "not supported by FFmpeg";
    return false;
  }

  qDebug() << "[" << timeStr << "][HWAccel] Attempting" << typeName << "...";

  int err = av_hwdevice_ctx_create(&m_deviceCtx, type, nullptr, nullptr, 0);
  if (err < 0) {
    qDebug() << "[" << timeStr << "][HWAccel]" << typeName << "failed (error"
             << err << ")";
    return false;
  }

  m_pixFmt = targetPixFmt;
  m_currentMode = mode;
  qInfo() << "[" << timeStr << "][HWAccel] SUCCESS:" << typeName
          << "initialized";
  return true;
}

void HardwareAccelerationManager::setMode(SettingsHelper::HWAccel mode) {
  QMutexLocker locker(&m_mutex);
  if (m_currentMode == mode && m_deviceCtx)
    return;

  QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");

  // SAFETY: Only block Vulkan (causes system crashes)
  if (mode == SettingsHelper::Vulkan) {
    qWarning() << "[" << timeStr
               << "][HWAccel] BLOCKED: Vulkan causes system crashes.";
    mode = SettingsHelper::D3D11VA; // Try D3D11 instead
  }

  qDebug() << "[" << timeStr << "][HWAccel] Requested mode:" << mode;

  // Cleanup old context
  if (m_deviceCtx) {
    av_buffer_unref(&m_deviceCtx);
    m_deviceCtx = nullptr;
  }

  // If user explicitly requested None, skip GPU entirely
  if (mode == SettingsHelper::None) {
    m_currentMode = SettingsHelper::None;
    m_pixFmt = AV_PIX_FMT_NONE;
    QString cpuFeatures = detectCPUFeatures();
    qInfo() << "[" << timeStr
            << "][HWAccel] CPU decoding selected (no acceleration).";
    qInfo() << "[" << timeStr << "][HWAccel] CPU Features:" << cpuFeatures;
    return;
  }

  // Fall back sequence: requested -> Auto chain -> None (CPU)
  if (mode != SettingsHelper::Auto && tryInitialize(mode, timeStr)) {
    return; // Success on explicitly requested mode!
  }

  // Auto Fallback Chain: CUDA -> QSV -> D3D11VA -> DXVA2
  qInfo() << "[" << timeStr << "][HWAccel] Initiating Auto Fallback Chain...";
  
  if (tryInitialize(SettingsHelper::CUDA, timeStr)) return;
  if (tryInitialize(SettingsHelper::QSV, timeStr)) return;
  if (tryInitialize(SettingsHelper::D3D11VA, timeStr)) return;
  if (tryInitialize(SettingsHelper::DXVA2, timeStr)) return;

  // Fall back to CPU (None)
  m_currentMode = SettingsHelper::None;
  m_pixFmt = AV_PIX_FMT_NONE;
  QString cpuFeatures = detectCPUFeatures();
  qWarning() << "[" << timeStr
             << "][HWAccel] All GPU backends unavailable. Falling back to CPU "
                "decoding.";
  qInfo() << "[" << timeStr << "][HWAccel] CPU Features:" << cpuFeatures;
}

AVBufferRef *HardwareAccelerationManager::deviceContext() const {
  QMutexLocker locker(&m_mutex);
  return m_deviceCtx ? av_buffer_ref(m_deviceCtx) : nullptr;
}

AVPixelFormat HardwareAccelerationManager::pixelFormat() const {
  QMutexLocker locker(&m_mutex);
  return m_pixFmt;
}

QString HardwareAccelerationManager::currentModeName() const {
  QMutexLocker locker(&m_mutex);
  switch (m_currentMode) {
  case SettingsHelper::Auto:
    return "Auto";
  case SettingsHelper::CUDA:
    return "CUDA (NVIDIA)";
  case SettingsHelper::QSV:
    return "QSV (Intel QuickSync)";
  case SettingsHelper::D3D11VA:
    return "D3D11VA";
  case SettingsHelper::DXVA2:
    return "DXVA2";
  case SettingsHelper::Vulkan:
    return "Vulkan";
  case SettingsHelper::OpenCL:
    return "OpenCL";
  default:
    return "None (CPU)";
  }
}

QString HardwareAccelerationManager::cpuInfo() const {
  return detectCPUFeatures();
}
