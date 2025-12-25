#include "ImageProcessor.h"
#include <QImage>
#include <QDebug>

ImageProcessor::ImageProcessor(QObject *parent) : QObject(parent)
{
}

bool ImageProcessor::resizeImage(const QString &sourcePath, const QString &destinationPath, const QSize &targetSize)
{
    QImage image;
    if (!image.load(sourcePath)) {
        emit imageProcessingError(QString("Failed to load image from %1").arg(sourcePath));
        qWarning() << "ImageProcessor: Failed to load image from" << sourcePath;
        return false;
    }

    QImage scaledImage = image.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    if (!scaledImage.save(destinationPath)) {
        emit imageProcessingError(QString("Failed to save resized image to %1").arg(destinationPath));
        qWarning() << "ImageProcessor: Failed to save resized image to" << destinationPath;
        return false;
    }

    qDebug() << "ImageProcessor: Successfully resized" << sourcePath << "to" << targetSize << "and saved to" << destinationPath;
    return true;
}
