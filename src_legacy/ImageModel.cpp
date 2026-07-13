#include "ImageModel.h"
#include "TaskScheduler.h"
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QImageReader>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QStorageInfo>
#include <algorithm>
#include <libraw/libraw.h>

ImageModel::ImageModel(QObject *parent) : QAbstractListModel(parent) {
  // Constructor should be lightweight. Scanning happens via scanDirectory().
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
    // qInfo() << "Checking RAW for" << info.filePath << ": " << isRaw; //
    // Debugging
    return isRaw;
  }
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
  roles[IsSelectedRole] = "isSelected";
  return roles;
}

void ImageModel::scanDirectory(const QString &path) {
  if (m_isLoading) {
    return;
  }

  m_isLoading = true;
  emit isLoadingChanged();

  // Clear current images immediately so UI updates
  beginResetModel();
  m_allItems.clear();
  m_images.clear();
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

        bool isNetworkPath = false;
        if (cleanPath.startsWith("\\\\")) {
          isNetworkPath = true;
          qDebug() << "[NetworkScan] Detected UNC path:" << cleanPath;
        } else if (cleanPath.length() >= 3 && cleanPath[1] == ':') {
          QStorageInfo storage(cleanPath);
          if (storage.isValid() && storage.device().startsWith("\\\\")) {
            isNetworkPath = true;
            qDebug() << "[NetworkScan] Detected mapped network drive:" << cleanPath;
          }
        }

        QDirIterator it(cleanPath,
                        QStringList()
                            << "*.jpg" << "*.jpeg" << "*.png" << "*.mp4"
                            << "*.mkv" << "*.avi" << "*.mov"
                            << "*.arw" << "*.cr2" << "*.dng" << "*.nef"
                            << "*.webp" << "*.heic"
                            << "*.tiff"
                            << "*.bmp" << "*.gif" << "*.ico" << "*.tga"
                            << "*.sr2" << "*.srf" << "*.orf" << "*.rw2"
                            << "*.pef" << "*.raf"
                            << "*.webm" << "*.flv" << "*.vob" << "*.ogg"
                            << "*.ogv"
                            << "*.mts" << "*.m2ts" << "*.ts" << "*.3gp",
                        QDir::Files, QDirIterator::Subdirectories);

        QList<ImageInfo> batch;
        const int BATCH_SIZE = 100;
        QRegularExpression dateRegex("(\\d{8})_(\\d{6})");

        while (it.hasNext()) {
          it.next();
          QFileInfo fileInfo = it.fileInfo();
          ImageInfo info;
          info.filePath = fileInfo.absoluteFilePath();
          info.fileName = fileInfo.fileName();
          info.size = fileInfo.size();
          info.date = fileInfo.birthTime(); // Default

          // Optimize: Try parsing filename for date
          QRegularExpressionMatch match = dateRegex.match(info.fileName);
          if (match.hasMatch()) {
            QString dateStr = match.captured(1) + match.captured(2);
            QDateTime dt = QDateTime::fromString(dateStr, "yyyyMMddHHmmss");
            if (dt.isValid())
              info.date = dt;
          }

          batch.append(info);

          if (batch.size() >= BATCH_SIZE) {
            QMetaObject::invokeMethod(this, [this, batch, isNetworkPath]() {
              m_allItems.append(batch);
              if (!isNetworkPath) {
                if (m_filterQuery.isEmpty()) {
                  beginInsertRows(QModelIndex(), m_images.count(),
                                  m_images.count() + batch.count() - 1);
                  m_images.append(batch);
                  endInsertRows();
                } else {
                  applyFilter();
                }
              }
            });
            batch.clear();
          }
        }

        // Append remaining
        if (!batch.isEmpty()) {
          QMetaObject::invokeMethod(this, [this, batch]() {
            m_allItems.append(batch);
            if (m_filterQuery.isEmpty()) {
              beginInsertRows(QModelIndex(), m_images.count(),
                              m_images.count() + batch.count() - 1);
              m_images.append(batch);
              endInsertRows();
            } else {
              applyFilter();
            }
          });
        }

        // Final Sort and completion
        QMetaObject::invokeMethod(this, [this, timer]() {
          std::sort(m_allItems.begin(), m_allItems.end(),
                    [](const ImageInfo &a, const ImageInfo &b) {
                      return a.date > b.date;
                    });
          applyFilter();

          m_isLoading = false;
          emit isLoadingChanged();
          qDebug() << "Scanning finished in" << timer.elapsed() << "ms. Found"
                   << m_images.count() << "items.";
        });
      },
      TaskScheduler::IO_BOUND, TaskScheduler::Normal);
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

void ImageModel::setFilterQuery(const QString &query) {
  if (m_filterQuery != query) {
    m_filterQuery = query;
    emit filterQueryChanged();
    applyFilter();
  }
}

void ImageModel::applyFilter() {
  beginResetModel();
  if (m_filterQuery.isEmpty()) {
    m_images = m_allItems;
  } else {
    m_images.clear();
    QString lowerQuery = m_filterQuery.toLower();
    for (const auto &item : m_allItems) {
      if (item.fileName.toLower().contains(lowerQuery)) {
        m_images.append(item);
      }
    }
  }
  endResetModel();
}

void ImageModel::clearSelection() {
  for (int i = 0; i < m_images.count(); ++i) {
    m_images[i].isSelected = false;
  }
  if (!m_images.isEmpty()) {
    emit dataChanged(createIndex(0, 0), createIndex(m_images.count() - 1, 0), {IsSelectedRole});
    emit selectedCountChanged();
  }
}

void ImageModel::selectAll() {
  for (int i = 0; i < m_images.count(); ++i) {
    m_images[i].isSelected = true;
  }
  if (!m_images.isEmpty()) {
    emit dataChanged(createIndex(0, 0), createIndex(m_images.count() - 1, 0), {IsSelectedRole});
    emit selectedCountChanged();
  }
}

void ImageModel::invertSelection() {
  for (int i = 0; i < m_images.count(); ++i) {
    m_images[i].isSelected = !m_images[i].isSelected;
  }
  if (!m_images.isEmpty()) {
    emit dataChanged(createIndex(0, 0), createIndex(m_images.count() - 1, 0), {IsSelectedRole});
    emit selectedCountChanged();
  }
}

void ImageModel::selectItems(const QList<int> &indices) {
  int minIndex = -1;
  int maxIndex = -1;
  bool changed = false;
  
  for (int index : indices) {
    if (index >= 0 && index < m_images.count() && !m_images[index].isSelected) {
      m_images[index].isSelected = true;
      changed = true;
      if (minIndex == -1 || index < minIndex) minIndex = index;
      if (maxIndex == -1 || index > maxIndex) maxIndex = index;
    }
  }

  if (changed) {
    emit dataChanged(createIndex(minIndex, 0), createIndex(maxIndex, 0), {IsSelectedRole});
    emit selectedCountChanged();
  }
}

void ImageModel::deleteSelected() {
  for (int i = m_images.count() - 1; i >= 0; --i) {
    if (m_images[i].isSelected) {
      beginRemoveRows(QModelIndex(), i, i);
      m_images.removeAt(i);
      endRemoveRows();
    }
  }
  emit selectedCountChanged();
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

qint64 ImageModel::getSelectedTotalSizeBytes() const {
  qint64 totalBytes = 0;
  for (const auto &item : m_images) {
    if (item.isSelected) {
      QFileInfo fi(item.filePath);
      totalBytes += fi.size();
    }
  }
  return totalBytes;
}

QStringList ImageModel::getActiveDirectories() const {
  QSet<QString> dirs;
  for (const auto &item : m_images) {
    QString dirPath = QFileInfo(item.filePath).absolutePath();
    dirs.insert(QDir::fromNativeSeparators(dirPath).toLower());
  }
  return dirs.values();
}

bool ImageModel::setData(const QModelIndex &index, const QVariant &value, int role) {
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

int ImageModel::selectedCount() const {
  int count = 0;
  for (const auto &item : m_images) {
    if (item.isSelected) count++;
  }
  return count;
}
