#include "ImageProcessor.h"
#include "../src_legacy/AsyncImageProvider.h"
#include "FileCacheManager.h"
#include <QDebug>
#include <QDir>
#include <QSettings>
#include <QImageReader>
#include <QImageWriter>
#include <QPainter>
#include <QPainterPath>
#include <QFont>
#include <QStandardPaths>
#include <cmath>
#include <algorithm>

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

QStringList ImageProcessor::getAvailableFilters() const {
  return QStringList{
    "None",
    "Auto Balance",
    "Ekta E100g",
    "Ekta E100VS",
    "Velvia 100",
    "Provia 100F",
    "Astia 100F",
    "Sunbeam",
    "Amber",
    "Shadow",
    "Shade",
    "Glow",
    "Crystal"
  };
}

QString ImageProcessor::applyFilterPreview(const QString &sourcePath,
                                           const QString &filterName,
                                           qreal intensity,
                                           qreal exposure,
                                           qreal contrast,
                                           qreal saturation,
                                           qreal temperature) {
  QString cleanPath = sourcePath;
  if (cleanPath.startsWith("file:///")) cleanPath = cleanPath.mid(8);
  cleanPath = QDir::fromNativeSeparators(cleanPath);

  QImageReader reader(cleanPath);
  reader.setAutoTransform(true);
  QSize origSize = reader.size();
  if (origSize.isValid()) {
    QSize scaled = origSize.scaled(1024, 768, Qt::KeepAspectRatio);
    reader.setScaledSize(scaled);
  }
  QImage img = reader.read();
  if (img.isNull()) return "";

  QImage filtered = processFiltersAndAdjustments(img, filterName, intensity, exposure, contrast, saturation, temperature);
  QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
  QString tempPath = tempDir + "/qg_preview_live.jpg";
  filtered.save(tempPath, "JPEG", 88);

  return "file:///" + QDir::fromNativeSeparators(tempPath);
}

static inline int clampPixel(int val) {
  return std::clamp(val, 0, 255);
}

QImage ImageProcessor::applySingleFilter(const QImage &src, const QString &filterName, qreal intensity) {
  if (filterName.isEmpty() || filterName == "None" || intensity <= 0.001) {
    return src;
  }

  intensity = std::clamp(intensity, 0.0, 1.0);
  QImage base = src.convertToFormat(QImage::Format_ARGB32);
  QImage dst = base;
  int width = dst.width();
  int height = dst.height();

  if (filterName == "Auto Balance") {
    // Histogram Equalization & Gray-World Auto White Balance
    quint64 totalR = 0, totalG = 0, totalB = 0;
    int minR = 255, maxR = 0, minG = 255, maxG = 0, minB = 255, maxB = 0;
    quint64 totalPixels = (quint64)width * height;

    for (int y = 0; y < height; ++y) {
      const QRgb *scan = (const QRgb *)base.constScanLine(y);
      for (int x = 0; x < width; ++x) {
        int r = qRed(scan[x]), g = qGreen(scan[x]), b = qBlue(scan[x]);
        totalR += r; totalG += g; totalB += b;
        if (r < minR) minR = r; if (r > maxR) maxR = r;
        if (g < minG) minG = g; if (g > maxG) maxG = g;
        if (b < minB) minB = b; if (b > maxB) maxB = b;
      }
    }

    double avgR = (double)totalR / totalPixels;
    double avgG = (double)totalG / totalPixels;
    double avgB = (double)totalB / totalPixels;
    double gray = (avgR + avgG + avgB) / 3.0;

    double scaleR = (avgR > 1.0) ? (gray / avgR) : 1.0;
    double scaleG = (avgG > 1.0) ? (gray / avgG) : 1.0;
    double scaleB = (avgB > 1.0) ? (gray / avgB) : 1.0;

    int rangeR = std::max(1, maxR - minR);
    int rangeG = std::max(1, maxG - minG);
    int rangeB = std::max(1, maxB - minB);

    for (int y = 0; y < height; ++y) {
      QRgb *line = (QRgb *)dst.scanLine(y);
      for (int x = 0; x < width; ++x) {
        int r = qRed(line[x]);
        int g = qGreen(line[x]);
        int b = qBlue(line[x]);

        int strR = clampPixel((int)(((r - minR) * 255.0 / rangeR) * scaleR));
        int strG = clampPixel((int)(((g - minG) * 255.0 / rangeG) * scaleG));
        int strB = clampPixel((int)(((b - minB) * 255.0 / rangeB) * scaleB));

        int finalR = clampPixel((int)(r * (1.0 - intensity) + strR * intensity));
        int finalG = clampPixel((int)(g * (1.0 - intensity) + strG * intensity));
        int finalB = clampPixel((int)(b * (1.0 - intensity) + strB * intensity));

        line[x] = qRgba(finalR, finalG, finalB, qAlpha(line[x]));
      }
    }
    return dst;
  }

  // Pixel-by-pixel color science transforms
  for (int y = 0; y < height; ++y) {
    QRgb *line = (QRgb *)dst.scanLine(y);
    for (int x = 0; x < width; ++x) {
      int r = qRed(line[x]);
      int g = qGreen(line[x]);
      int b = qBlue(line[x]);
      int a = qAlpha(line[x]);

      double nr = r / 255.0;
      double ng = g / 255.0;
      double nb = b / 255.0;

      double outR = nr, outG = ng, outB = nb;

      if (filterName == "Ekta E100g") {
        // Kodak Ektachrome E100G: Natural saturation, vibrant blues & emeralds, smooth highlights
        outR = std::pow(nr, 0.94) * 1.02;
        outG = std::pow(ng, 0.90) * 1.05;
        outB = std::pow(nb, 0.88) * 1.08;
      } else if (filterName == "Ekta E100VS") {
        // Kodak Ektachrome E100VS: Vivid Saturation, punchy S-curve, deep contrast
        outR = 1.0 / (1.0 + std::exp(-10.0 * (nr - 0.48)));
        outG = 1.0 / (1.0 + std::exp(-10.0 * (ng - 0.48)));
        outB = 1.0 / (1.0 + std::exp(-10.0 * (nb - 0.48)));
        outR *= 1.15; outG *= 1.12; outB *= 1.18;
      } else if (filterName == "Velvia 100") {
        // Fujichrome Velvia 100: Ultra-saturated landscape, deep blacks, emerald greens, magenta highlights
        outR = std::pow(nr, 1.15) * 1.18;
        outG = std::pow(ng, 0.92) * 1.22;
        outB = std::pow(nb, 1.10) * 1.25;
      } else if (filterName == "Provia 100F") {
        // Fujichrome Provia 100F: True-to-life fidelity, neutral grain, fine tonal gradation
        outR = std::pow(nr, 0.98) * 1.03;
        outG = std::pow(ng, 0.97) * 1.03;
        outB = std::pow(nb, 0.96) * 1.04;
      } else if (filterName == "Astia 100F") {
        // Fujichrome Astia 100F: Soft portrait curve, delicate skin tones, smooth highlight rolloff
        outR = std::pow(nr, 0.92) * 1.04;
        outG = std::pow(ng, 0.94) * 1.02;
        outB = std::pow(nb, 0.98) * 0.98;
      } else if (filterName == "Sunbeam") {
        // Warm golden hour highlight bloom + amber warmth
        outR = nr + 0.12 * (1.0 - nr);
        outG = ng + 0.06 * (1.0 - ng);
        outB = nb * 0.88;
      } else if (filterName == "Amber") {
        // Golden-amber vintage split tone
        outR = nr * 1.20 + 0.05;
        outG = ng * 1.08 + 0.02;
        outB = nb * 0.78;
      } else if (filterName == "Shadow") {
        // Deep crushed blacks, cool blue shadow tint
        outR = std::pow(nr, 1.35) * 0.95;
        outG = std::pow(ng, 1.30) * 0.98;
        outB = std::pow(nb, 1.20) * 1.10;
      } else if (filterName == "Shade") {
        // Soft muted contrast, cool ambient undertones
        outR = nr * 0.92 + 0.04;
        outG = ng * 0.96 + 0.04;
        outB = nb * 1.08 + 0.06;
      } else if (filterName == "Glow") {
        // Highlight bloom & gentle softening
        double lum = 0.299 * nr + 0.587 * ng + 0.114 * nb;
        double bloom = (lum > 0.6) ? (lum - 0.6) * 0.8 : 0.0;
        outR = nr + bloom;
        outG = ng + bloom;
        outB = nb + bloom;
      } else if (filterName == "Crystal") {
        // High clarity micro-contrast, edge sharpness, cool high-key tone
        outR = std::pow(nr, 1.10) * 0.98;
        outG = std::pow(ng, 1.05) * 1.02;
        outB = std::pow(nb, 0.95) * 1.12;
      }

      int resR = clampPixel((int)((nr * (1.0 - intensity) + outR * intensity) * 255.0));
      int resG = clampPixel((int)((ng * (1.0 - intensity) + outG * intensity) * 255.0));
      int resB = clampPixel((int)((nb * (1.0 - intensity) + outB * intensity) * 255.0));

      line[x] = qRgba(resR, resG, resB, a);
    }
  }

  return dst;
}

QImage ImageProcessor::processFiltersAndAdjustments(const QImage &src,
                                                   const QString &filterName,
                                                   qreal intensity,
                                                   qreal exposure,
                                                   qreal contrast,
                                                   qreal saturation,
                                                   qreal temperature) {
  QImage img = applySingleFilter(src, filterName, intensity);
  if (std::abs(exposure) < 0.001 && std::abs(contrast) < 0.001 &&
      std::abs(saturation) < 0.001 && std::abs(temperature) < 0.001) {
    return img;
  }

  img = img.convertToFormat(QImage::Format_ARGB32);
  int width = img.width();
  int height = img.height();

  double expFactor = std::pow(2.0, exposure);
  double contFactor = (contrast >= 0) ? (1.0 + contrast * 1.5) : (1.0 + contrast * 0.8);
  double satFactor = (saturation >= 0) ? (1.0 + saturation * 1.5) : (1.0 + saturation);

  for (int y = 0; y < height; ++y) {
    QRgb *line = (QRgb *)img.scanLine(y);
    for (int x = 0; x < width; ++x) {
      int r = qRed(line[x]);
      int g = qGreen(line[x]);
      int b = qBlue(line[x]);
      int a = qAlpha(line[x]);

      // Exposure
      double dr = r * expFactor;
      double dg = g * expFactor;
      double db = b * expFactor;

      // Temperature
      if (temperature > 0) {
        dr += temperature * 30.0;
        db -= temperature * 30.0;
      } else if (temperature < 0) {
        dr += temperature * 20.0;
        db -= temperature * 35.0;
      }

      // Contrast
      dr = (dr - 128.0) * contFactor + 128.0;
      dg = (dg - 128.0) * contFactor + 128.0;
      db = (db - 128.0) * contFactor + 128.0;

      // Saturation
      double gray = 0.299 * dr + 0.587 * dg + 0.114 * db;
      dr = gray + (dr - gray) * satFactor;
      dg = gray + (dg - gray) * satFactor;
      db = gray + (db - gray) * satFactor;

      line[x] = qRgba(clampPixel((int)dr), clampPixel((int)dg), clampPixel((int)db), a);
    }
  }

  return img;
}

void ImageProcessor::renderAnnotationsOnImage(QImage &target, const QVariantList &annotations, const QSize &canvasSize) {
  if (annotations.isEmpty()) return;

  QPainter p(&target);
  p.setRenderHint(QPainter::Antialiasing, true);
  p.setRenderHint(QPainter::SmoothPixmapTransform, true);
  p.setRenderHint(QPainter::TextAntialiasing, true);

  double scaleX = canvasSize.width() > 0 ? ((double)target.width() / canvasSize.width()) : 1.0;
  double scaleY = canvasSize.height() > 0 ? ((double)target.height() / canvasSize.height()) : 1.0;

  for (const QVariant &item : annotations) {
    QVariantMap map = item.toMap();
    QString type = map.value("type").toString();
    QColor color(map.value("color", "#FFFFFF").toString());
    qreal width = map.value("width", 4.0).toReal() * std::min(scaleX, scaleY);
    qreal opacity = map.value("opacity", 1.0).toReal();
    color.setAlphaF(std::clamp(opacity, 0.0, 1.0));

    if (type == "pen") {
      QVariantList points = map.value("points").toList();
      if (points.size() >= 2) {
        QPainterPath path;
        QVariantMap p0 = points.first().toMap();
        path.moveTo(p0.value("x").toReal() * scaleX, p0.value("y").toReal() * scaleY);
        for (int i = 1; i < points.size(); ++i) {
          QVariantMap pt = points[i].toMap();
          path.lineTo(pt.value("x").toReal() * scaleX, pt.value("y").toReal() * scaleY);
        }
        QPen pen(color, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.strokePath(path, pen);
      }
    } else if (type == "arrow") {
      qreal x1 = map.value("x1").toReal() * scaleX;
      qreal y1 = map.value("y1").toReal() * scaleY;
      qreal x2 = map.value("x2").toReal() * scaleX;
      qreal y2 = map.value("y2").toReal() * scaleY;

      QPen pen(color, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
      p.setPen(pen);
      p.drawLine(QPointF(x1, y1), QPointF(x2, y2));

      // Arrowhead
      double angle = std::atan2(y2 - y1, x2 - x1);
      double headLen = std::max(12.0, width * 3.5);
      QPointF pA(x2 - headLen * std::cos(angle - M_PI / 6.0), y2 - headLen * std::sin(angle - M_PI / 6.0));
      QPointF pB(x2 - headLen * std::cos(angle + M_PI / 6.0), y2 - headLen * std::sin(angle + M_PI / 6.0));

      QPainterPath headPath;
      headPath.moveTo(x2, y2);
      headPath.lineTo(pA);
      headPath.lineTo(pB);
      headPath.closeSubpath();
      p.fillPath(headPath, color);
    } else if (type == "rect") {
      qreal rx = map.value("x").toReal() * scaleX;
      qreal ry = map.value("y").toReal() * scaleY;
      qreal rw = map.value("w").toReal() * scaleX;
      qreal rh = map.value("h").toReal() * scaleY;
      QPen pen(color, width);
      p.setPen(pen);
      p.setBrush(Qt::NoBrush);
      p.drawRoundedRect(QRectF(rx, ry, rw, rh), 6 * scaleX, 6 * scaleY);
    } else if (type == "circle") {
      qreal cx = map.value("x").toReal() * scaleX;
      qreal cy = map.value("y").toReal() * scaleY;
      qreal cw = map.value("w").toReal() * scaleX;
      qreal ch = map.value("h").toReal() * scaleY;
      QPen pen(color, width);
      p.setPen(pen);
      p.setBrush(Qt::NoBrush);
      p.drawEllipse(QRectF(cx, cy, cw, ch));
    } else if (type == "text") {
      qreal tx = map.value("x").toReal() * scaleX;
      qreal ty = map.value("y").toReal() * scaleY;
      QString text = map.value("text").toString();
      int fontSize = std::max(12, (int)(map.value("fontSize", 20).toInt() * std::min(scaleX, scaleY)));
      QFont font("Segoe UI", fontSize, QFont::Bold);
      p.setFont(font);

      QFontMetrics fm(font);
      QRect textRect = fm.boundingRect(text);
      QRect pillRect(tx - 6 * scaleX, ty - fm.ascent() - 4 * scaleY, textRect.width() + 12 * scaleX, textRect.height() + 8 * scaleY);

      // Draw background pill
      p.setPen(Qt::NoPen);
      p.setBrush(QColor(20, 20, 20, 180));
      p.drawRoundedRect(pillRect, 6 * scaleX, 6 * scaleY);

      // Draw text
      p.setPen(color);
      p.drawText(QPointF(tx, ty), text);
    }
  }
}

bool ImageProcessor::saveEditedImage(const QString &sourcePath,
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
                                     int quality) {
  QString cleanSrc = sourcePath;
  if (cleanSrc.startsWith("file:///")) cleanSrc = cleanSrc.mid(8);
  cleanSrc = QDir::fromNativeSeparators(cleanSrc);

  QString cleanDst = destPath;
  if (cleanDst.startsWith("file:///")) cleanDst = cleanDst.mid(8);
  cleanDst = QDir::fromNativeSeparators(cleanDst);

  QImageReader reader(cleanSrc);
  reader.setAutoTransform(true);
  QImage img = reader.read();
  if (img.isNull()) {
    qWarning() << "[ImageProcessor] Failed to read source for editing:" << cleanSrc;
    return false;
  }

  // 1. Rotation & Flip Transforms
  if (rotationDegrees != 0 || flipH || flipV) {
    QTransform transform;
    if (rotationDegrees != 0) transform.rotate(rotationDegrees);
    if (flipH || flipV) transform.scale(flipH ? -1 : 1, flipV ? -1 : 1);
    img = img.transformed(transform, Qt::SmoothTransformation);
  }

  // 2. Crop normalized rectangle
  if (cropW > 0.01 && cropH > 0.01 && (cropX > 0.001 || cropY > 0.001 || cropW < 0.999 || cropH < 0.999)) {
    int cx = std::clamp((int)(cropX * img.width()), 0, img.width() - 1);
    int cy = std::clamp((int)(cropY * img.height()), 0, img.height() - 1);
    int cw = std::clamp((int)(cropW * img.width()), 1, img.width() - cx);
    int ch = std::clamp((int)(cropH * img.height()), 1, img.height() - cy);
    img = img.copy(cx, cy, cw, ch);
  }

  // 3. Color Science Filters & Tone Adjustments
  img = processFiltersAndAdjustments(img, filterName, filterIntensity, exposure, contrast, saturation, temperature);

  // 4. Vector Annotations
  if (!annotations.isEmpty()) {
    QSize canvasSize = img.size();
    renderAnnotationsOnImage(img, annotations, canvasSize);
  }

  // 5. Save Output
  QFileInfo outInfo(cleanDst);
  QDir().mkpath(outInfo.dir().absolutePath());

  bool saved = img.save(cleanDst, nullptr, quality);
  if (saved) {
    qDebug() << "[ImageProcessor] Successfully saved edited image:" << cleanDst << "(" << img.width() << "x" << img.height() << ")";
    clearImageCache();
  } else {
    qWarning() << "[ImageProcessor] Failed to save image to:" << cleanDst;
  }

  return saved;
}
