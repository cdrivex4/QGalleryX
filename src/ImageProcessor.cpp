#include "ImageProcessor.h"
#include "../src_legacy/AsyncImageProvider.h"
#include "FileCacheManager.h"
#include <QDebug>
#include <QDir>
#include <QSettings>

ImageProcessor::ImageProcessor(QObject *parent) : QObject(parent) {}

void ImageProcessor::clearImageCache() {
  AsyncImageProvider::clearCache();
  FileCacheManager::instance().clearCache();
  qDebug() << "ImageProcessor: Image cache cleared (both RAM and Disk).";
}

void ImageProcessor::rotateImageVirtual(const QString &sourcePath, int degrees) {
  QString cleanPath = sourcePath;
  if (cleanPath.startsWith("file:///")) cleanPath = cleanPath.mid(8);
  cleanPath = QDir::fromNativeSeparators(cleanPath);

  QSettings settings("SamsungClone", "VirtualRotations");
  int current = settings.value(cleanPath, 0).toInt();
  if (current == 0) current = settings.value(sourcePath, 0).toInt();
  current = (current + degrees) % 360;
  if (current < 0) current += 360;
  settings.setValue(cleanPath, current);
  settings.setValue(sourcePath, current);
  settings.setValue(QDir::toNativeSeparators(cleanPath), current);
  settings.sync();
  qDebug() << "ImageProcessor: Virtual rotation set to" << current << "for" << cleanPath;
}

int ImageProcessor::getVirtualRotation(const QString &sourcePath) {
  QString cleanPath = sourcePath;
  if (cleanPath.startsWith("file:///")) cleanPath = cleanPath.mid(8);
  cleanPath = QDir::fromNativeSeparators(cleanPath);

  QSettings settings("SamsungClone", "VirtualRotations");
  int rot = settings.value(cleanPath, 0).toInt();
  if (rot == 0) rot = settings.value(sourcePath, 0).toInt();
  if (rot == 0) rot = settings.value(QDir::toNativeSeparators(cleanPath), 0).toInt();
  return rot;
}
