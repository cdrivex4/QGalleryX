#include "SettingsHelper.h"
#include "AsyncImageProvider.h"
#include "HardwareAccelerationManager.h"
#include "LogManager.h"
#include "SystemMonitor.h"
#include <QCoreApplication>
#include <QDebug>
#include <QProcess>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QThread>

SettingsHelper::SettingsHelper(QObject *parent)
    : QObject(parent), m_settings("SamsungClone", "Gallery") {
  AsyncImageProvider::s_logLevel = logLevel();
  AsyncImageProvider::s_accelerateRaw = rawAcceleration();
  AsyncImageProvider::s_useDiskCache = useDiskCache();
  AsyncImageProvider::s_videoAcceleration = videoAcceleration();
  HardwareAccelerationManager::instance().setMode(
      static_cast<SettingsHelper::HWAccel>(
          AsyncImageProvider::s_videoAcceleration.load()));
}

QString SettingsHelper::graphicsApi() const { return m_graphicsApi; }
QString SettingsHelper::graphicsDriver() const { return m_graphicsDriver; }
QString SettingsHelper::graphicsProfile() const { return m_graphicsProfile; }

int SettingsHelper::selectedApi() const {
  return m_settings.value("graphicsApi", 0).toInt(); // 0 = Auto/Default
}

void SettingsHelper::setSelectedApi(int api) {
  if (selectedApi() == api)
    return;
  m_settings.setValue("graphicsApi", api);
  emit selectedApiChanged();
}

int SettingsHelper::thumbnailSize() const {
  return m_settings.value("thumbnailSize", 150).toInt();
}

void SettingsHelper::setThumbnailSize(int size) {
  if (thumbnailSize() == size)
    return;
  m_settings.setValue("thumbnailSize", size);
  emit thumbnailSizeChanged();
}

int SettingsHelper::cacheSizeMB() const {
  return m_settings.value("cacheSizeMB", 512).toInt();
}

void SettingsHelper::setCacheSizeMB(int sizeMB) {
  if (cacheSizeMB() == sizeMB)
    return;
  m_settings.setValue("cacheSizeMB", sizeMB);
  AsyncImageProvider::setCacheMaxCost(sizeMB * 1024);
  emit cacheSizeMBChanged();
}

int SettingsHelper::concurrentThreads() const {
  return m_settings.value("concurrentThreads", 4).toInt();
}

void SettingsHelper::setConcurrentThreads(int count) {
  if (concurrentThreads() == count)
    return;
  m_settings.setValue("concurrentThreads", count);
  emit concurrentThreadsChanged();
}

int SettingsHelper::gridSize() const {
  return m_settings.value("gridSize", 150).toInt();
}

void SettingsHelper::setGridSize(int size) {
  if (gridSize() == size)
    return;
  m_settings.setValue("gridSize", size);
  emit gridSizeChanged();
}

int SettingsHelper::logLevel() const {
  return m_settings.value("logLevel", 0).toInt();
}

void SettingsHelper::setLogLevel(int level) {
  if (logLevel() == level)
    return;
  m_settings.setValue("logLevel", level);
  AsyncImageProvider::s_logLevel = level;
  LogManager::instance().setLogLevel(level); // Apply to LogManager
  emit logLevelChanged();
}

bool SettingsHelper::rawAcceleration() const {
  return m_settings.value("rawAcceleration", true).toBool();
}

void SettingsHelper::setRawAcceleration(bool enable) {
  if (rawAcceleration() == enable)
    return;
  m_settings.setValue("rawAcceleration", enable);
  AsyncImageProvider::s_accelerateRaw = enable;
  emit rawAccelerationChanged();
}

bool SettingsHelper::useDiskCache() const {
  return m_settings.value("useDiskCache", false).toBool();
}

void SettingsHelper::setUseDiskCache(bool enable) {
  if (useDiskCache() == enable)
    return;
  m_settings.setValue("useDiskCache", enable);
  AsyncImageProvider::s_useDiskCache = enable;
  emit useDiskCacheChanged();
}

int SettingsHelper::videoAcceleration() const {
  return m_settings.value("videoAcceleration", 1)
      .toInt(); // Default to 1 (Auto) for GPU fallback chain
}

void SettingsHelper::setVideoAcceleration(int mode) {
  if (videoAcceleration() == mode)
    return;
  m_settings.setValue("videoAcceleration", mode);
  AsyncImageProvider::s_videoAcceleration = mode;
  HardwareAccelerationManager::instance().setMode(
      static_cast<SettingsHelper::HWAccel>(mode));
  emit videoAccelerationChanged();
}

bool SettingsHelper::showWatermark() const {
  return m_settings.value("showWatermark", false).toBool();
}

void SettingsHelper::setShowWatermark(bool show) {
  if (showWatermark() == show)
    return;
  m_settings.setValue("showWatermark", show);
  emit showWatermarkChanged();
}

bool SettingsHelper::showDiagnostics() const {
  return m_settings.value("showDiagnostics", false).toBool();
}

void SettingsHelper::setShowDiagnostics(bool show) {
  if (showDiagnostics() == show) return;
  m_settings.setValue("showDiagnostics", show);
  emit showDiagnosticsChanged();
}

bool SettingsHelper::useFastImage() const {
  return m_settings.value("useFastImage", true).toBool();
}

void SettingsHelper::setUseFastImage(bool use) {
  if (useFastImage() == use) return;
  m_settings.setValue("useFastImage", use);
  emit useFastImageChanged();
}

void SettingsHelper::restartApp() {
  qApp->quit();
  QProcess::startDetached(qApp->arguments()[0], qApp->arguments());
}

bool SettingsHelper::isApiSupported(int apiValue) {
  QSGRendererInterface::GraphicsApi api =
      static_cast<QSGRendererInterface::GraphicsApi>(apiValue);
  return QSGRendererInterface::isApiRhiBased(api);
}

QVariantMap SettingsHelper::getCacheStats() {
  return AsyncImageProvider::getCacheStats();
}

void SettingsHelper::clearDiskCache() { AsyncImageProvider::clearDiskCache(); }

QString SettingsHelper::getGpuName(QObject *window) {
  if (SystemMonitor::instance()) {
    return SystemMonitor::instance()->gpuName();
  }
  return "Unknown GPU";
}

void SettingsHelper::refreshGraphicsInfo(QObject *window) {
  if (!window)
    return;

  QQuickWindow *quickWindow = qobject_cast<QQuickWindow *>(window);
  if (quickWindow) {
    QSGRendererInterface *rif = quickWindow->rendererInterface();
    if (rif) {
      QSGRendererInterface::GraphicsApi api = rif->graphicsApi();
      switch (api) {
      case QSGRendererInterface::Direct3D11:
        m_graphicsApi = "Direct3D 11";
        break;
      case QSGRendererInterface::Vulkan:
        m_graphicsApi = "Vulkan";
        break;
      case QSGRendererInterface::OpenGL:
        m_graphicsApi = "OpenGL";
        break;
      case QSGRendererInterface::Software:
        m_graphicsApi = "Software";
        break;
      case QSGRendererInterface::Metal:
        m_graphicsApi = "Metal";
        break;
      default:
        m_graphicsApi = "Unknown";
        break;
      }
      emit graphicsApiChanged();
    }
  }
}
