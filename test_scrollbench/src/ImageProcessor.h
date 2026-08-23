#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include <QObject>
#include <QString>

class ImageProcessor : public QObject {
  Q_OBJECT
public:
  explicit ImageProcessor(QObject *parent = nullptr);

  Q_INVOKABLE void rotateImageVirtual(const QString &sourcePath, int degrees);
  Q_INVOKABLE int getVirtualRotation(const QString &sourcePath);
  Q_INVOKABLE void clearImageCache();
};

#endif // IMAGEPROCESSOR_H
