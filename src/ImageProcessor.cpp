#include "ImageProcessor.h"
#include "../src_legacy/AsyncImageProvider.h"
#include "FileCacheManager.h"
#include <QDebug>
#include <QImage>
#include <QSettings>

ImageProcessor::ImageProcessor(QObject *parent) : QObject(parent) {}

bool ImageProcessor::resizeImage(const QString &sourcePath,
                                 const QString &destinationPath,
                                 const QSize &targetSize) {
  QImage image;
  if (!image.load(sourcePath)) {
    emit imageProcessingError(
        QString("Failed to load image from %1").arg(sourcePath));
    qWarning() << "ImageProcessor: Failed to load image from" << sourcePath;
    return false;
  }

  QImage scaledImage =
      image.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

  if (!scaledImage.save(destinationPath)) {
    emit imageProcessingError(
        QString("Failed to save resized image to %1").arg(destinationPath));
    qWarning() << "ImageProcessor: Failed to save resized image to"
               << destinationPath;
    return false;
  }

  qDebug() << "ImageProcessor: Successfully resized" << sourcePath << "to"
           << targetSize << "and saved to" << destinationPath;
  return true;
}

bool ImageProcessor::rotateImage(const QString &sourcePath, int degrees) {
  QImage image;
  if (!image.load(sourcePath)) {
    emit imageProcessingError(
        QString("Failed to load image from %1").arg(sourcePath));
    return false;
  }

  QTransform transform;
  transform.rotate(degrees);
  QImage rotatedImage = image.transformed(transform, Qt::SmoothTransformation);

  if (!rotatedImage.save(sourcePath)) {
    emit imageProcessingError(
        QString("Failed to save rotated image to %1").arg(sourcePath));
    return false;
  }

  qDebug() << "ImageProcessor: Successfully rotated" << sourcePath << "by"
           << degrees << "degrees";
  return true;
}

void ImageProcessor::clearImageCache() {
  AsyncImageProvider::clearCache();
  FileCacheManager::instance().clearCache();
  qDebug() << "ImageProcessor: Image cache cleared (both RAM and Disk).";
}

void ImageProcessor::rotateImageVirtual(const QString &sourcePath, int degrees) {
  QSettings settings("SamsungClone", "VirtualRotations");
  int current = settings.value(sourcePath, 0).toInt();
  current = (current + degrees) % 360;
  if (current < 0) current += 360;
  settings.setValue(sourcePath, current);
  qDebug() << "ImageProcessor: Virtual rotation set to" << current << "for" << sourcePath;
}

int ImageProcessor::getVirtualRotation(const QString &sourcePath) {
  QSettings settings("SamsungClone", "VirtualRotations");
  return settings.value(sourcePath, 0).toInt();
}
