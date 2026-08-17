#include "ImageModel.h"
#include "FastVolumeScanner.h"
#include "TaskScheduler.h"
#include "VideoThumbnailer.h"
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QImageReader>
#include <QRegularExpression>
#include <QStandardPaths>
#include <algorithm>
#include <libraw/libraw.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "VisibleRangeManager.h"

ImageModel::ImageModel(QObject *parent) : QAbstractListModel(parent) {
  // Constructor should be lightweight. Scanning happens via scanDirectory().
  m_visibleStartIndex = -1;
  m_visibleEndIndex = -1;

  m_updateTimer = new QTimer(this);
  m_updateTimer->setInterval(16); // ~60fps
  connect(m_updateTimer, &QTimer::timeout, this,
          &ImageModel::processPendingUpdates);
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
  case IsRawRole: {
    QString ext = QFileInfo(info.filePath).suffix().toLower();
    bool isRaw = (ext == "arw" || ext == "cr2" || ext == "dng" ||
                  ext == "nef" || ext == "sr2" || ext == "srf" ||
                  ext == "orf" || ext == "rw2" || ext == "pef" || ext == "raf");
    return isRaw;
  }
  case DateTimeRole:
    return info.date;
  case IsBurstRole:
    return info.isBurst;
  case IsSelectedRole:
    return info.isSelected;

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
  roles[IsRawRole] = "isRaw";
  roles[DateTimeRole] = "dateTime";
  roles[IsBurstRole] = "isBurst";
  roles[IsSelectedRole] = "isSelected";
  return roles;
}

void ImageModel::setVisibleStartIndex(int index) {
  if (m_visibleStartIndex != index) {
    m_visibleStartIndex = index;
    updateVisiblePaths();
    emit visibleRangeChanged();
  }
}

void ImageModel::setVisibleEndIndex(int index) {
  if (m_visibleEndIndex != index) {
    m_visibleEndIndex = index;
    updateVisiblePaths();
    emit visibleRangeChanged();
  }
}

bool ImageModel::isPathVisible(const QString &path) {
  return VisibleRangeManager::instance().isPathVisible(path);
}

void ImageModel::updateVisiblePaths() {
  if (m_images.isEmpty() || m_visibleStartIndex < 0 || m_visibleEndIndex < 0) {
    return;
  }

  QSet<QString> visiblePaths;
  const int BUFFER_ZONE = 200; // 2-screen buffer zone
  int start = qMax(0, m_visibleStartIndex - BUFFER_ZONE);
  int end = qMin(m_visibleEndIndex + BUFFER_ZONE, (int)m_images.count() - 1);
  for (int i = start; i <= end; ++i) {
    visiblePaths.insert(m_images[i].filePath);
  }
  VisibleRangeManager::instance().setVisiblePaths(visiblePaths);
}

void ImageModel::scanDirectory(const QString &path) {
  if (m_isLoading) {
    return;
  }

  m_isLoading = true;
  emit isLoadingChanged();

  // Clear current images immediately so UI updates
  beginResetModel();
  m_images.clear();
  m_pendingInsertions.clear();
  endResetModel();

  // Clear previous pending tasks (e.g. thumbnails from old folder)
  TaskScheduler::instance().clear();

  // Run scanning via TaskScheduler
  TaskScheduler::instance().addTask(
      [this, path]() {
        QElapsedTimer timer;
        timer.start();

        QString cleanPath;
        QUrl url(path);
        if (url.isValid() && url.isLocalFile()) {
          cleanPath = url.toLocalFile();
        } else {
          // Fallback for raw paths (not proper URLs)
          cleanPath = path;
        }

        // Handle odd edge case: /C:/Users... which QUrl might return on some Qt
        // versions/platforms
        if (cleanPath.startsWith("/") && cleanPath.length() > 2 &&
            cleanPath[2] == ':') {
          cleanPath = cleanPath.mid(1);
        }

        cleanPath = QDir::toNativeSeparators(cleanPath);

        if (cleanPath.length() > 1 && cleanPath[1] == ':') {
          cleanPath[0] = cleanPath[0].toUpper();
        }

        if (!QDir(cleanPath).exists()) {
          qWarning() << "Directory does not exist:" << cleanPath;
          QMetaObject::invokeMethod(this, [this]() {
            m_isLoading = false;
            emit isLoadingChanged();
          });
          return;
        }

        qDebug() << "Scanning directory:" << cleanPath;

        QStringList extensions = {
            "jpg", "jpeg", "png",  "mp4",  "mkv",  "avi", "mov",  "arw", "cr2",
            "dng", "nef",  "webp", "heic", "tiff", "bmp", "gif",  "ico", "tga",
            "sr2", "srf",  "orf",  "rw2",  "pef",  "raf", "webm", "flv", "vob",
            "ogg", "ogv",  "mts",  "m2ts", "ts",   "3gp"};

        QStringList filters;
        for (const QString &ext : extensions) {
          filters << "*." + ext;
          filters << "*." + ext.toUpper();
        }

        QDirIterator it(cleanPath, filters, QDir::Files,
                        QDirIterator::Subdirectories);

        QList<ImageInfo> batch;
        const int BATCH_SIZE = 100;
        QRegularExpression dateRegex("(\\d{8})_(\\d{6})");

        // --- Fast MFT Scan Attempt ---
        bool fastScanSuccess = false;

        // Only attempt MFT scan if path is on a local NTFS drive (simplistic
        // check for now) cleanPath is e.g. "C:/Users/..."
        if (cleanPath.length() >= 3 && cleanPath[1] == ':' &&
            cleanPath[2] == '/') {
          FastVolumeScanner fastScanner;
          if (fastScanner.scanVolume(cleanPath)) {
            qDebug() << "FastScanner: Success! Filtering results...";
            QVector<ScannedFile> scannedFiles = fastScanner.getScannedFiles();
            QString searchPrefix = cleanPath;
            if (!searchPrefix.endsWith("/"))
              searchPrefix += "/";

            int itemsFound = 0;
            for (const ScannedFile &sf : scannedFiles) {
              // Check inclusion
              if (sf.path.startsWith(searchPrefix, Qt::CaseInsensitive)) {
                // Check Extension directly using standard substring for speed
                int dotIdx = sf.path.lastIndexOf('.');
                if (dotIdx > 0) {
                  QString ext = sf.path.mid(dotIdx + 1).toLower();
                  if (extensions.contains(ext)) {
                    ImageInfo info;
                    info.filePath = sf.path;
                    int slashIdx = sf.path.lastIndexOf('/');
                    info.fileName =
                        slashIdx >= 0 ? sf.path.mid(slashIdx + 1) : sf.path;
                    info.size = sf.size;

                    // Convert 100-nanosecond intervals since Jan 1, 1601 to
                    // QDateTime
                    if (sf.creationTime > 0) {
                      qint64 msecsSince1601 = sf.creationTime / 10000;
                      // 11644473600000 = ms between 1601 and 1970
                      qint64 msecsSinceEpoch =
                          msecsSince1601 - 11644473600000LL;
                      info.date =
                          QDateTime::fromMSecsSinceEpoch(msecsSinceEpoch);
                    } else {
                      info.date = QDateTime::currentDateTime();
                    }

                    batch.append(info);
                    itemsFound++;
                  }
                }
              }
            }
            qDebug() << "FastScanner: Filtered" << itemsFound
                     << "items with instantly loaded metadata.";
            fastScanSuccess = true;
          }
        }

        if (!fastScanSuccess) {
          // Fallback to QDirIterator
          QDirIterator it(cleanPath, filters, QDir::Files,
                          QDirIterator::Subdirectories);

          while (it.hasNext()) {
            it.next();
            QFileInfo fileInfo = it.fileInfo();
            ImageInfo info;
            info.filePath = fileInfo.absoluteFilePath();
            info.fileName = fileInfo.fileName();
            // Deferred QFileInfo disk IO for size and date
            // Fallback to fast date parsing from filename

            // Optimize: Try parsing filename for date
            QRegularExpressionMatch match = dateRegex.match(info.fileName);
            if (match.hasMatch()) {
              QString dateStr = match.captured(1) + match.captured(2);
              QDateTime dt = QDateTime::fromString(dateStr, "yyyyMMddHHmmss");
              if (dt.isValid())
                info.date = dt;
            }

            batch.append(info);
          }
        }

        // Final Sort and completion
        // Final Sort and completion
        // Do the sorting in the background thread!
        std::sort(batch.begin(), batch.end(),
                  [](const ImageInfo &a, const ImageInfo &b) {
                    return a.date > b.date;
                  });

        // Burst Detection: Group shots within 2 seconds
        if (batch.size() > 1) {
          const qint64 BURST_THRESHOLD_MS = 2000;
          for (int i = 0; i < batch.size(); ++i) {
            bool prevNear =
                (i > 0) && (std::abs(batch[i].date.msecsTo(batch[i - 1].date)) <
                            BURST_THRESHOLD_MS);
            bool nextNear =
                (i < batch.size() - 1) &&
                (std::abs(batch[i].date.msecsTo(batch[i + 1].date)) <
                 BURST_THRESHOLD_MS);
            batch[i].isBurst = (prevNear || nextNear);
          }
        }

        QMetaObject::invokeMethod(this, [this, batch, timer]() {
          m_allImages = batch;
          m_pendingInsertions = batch;
          if (!m_filterQuery.isEmpty()) {
            // If we already have a filter, apply it immediately.
            m_pendingInsertions.clear();
            applyFilter();
            m_isLoading = false;
            emit isLoadingChanged();
            qDebug()
                << "Scanning background work finished with filter applied.";
          } else {
            m_updateTimer->start();
            qDebug() << "Scanning background work finished in"
                     << timer.elapsed() << "ms. Starting batched insertion of"
                     << m_pendingInsertions.count() << "items.";
          }
        });
      },
      TaskScheduler::IO_BOUND, TaskScheduler::Normal);
}

void ImageModel::setFilterQuery(const QString &query) {
  if (m_filterQuery != query) {
    m_filterQuery = query;
    emit filterQueryChanged();
    applyFilter();
  }
}

void ImageModel::applyFilter() {
  m_updateTimer->stop();
  beginResetModel();
  m_images.clear();

  if (m_filterQuery.isEmpty()) {
    // We could do gradual insertion again, but for now just load all
    m_images = m_allImages;
  } else {
    QString q = m_filterQuery.toLower();
    for (const auto &item : m_allImages) {
      if (item.fileName.toLower().contains(q) ||
          item.filePath.toLower().contains(q)) {
        m_images.append(item);
      }
    }
  }
  endResetModel();

  m_pendingInsertions.clear();
  m_isLoading = false;
  emit isLoadingChanged();
}

QStringList ImageModel::getActiveDirectories() const {
  QSet<QString> dirs;
  for (const auto &item : m_images) {
    QString dirPath = QFileInfo(item.filePath).absolutePath();
    dirs.insert(QDir::fromNativeSeparators(dirPath).toLower());
  }
  return dirs.values();
}

void ImageModel::processPendingUpdates() {
  if (m_pendingInsertions.isEmpty()) {
    m_updateTimer->stop();
    m_isLoading = false;
    emit isLoadingChanged();
    return;
  }

  const int BATCH_SIZE = 100; // Insert 100 items per frame
  int count = qMin(BATCH_SIZE, (int)m_pendingInsertions.size());

  int startIdx = m_images.size();
  beginInsertRows(QModelIndex(), startIdx, startIdx + count - 1);
  for (int i = 0; i < count; ++i) {
    m_images.append(m_pendingInsertions.takeFirst());
  }
  endInsertRows();
}

bool ImageModel::cropImage(int index, const QRectF &cropRect) {
  if (index < 0 || index >= m_images.count())
    return false;

  ImageInfo info = m_images[index];
  QString filePath = info.filePath;

  QImage img(filePath);
  if (img.isNull())
    return false;

  QRect rect(cropRect.x() * img.width(), cropRect.y() * img.height(),
             cropRect.width() * img.width(), cropRect.height() * img.height());

  QImage cropped = img.copy(rect);
  return cropped.save(filePath);
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
  bool isRaw = (ext == "arw" || ext == "cr2" || ext == "dng" || ext == "nef" ||
                ext == "sr2" || ext == "srf" || ext == "orf" || ext == "rw2" ||
                ext == "pef" || ext == "raf");
  bool isVideo = (ext == "mp4" || ext == "mkv" || ext == "avi" ||
                  ext == "mov" || ext == "webm");

  if (isVideo) {
    VideoThumbnailer v;
    QVariantMap vmeta = v.getMetadata(info.filePath);
    for (auto it = vmeta.begin(); it != vmeta.end(); ++it) {
      meta[it.key()] = it.value();
    }
    meta["Type"] = "Video";
  } else if (isRaw) {
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
      meta["Shutter"] =
          QString("1/%1").arg(1.0 / RawProcessor.imgdata.other.shutter);
      meta["Aperture"] =
          QString("f/%1").arg(RawProcessor.imgdata.other.aperture);
      meta["Type"] = "RAW Image";
      RawProcessor.recycle();
    }
  } else {
    // Standard Image - Use QImageReader for resolution
    QImageReader reader(info.filePath);
    if (reader.canRead()) {
      QSize size = reader.size();
      meta["Resolution"] =
          QString("%1x%2").arg(size.width()).arg(size.height());
    }
    meta["Type"] = "Image";
  }
  return meta;
}
bool ImageModel::setData(const QModelIndex &index, const QVariant &value,
                         int role) {
  if (!index.isValid() || index.row() >= m_images.count())
    return false;

  if (role == IsSelectedRole) {
    m_images[index.row()].isSelected = value.toBool();
    emit dataChanged(index, index, {role});
    emit selectedCountChanged();
    return true;
  }
  return false;
}

void ImageModel::clearSelection() {
  for (int i = 0; i < m_images.count(); ++i) {
    m_images[i].isSelected = false;
  }
  if (!m_images.isEmpty()) {
    emit dataChanged(createIndex(0, 0), createIndex(m_images.count() - 1, 0),
                     {IsSelectedRole});
    emit selectedCountChanged();
  }
}

int ImageModel::selectedCount() const {
  int count = 0;
  for (const auto &item : m_images) {
    if (item.isSelected)
      count++;
  }
  return count;
}

QStringList ImageModel::getSelectedPaths() const {
  QStringList paths;
  for (const auto &item : m_images) {
    if (item.isSelected) {
      paths.append(item.filePath);
    }
  }
  return paths;
}

int ImageModel::getProxyIndexForSourceIndex(int sourceIndex) const {
  return sourceIndex;
}
