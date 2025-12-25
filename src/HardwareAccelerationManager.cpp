#include "HardwareAccelerationManager.h"
#include <QDateTime>
#include <QDebug>
#include <QMutexLocker>
#include <QSet>
#include <QStringList>

#ifdef _MSC_VER
#include <intrin.h>
#endif

// CPU feature detection
static QString detectCPUFeatures() {
  QStringList features;

#ifdef _MSC_VER
  int cpuInfo[4];
  __cpuid(cpuInfo, 1);

  if (cpuInfo[3] & (1 << 25))
    features << "SSE";
  if (cpuInfo[3] & (1 << 26))
    features << "SSE2";
  if (cpuInfo[2] & (1 << 0))
    features << "SSE3";
  if (cpuInfo[2] & (1 << 9))
    features << "SSSE3";
  if (cpuInfo[2] & (1 << 19))
    features << "SSE4.1";
  if (cpuInfo[2] & (1 << 20))
    features << "SSE4.2";
  if (cpuInfo[2] & (1 << 28))
    features << "AVX";

  // Check AVX2
  __cpuid(cpuInfo, 7);
  if (cpuInfo[1] & (1 << 5))
    features << "AVX2";

  // Check AVX512 (F, BW, DQ, VL)
  if (cpuInfo[1] & (1 << 16))
    features << "AVX512F";
  if (cpuInfo[1] & (1 << 30))
    features << "AVX512BW";
  if (cpuInfo[1] & (1 << 17))
    features << "AVX512DQ";
  if (cpuInfo[1] & (1 << 31))
    features << "AVX512VL";
#elif defined(__GNUC__) || defined(__clang__)
#if defined(__SSE__)
  features << "SSE";
#endif
#if defined(__SSE2__)
  features << "SSE2";
#endif
#if defined(__SSE3__)
  features << "SSE3";
#endif
#if defined(__SSSE3__)
  features << "SSSE3";
#endif
#if defined(__SSE4_1__)
  features << "SSE4.1";
#endif
#if defined(__SSE4_2__)
  features << "SSE4.2";
#endif
#if defined(__AVX__)
  features << "AVX";
#endif
#if defined(__AVX2__)
  features << "AVX2";
#endif
#if defined(__AVX512F__)
  features << "AVX512F";
#endif
#endif

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
  case SettingsHelper::D3D11VA:
    typeName = "d3d11va";
    targetPixFmt = AV_PIX_FMT_D3D11;
    break;
  case SettingsHelper::OpenCL:
    typeName = "opencl";
    targetPixFmt = AV_PIX_FMT_OPENCL;
    break;
  case SettingsHelper::None:
  default:
    return false; // Can't "initialize" None
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

  // Fall back sequence: requested -> D3D11VA -> OpenCL -> None (CPU)
  if (tryInitialize(mode, timeStr)) {
    return; // Success!
  }

  // Try D3D11 if not already tried
  if (mode != SettingsHelper::D3D11VA) {
    qInfo() << "[" << timeStr << "][HWAccel] Falling back to D3D11VA...";
    if (tryInitialize(SettingsHelper::D3D11VA, timeStr)) {
      return;
    }
  }

  // Try OpenCL as last GPU option
  if (mode != SettingsHelper::OpenCL) {
    qInfo() << "[" << timeStr << "][HWAccel] Falling back to OpenCL...";
    if (tryInitialize(SettingsHelper::OpenCL, timeStr)) {
      return;
    }
  }

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
  case SettingsHelper::D3D11VA:
    return "D3D11VA";
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
