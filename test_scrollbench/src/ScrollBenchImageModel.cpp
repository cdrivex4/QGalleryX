#include "ScrollBenchImageModel.h"
#include "../../src/VisibleRangeManager.h"
#include "../src/FrameBudgetScheduler.h"
#include "../src/TaskScheduler.h"
#include <QColor>
#include <QDebug>
#include <QDirIterator>
#include <QFileInfo>
#include <QImageReader>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QTimer>
#include <libraw/libraw.h>

ScrollBenchImageModel::ScrollBenchImageModel(QObject *parent)
    : QAbstractListModel(parent) {
  m_updateTimer = new QTimer(this);
  m_updateTimer->setInterval(16); // ~60fps
  m_updateTimer->setSingleShot(true);
  connect(m_updateTimer, &QTimer::timeout, this,
          &ScrollBenchImageModel::processPendingUpdates);

  m_forceUpdateTimer = new QTimer(this); // Initialize the new timer
  m_forceUpdateTimer->setSingleShot(true);
  connect(m_forceUpdateTimer, &QTimer::timeout, this,
          &ScrollBenchImageModel::forceUpdateGridView); // Connect to new signal
}

void ScrollBenchImageModel::forceDelayedUpdate() { emit forceUpdateGridView(); }

void ScrollBenchImageModel::setFrameScheduler(FrameBudgetScheduler *scheduler) {
  m_frameScheduler = scheduler;
}

int ScrollBenchImageModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return m_items.count();
}

QVariant ScrollBenchImageModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= m_items.count())
    return QVariant();

  const ImageItem &item = m_items[index.row()];

  switch (role) {
  case FilePathRole:
    return item.path;
  case FileNameRole:
    return item.fileName;
  case ImageIndexRole:
    return index.row();
  case IsLoadedRole:
    return item.isLoaded;
  case ColorRole:
    return item.color;
  case IsSelectedRole:
    return item.isSelected;
  case IsBurstRole:
    return item.isBurst;
  case SectionDayRole: {
    QDate date = item.date.date();
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
    return item.date.date().toString("MMMM yyyy");
  }
  case SectionYearRole: {
    return item.date.date().toString("yyyy");
  }
  case SectionWeekRole: {
    int year = item.date.date().year();
    int week = item.date.date().weekNumber();
    return QString("%1 - Week %2").arg(year).arg(week);
  }
  case IsRawRole: {
    QString ext = QFileInfo(item.path).suffix().toLower();
    return (ext == "arw" || ext == "cr2" || ext == "dng" || ext == "nef" ||
            ext == "sr2" || ext == "srf" || ext == "orf" || ext == "rw2" ||
            ext == "pef" || ext == "raf");
  }
  default:
    return QVariant();
  }
}

QHash<int, QByteArray> ScrollBenchImageModel::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[FilePathRole] = "filePath";
  roles[FileNameRole] = "fileName";
  roles[ImageIndexRole] = "imageIndex";
  roles[IsLoadedRole] = "isLoaded";
  roles[ColorRole] = "testColor";
  roles[IsSelectedRole] = "isSelected";
  roles[IsBurstRole] = "isBurst";
  roles[SectionDayRole] = "sectionDay";
  roles[SectionMonthRole] = "sectionMonth";
  roles[SectionYearRole] = "sectionYear";
  roles[SectionWeekRole] = "sectionWeek";
  roles[IsRawRole] = "isRaw";
  return roles;
}

void ScrollBenchImageModel::setVisibleStartIndex(int index) {
  if (m_visibleStartIndex != index) {
    int delta = std::abs(m_visibleStartIndex - index);
    m_visibleStartIndex = index;
    emit visibleRangeChanged();

    if (m_viewportCullingEnabled) {
      if (delta > 10) {
        cancelPendingRequests();
      }
      updateVisibleRange();
    }
  }
}

void ScrollBenchImageModel::setVisibleEndIndex(int index) {
  qDebug() << "setVisibleEndIndex called with:" << index;
  if (m_visibleEndIndex != index) {
    m_visibleEndIndex = index;
    emit visibleRangeChanged();

    if (m_viewportCullingEnabled) {
      updateVisibleRange();
    }
  }
}

void ScrollBenchImageModel::setViewportCullingEnabled(bool enabled) {
  if (m_viewportCullingEnabled != enabled) {
    m_viewportCullingEnabled = enabled;
    emit viewportCullingEnabledChanged();

    if (enabled) {
      cancelPendingRequests();
      updateVisibleRange();
    } else {
      for (int i = 0; i < m_items.count(); ++i) {
        requestThumbnail(i);
      }
    }
  }
}

void ScrollBenchImageModel::generateTestData(int count) {
  beginResetModel();
  m_items.clear();
  m_items.reserve(count);

  QStringList colors = {"#FF6B6B", "#4ECDC4", "#45B7D1", "#FFA07A", "#98D8C8",
                        "#F7DC6F", "#BB8FCE", "#85C1E2", "#F8B739", "#52B788"};

  QDateTime now = QDateTime::currentDateTime();
  for (int i = 0; i < count; ++i) {
    ImageItem item;
    item.fileName = QString("test_image_%1.jpg").arg(i, 5, 10, QChar('0'));
    item.path = QString("synthetic://test/%1").arg(item.fileName);
    item.color = colors[i % colors.size()];
    item.isLoaded = false;

    // Simulate bursts: only the first 5 items
    if (i < 5) {
      item.date = now.addMSecs(i * 100);
      item.isBurst = true;
    } else {
      item.date = now.addDays(-i / 5);
      item.isBurst = false;
    }
    m_items.append(item);
  }

  m_totalItems = count;
  m_remainingItems = count;
  emit remainingItemsChanged();
  endResetModel();

  qDebug() << "Generated" << count << "test items";

  if (m_viewportCullingEnabled && m_visibleEndIndex > 0) {
    updateVisibleRange();
  }
}

void ScrollBenchImageModel::clearData() {
  beginResetModel();
  m_items.clear();
  m_totalItems = 0;
  endResetModel();
}

void ScrollBenchImageModel::scanDirectory(const QString &path) {
  if (m_isLoading)
    return;

  m_isLoading = true;
  emit isLoadingChanged();

  beginResetModel();
  m_items.clear();
  m_totalItems = 0;
  m_remainingItems = 0;
  emit remainingItemsChanged();
  m_visibleStartIndex = 0;
  m_visibleEndIndex = 0;
  endResetModel();

  cancelPendingRequests();

  TaskScheduler::instance().addTask(
      [this, path]() {
        QElapsedTimer timer;
        timer.start();

        // Use QUrl to robustly parse file:// URIs and UNC paths
        QString cleanPath;
        QUrl url(path);
        if (url.isValid() && url.isLocalFile()) {
          cleanPath = url.toLocalFile();
        } else {
          cleanPath = path;
          if (cleanPath.startsWith("file:///"))
            cleanPath = cleanPath.mid(8);
          else if (cleanPath.startsWith("file://"))
            cleanPath = cleanPath.mid(7);
        }

        cleanPath = QDir::toNativeSeparators(cleanPath);

        if (!QDir(cleanPath).exists()) {
          qWarning() << "Directory does not exist:" << cleanPath;
          QMetaObject::invokeMethod(this, [this]() {
            m_isLoading = false;
            emit isLoadingChanged();
          });
          return;
        }

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

        QDirIterator it(cleanPath, filters, QDir::Files | QDir::Readable,
                        QDirIterator::Subdirectories);

        QVector<ImageItem> batch;
        batch.reserve(100);
        int totalFound = 0;
        QRegularExpression dateRegex("(\\d{8})_(\\d{6})");

        while (it.hasNext() && !m_scanCancelled) {
          QString filePath = it.next();
          QFileInfo fileInfo(filePath);

          ImageItem item;
          item.fileName = fileInfo.fileName();
          item.path = fileInfo.absoluteFilePath();
          item.color = "#444444";
          item.isLoaded = false;
          item.date = fileInfo.birthTime();

          QRegularExpressionMatch match = dateRegex.match(item.fileName);
          if (match.hasMatch()) {
            QString dateStr = match.captured(1) + match.captured(2);
            QDateTime dt = QDateTime::fromString(dateStr, "yyyyMMddHHmmss");
            if (dt.isValid())
              item.date = dt;
          }

          batch.append(item);
          totalFound++;
        }

        if (!batch.isEmpty()) {
          QMetaObject::invokeMethod(this, [this, batch, timer, totalFound,
                                           cleanPath]() {
            beginResetModel();
            m_items = batch;
            std::sort(m_items.begin(), m_items.end(),
                      [](const ImageItem &a, const ImageItem &b) {
                        return a.date > b.date;
                      });

            // Burst Detection
            if (m_items.size() > 1) {
              const qint64 BURST_THRESHOLD_MS = 2000;
              for (int i = 0; i < m_items.size(); ++i) {
                bool prevNear =
                    (i > 0) &&
                    (std::abs(m_items[i].date.msecsTo(m_items[i - 1].date)) <
                     BURST_THRESHOLD_MS);
                bool nextNear =
                    (i < m_items.size() - 1) &&
                    (std::abs(m_items[i].date.msecsTo(m_items[i + 1].date)) <
                     BURST_THRESHOLD_MS);
                m_items[i].isBurst = (prevNear || nextNear);
              }
            }

            endResetModel();

            m_isLoading = false;
            m_totalItems = m_items.count();
            m_remainingItems = m_totalItems;
            emit remainingItemsChanged();
            emit isLoadingChanged();
            emit scanComplete(totalFound);
            qDebug() << "Async Scan finished:" << totalFound << "items in"
                     << timer.elapsed() << "ms from" << cleanPath;
            m_forceUpdateTimer->start(200);
          });
        }
      },
      TaskScheduler::IO_BOUND, TaskScheduler::Normal);
}

void ScrollBenchImageModel::updateVisibleRange() {
  if (!m_viewportCullingEnabled) {
    return;
  }

  int startIdx = qMax(0, m_visibleStartIndex - BUFFER_SIZE);
  int endIdx = qMin(m_items.count() - 1, m_visibleEndIndex + BUFFER_SIZE);

  qDebug() << "Visible range updated in C++:" << startIdx << "to" << endIdx
           << "(viewport:" << m_visibleStartIndex << "-" << m_visibleEndIndex
           << ", total items:" << m_items.count() << ")";

  QSet<QString> visiblePaths;
  for (int i = startIdx; i <= endIdx; ++i) {
    if (i >= 0 && i < m_items.count()) {
      visiblePaths.insert(m_items[i].path);
    }
  }
  qDebug() << "[Model] Setting visible paths:" << visiblePaths.size();
  VisibleRangeManager::instance().setVisiblePaths(visiblePaths);

  int requestedCount = 0;
  for (int i = startIdx; i <= endIdx; ++i) {
    if (i >= 0 && i < m_items.count() && !m_items[i].isLoaded) {
      requestThumbnail(i);
      requestedCount++;
    }
  }
  qDebug() << "requestThumbnail calls initiated:" << requestedCount
           << "for visible range";
}

void ScrollBenchImageModel::requestThumbnail(int index) {
  if (index < 0 || index >= m_items.count() || m_items[index].isLoaded) {
    return;
  }
  // Avoid duplicate requests if already pending
  if (m_activelyRequesting.contains(index))
    return;
  m_activelyRequesting.insert(index);

  auto deliverTask = [this, index]() {
    m_activelyRequesting.remove(index);
    if (index >= 0 && index < m_items.count()) {
      if (!m_items[index].isLoaded) {
        m_items[index].isLoaded = true;
        m_remainingItems = qMax(0, m_remainingItems - 1);
        emit remainingItemsChanged();
      }

      m_pendingDecodes = qMax(0, m_pendingDecodes - 1);
      emit pendingDecodeCountChanged();

      m_pendingLoadedIndices.insert(index);
      if (!m_updateTimer->isActive()) {
        m_updateTimer->start();
      }
    }
  };

  m_pendingDecodes++;
  emit pendingDecodeCountChanged();

  if (m_frameScheduler) {
    m_frameScheduler->onTaskCompleted(deliverTask);
  } else {
    QMetaObject::invokeMethod(this, deliverTask, Qt::QueuedConnection);
  }
}

void ScrollBenchImageModel::processPendingUpdates() {
  if (m_pendingLoadedIndices.isEmpty())
    return;

  QList<int> sortedIndices = m_pendingLoadedIndices.values();
  std::sort(sortedIndices.begin(), sortedIndices.end());
  m_pendingLoadedIndices.clear();

  if (sortedIndices.isEmpty())
    return;

  int startRange = sortedIndices.first();
  int endRange = startRange;

  for (int i = 1; i < sortedIndices.count(); ++i) {
    if (sortedIndices[i] == endRange + 1) {
      endRange = sortedIndices[i];
    } else {
      emit dataChanged(createIndex(startRange, 0), createIndex(endRange, 0),
                       {IsLoadedRole});
      startRange = sortedIndices[i];
      endRange = startRange;
    }
  }
  emit dataChanged(createIndex(startRange, 0), createIndex(endRange, 0),
                   {IsLoadedRole});
  qDebug() << "processPendingUpdates: Emitted dataChanged for range:"
           << startRange << "to" << endRange;
}

void ScrollBenchImageModel::cancelPendingRequests() {
  m_pendingDecodes = 0;
  emit pendingDecodeCountChanged();
  m_pendingLoadedIndices.clear();
  m_activelyRequesting.clear();

  // Note: We no longer clear the TaskScheduler. With LIFO logic,
  // new requests automatically move to the front, and preserving
  // existing tasks ensures AsyncImageProvider responses aren't lost.

  if (m_updateTimer && m_updateTimer->isActive()) {
    m_updateTimer->stop();
  }
}

void ScrollBenchImageModel::cancelScan() { m_scanCancelled = true; }

// Selection methods
void ScrollBenchImageModel::toggleSelection(int index) {
  if (index < 0 || index >= m_items.count())
    return;

  m_items[index].isSelected = !m_items[index].isSelected;

  QModelIndex modelIndex = createIndex(index, 0);
  emit dataChanged(modelIndex, modelIndex, {IsSelectedRole});
  emit selectedCountChanged();
}

void ScrollBenchImageModel::selectRange(int start, int end) {
  if (start < 0 || end < 0 || start >= m_items.count() ||
      end >= m_items.count()) {
    return;
  }

  int realStart = qMin(start, end);
  int realEnd = qMax(start, end);

  bool changed = false;
  for (int i = realStart; i <= realEnd; ++i) {
    if (!m_items[i].isSelected) {
      m_items[i].isSelected = true;
      changed = true;
    }
  }

  if (changed) {
    emit dataChanged(createIndex(realStart, 0), createIndex(realEnd, 0),
                     {IsSelectedRole});
    emit selectedCountChanged();
  }
}

void ScrollBenchImageModel::selectVisualRect(int colMin, int colMax, int rowMin,
                                             int rowMax, int columns) {
  if (columns <= 0)
    return;

  bool changed = false;
  int firstAffected = -1;
  int lastAffected = -1;

  for (int r = rowMin; r <= rowMax; ++r) {
    for (int c = colMin; c <= colMax; ++c) {
      int index = r * columns + c;
      if (index >= 0 && index < m_items.count()) {
        if (!m_items[index].isSelected) {
          m_items[index].isSelected = true;
          changed = true;
          if (firstAffected == -1)
            firstAffected = index;
          lastAffected = index;
        }
      }
    }
  }

  if (changed) {
    // We emit a range covering the min/max index touched, which might include
    // untouched items in between, but that's safe for a simple redraw hint.
    emit dataChanged(createIndex(firstAffected, 0),
                     createIndex(lastAffected, 0), {IsSelectedRole});
    emit selectedCountChanged();
  }
}

void ScrollBenchImageModel::selectAll() {
  for (int i = 0; i < m_items.count(); ++i) {
    m_items[i].isSelected = true;
  }

  if (!m_items.isEmpty()) {
    emit dataChanged(createIndex(0, 0), createIndex(m_items.count() - 1, 0),
                     {IsSelectedRole});
    emit selectedCountChanged();
  }
}

void ScrollBenchImageModel::clearSelection() {
  for (int i = 0; i < m_items.count(); ++i) {
    m_items[i].isSelected = false;
  }

  if (!m_items.isEmpty()) {
    emit dataChanged(createIndex(0, 0), createIndex(m_items.count() - 1, 0),
                     {IsSelectedRole});
    emit selectedCountChanged();
  }
}

void ScrollBenchImageModel::invertSelection() {
  for (int i = 0; i < m_items.count(); ++i) {
    m_items[i].isSelected = !m_items[i].isSelected;
  }

  if (!m_items.isEmpty()) {
    emit dataChanged(createIndex(0, 0), createIndex(m_items.count() - 1, 0),
                     {IsSelectedRole});
    emit selectedCountChanged();
  }
}

void ScrollBenchImageModel::deleteSelected() {
  // Remove items in reverse order to maintain indices
  for (int i = m_items.count() - 1; i >= 0; --i) {
    if (m_items[i].isSelected) {
      beginRemoveRows(QModelIndex(), i, i);
      m_items.removeAt(i);
      endRemoveRows();
    }
  }

  m_totalItems = m_items.count();
  emit selectedCountChanged();
}

int ScrollBenchImageModel::selectedCount() const {
  int count = 0;
  for (const auto &item : m_items) {
    if (item.isSelected)
      ++count;
  }
  return count;
}

bool ScrollBenchImageModel::cropImage(int index, const QRectF &cropRect) {
  if (index < 0 || index >= m_items.count())
    return false;

  QString filePath = m_items[index].path;
  QImage img(filePath);
  if (img.isNull())
    return false;

  QRect rect(cropRect.x() * img.width(), cropRect.y() * img.height(),
             cropRect.width() * img.width(), cropRect.height() * img.height());

  QImage cropped = img.copy(rect);
  return cropped.save(filePath);
}

QVariantMap ScrollBenchImageModel::getMetadata(int index) {
  if (index < 0 || index >= m_items.count())
    return {};

  const ImageItem &item = m_items[index];
  QVariantMap meta;
  meta["Filename"] = item.fileName;
  meta["Path"] = item.path;
  meta["Date"] = item.date.toString("yyyy-MM-dd HH:mm:ss");
  meta["Size"] = QString("%1 KB").arg(QFileInfo(item.path).size() / 1024);

  QString ext = QFileInfo(item.path).suffix().toLower();
  bool isRaw = (ext == "arw" || ext == "cr2" || ext == "dng" || ext == "nef" ||
                ext == "sr2" || ext == "srf" || ext == "orf" || ext == "rw2" ||
                ext == "pef" || ext == "raf");

  if (isRaw) {
    LibRaw RawProcessor;
    if (RawProcessor.open_file(item.path.toLocal8Bit().constData()) ==
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
    QImageReader reader(item.path);
    if (reader.canRead()) {
      QSize size = reader.size();
      meta["Resolution"] =
          QString("%1x%2").arg(size.width()).arg(size.height());
    }
  }
  return meta;
}
