#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QImage>

class ImageProcessor : public QObject {
  Q_OBJECT
public:
  explicit ImageProcessor(QObject *parent = nullptr);

  Q_INVOKABLE void rotateImageVirtual(const QString &sourcePath, int degrees);
  Q_INVOKABLE int getVirtualRotation(const QString &sourcePath);
  Q_INVOKABLE void clearImageCache();

  // Filter & Color Science Engine
  Q_INVOKABLE QStringList getAvailableFilters() const;
  Q_INVOKABLE QString applyFilterPreview(const QString &sourcePath,
                                         const QString &filterName,
                                         qreal intensity,
                                         qreal exposure = 0.0,
                                         qreal contrast = 0.0,
                                         qreal saturation = 0.0,
                                         qreal temperature = 0.0);

  // Full High-Resolution Composite Export Engine
  Q_INVOKABLE bool saveEditedImage(const QString &sourcePath,
                                   const QString &destPath,
                                   const QString &filterName,
                                   qreal filterIntensity,
                                   qreal exposure,
                                   qreal contrast,
                                   qreal saturation,
                                   qreal temperature,
                                   qreal cropX, qreal cropY, qreal cropW, qreal cropH,
                                   int rotationDegrees,
                                   bool flipH, bool flipV,
                                   const QVariantList &annotations,
                                   int quality = 92);

  // Helper algorithms
  static QImage processFiltersAndAdjustments(const QImage &src,
                                            const QString &filterName,
                                            qreal intensity,
                                            qreal exposure,
                                            qreal contrast,
                                            qreal saturation,
                                            qreal temperature);

  static QImage applySingleFilter(const QImage &src, const QString &filterName, qreal intensity);
  static void renderAnnotationsOnImage(QImage &target, const QVariantList &annotations, const QSize &canvasSize);
};

#endif // IMAGEPROCESSOR_H
