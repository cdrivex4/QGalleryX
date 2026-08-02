#include "SettingsHelper.h"
#include "AsyncImageProvider.h"
#include "../src/FileCacheManager.h"
#include "LogManager.h"
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
  // Also push to AsyncImageProvider immediately (Cost is in KB)
  AsyncImageProvider::setCacheMaxCost(sizeMB * 1024);
  emit cacheSizeMBChanged();
}

int SettingsHelper::diskCacheSizeMB() const {
  return m_settings.value("diskCacheSizeMB", 4096).toInt();
}

void SettingsHelper::setDiskCacheSizeMB(int sizeMB) {
  if (diskCacheSizeMB() == sizeMB)
    return;
  m_settings.setValue("diskCacheSizeMB", sizeMB);
  FileCacheManager::instance().setMaxDiskCacheSizeMB(sizeMB);
  emit diskCacheSizeMBChanged();
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

int SettingsHelper::gridResolution() const {
  // Keep underlying settings key as "gridSize" for backwards compatibility
  return m_settings.value("gridSize", 150).toInt();
}

void SettingsHelper::setGridResolution(int size) {
  if (gridResolution() == size)
    return;
  m_settings.setValue("gridSize", size);
  emit gridResolutionChanged();
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

#include "../src/FileCacheManager.h"
#include <QFileInfo>

QString SettingsHelper::getDiskCachePath() {
    return FileCacheManager::instance().getDbPath();
}

qint64 SettingsHelper::getDiskCacheUsage() {
    return QFileInfo(FileCacheManager::instance().getDbPath()).size();
}

void SettingsHelper::nukeDiskCache() {
    FileCacheManager::instance().nukeCache();
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
