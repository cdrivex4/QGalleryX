#include "ImageModel.h"
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QImageReader>
#include <QStandardPaths>
#include <QtConcurrent>
#include <algorithm>
#include <libraw/libraw.h>

ImageModel::ImageModel(QObject *parent) : QAbstractListModel(parent) {
  // Hardcoded default path as requested
  QString defaultPath =
      QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);

  // Directly scan. Validation happens in the worker thread.
  scanDirectory(defaultPath);
}

int ImageModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return m_images.count();
}

QVariant ImageModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= m_images.count())
    return QVariant();

  const ImageInfo &info = m_images[index.row()];

  switch (role) {
  case FilePathRole:
    return info.filePath;
  case FileNameRole:
    return info.fileName;
  case DateSectionRole: // Legacy support
  case SectionDayRole: {
    QDate date = info.date.date();
    QDate today = QDate::currentDate();
    if (date == today)
      return "Today";
    if (date == today.addDays(-1))
      return "Yesterday";
    if (date.year() == today.year())
      return date.toString("MMM d");
    return date.toString("MMM d, yyyy");
  }
  case SectionMonthRole: {
    return info.date.date().toString("MMMM yyyy");
  }
  case SectionYearRole: {
    return info.date.date().toString("yyyy");
  }
  case SectionWeekRole: {
    int year = info.date.date().year();
    int week = info.date.date().weekNumber();
    return QString("%1 - Week %2").arg(year).arg(week);
  }
  case ExifRole: {
    QVariantMap exif;
    QImageReader reader(info.filePath);
    QSize size = reader.size();
    if (size.isValid()) {
      exif["resolution"] =
          QString("%1x%2").arg(size.width()).arg(size.height());
      exif["size"] =
          QString("%1 KB").arg(QFileInfo(info.filePath).size() / 1024);
      exif["camera"] = "Unknown"; // Placeholder
    }
    return exif;
  }
  default:
    return QVariant();
  }
}

QHash<int, QByteArray> ImageModel::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[FilePathRole] = "filePath";
  roles[FileNameRole] = "fileName";
  roles[DateSectionRole] = "dateSection";
  roles[SectionDayRole] = "sectionDay";
  roles[SectionMonthRole] = "sectionMonth";
  roles[SectionYearRole] = "sectionYear";
  roles[SectionWeekRole] = "sectionWeek";
  roles[ExifRole] = "exif";
  return roles;
}

void ImageModel::scanDirectory(const QString &path) {
  // Clear current images immediately so UI updates
  beginResetModel();
  m_images.clear();
  endResetModel();

  // Run scanning in a background thread
  QFuture<QList<ImageInfo>> future = QtConcurrent::run([path]() {
    QList<ImageInfo> images;
    QString cleanPath = path;

    // Handle file:/// prefix
    if (cleanPath.startsWith("file:///")) {
      cleanPath = cleanPath.mid(8);
    } else if (cleanPath.startsWith("file:")) {
      cleanPath = cleanPath.mid(5);
    }

    // Handle Windows paths (e.g., /C:/Users...)
    if (cleanPath.startsWith("/") && cleanPath.contains(":")) {
      cleanPath = cleanPath.mid(1);
    }

    // Ensure drive letter is uppercase for consistency
    if (cleanPath.length() > 1 && cleanPath[1] == ':') {
      cleanPath[0] = cleanPath[0].toUpper();
    }

    if (!QDir(cleanPath).exists()) {
      qWarning() << "Directory does not exist:" << cleanPath;
      return images;
    }

    qDebug() << "Scanning directory:" << cleanPath;

    // Recursive scan
    QDirIterator it(cleanPath,
                    QStringList() << "*.jpg" << "*.jpeg" << "*.png" << "*.mp4"
                                  << "*.mkv" << "*.avi" << "*.mov"
                                  << "*.arw" << "*.cr2" << "*.dng" << "*.nef"
                                  << "*.webp" << "*.heic" << "*.tiff",
                    QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext()) {
      it.next();
      ImageInfo info;
      info.filePath = it.filePath();
      info.fileName = it.fileName();
      info.date = it.fileInfo().birthTime(); // Use creation time
      images.append(info);
    }

    // Sort by date (newest first)
    std::sort(
        images.begin(), images.end(),
        [](const ImageInfo &a, const ImageInfo &b) { return a.date > b.date; });

    return images;
  });

  auto *watcher = new QFutureWatcher<QList<ImageInfo>>(this);
  connect(watcher, &QFutureWatcher<QList<ImageInfo>>::finished, this,
          [this, watcher]() {
            beginResetModel();
            m_images = watcher->result();
            endResetModel();
            watcher->deleteLater();
            qDebug() << "Scanned" << m_images.count() << "items.";
          });

  watcher->setFuture(future);
}

bool ImageModel::cropImage(int index, const QRectF &cropRect) {
  if (index < 0 || index >= m_images.count())
    return false;

  ImageInfo info = m_images[index];
  QString filePath = info.filePath;

  QImage img(filePath);
  if (img.isNull())
    return false;

  // Convert relative rect (0.0-1.0) to pixels
  QRect rect(cropRect.x() * img.width(), cropRect.y() * img.height(),
             cropRect.width() * img.width(), cropRect.height() * img.height());

  QImage cropped = img.copy(rect);
  return cropped.save(filePath); // Overwrite original for now (simple edit)
}

QVariantMap ImageModel::getMetadata(int index) {
  if (index < 0 || index >= m_images.count())
    return {};

  const ImageInfo &info = m_images[index];
  QVariantMap meta;
  meta["Filename"] = info.fileName;
  meta["Path"] = info.filePath;
  meta["Date"] = info.date.toString("yyyy-MM-dd HH:mm:ss");
  meta["Size"] = QString("%1 KB").arg(QFileInfo(info.filePath).size() / 1024);

  QString ext = QFileInfo(info.filePath).suffix().toLower();
  bool isRaw = (ext == "arw" || ext == "cr2" || ext == "dng" || ext == "nef");

  if (isRaw) {
    LibRaw RawProcessor;
    if (RawProcessor.open_file(info.filePath.toLocal8Bit().constData()) ==
        LIBRAW_SUCCESS) {
      meta["Resolution"] = QString("%1x%2")
                               .arg(RawProcessor.imgdata.sizes.width)
                               .arg(RawProcessor.imgdata.sizes.height);
      meta["Camera"] = QString("%1 %2")
                           .arg(RawProcessor.imgdata.idata.make)
                           .arg(RawProcessor.imgdata.idata.model);
      meta["ISO"] = QString::number(RawProcessor.imgdata.other.iso_speed);
      meta["Shutter"] = QString::number(RawProcessor.imgdata.other.shutter);
      meta["Aperture"] = QString::number(RawProcessor.imgdata.other.aperture);
      RawProcessor.recycle();
    }
  } else {
    QImageReader reader(info.filePath);
    if (reader.canRead()) {
      QSize size = reader.size();
      meta["Resolution"] =
          QString("%1x%2").arg(size.width()).arg(size.height());
    }
  }
  return meta;
}
