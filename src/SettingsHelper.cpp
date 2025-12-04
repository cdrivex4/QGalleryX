#include "SettingsHelper.h"
#include "AsyncImageProvider.h"
#include <QCoreApplication>
#include <QDebug>
#include <QProcess>
#include <QSGRendererInterface>
#include <QThread>

SettingsHelper::SettingsHelper(QObject *parent)
    : QObject(parent), m_settings("SamsungClone", "Gallery") {
  AsyncImageProvider::s_logLevel = logLevel();
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
  emit logLevelChanged();
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
