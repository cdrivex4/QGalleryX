#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include <QObject>
#include <QSize>
#include <QString>

class ImageProcessor : public QObject {
  Q_OBJECT
public:
  explicit ImageProcessor(QObject *parent = nullptr);

  Q_INVOKABLE bool resizeImage(const QString &sourcePath,
                               const QString &destinationPath,
                               const QSize &targetSize);
  Q_INVOKABLE bool rotateImage(const QString &sourcePath, int degrees);
  Q_INVOKABLE void rotateImageVirtual(const QString &sourcePath, int degrees);
  Q_INVOKABLE int getVirtualRotation(const QString &sourcePath);
  Q_INVOKABLE void clearImageCache();

signals:
  void imageProcessingError(const QString &message);

private:
};

#endif // IMAGEPROCESSOR_H
