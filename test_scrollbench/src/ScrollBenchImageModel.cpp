#include "ScrollBenchImageModel.h"
#include "../../src/AsyncImageProvider.h"
#include "../../src/DesktopHelper.h"
#include "../../src/ImageProcessor.h"
#include "../../src/VisibleRangeManager.h"
#include "../src/FastVolumeScanner.h"
#include "../src/FrameBudgetScheduler.h"
#include "../src/TaskScheduler.h"
#include "FileTypeRouter.h"
#include <QColor>
#include <QDebug>
#include <QDirIterator>
#include <QFileInfo>
#include <QImageReader>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QStorageInfo>
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

  m_loadAllTimer = new QTimer(this);
  m_loadAllTimer->setInterval(30); // ~33fps batching
  connect(m_loadAllTimer, &QTimer::timeout, this, [this]() {
    if (m_viewportCullingEnabled) {
      m_loadAllTimer->stop();
      return;
    }

    int batchSize = 100;
    int itemsCount = m_items.count();

    for (int i = 0; i < batchSize; ++i) {
      if (m_loadAllIndex >= itemsCount) {
        m_loadAllTimer->stop();
        qDebug() << "[ViewportCulling] Load ALL Complete.";
        break;
      }

      // Skip if already loaded
      if (!m_items[m_loadAllIndex].isLoaded &&
          !m_activelyRequesting.contains(m_loadAllIndex)) {
        requestThumbnail(m_loadAllIndex);
      }
      m_loadAllIndex++;
    }
    // Update progress log occasionally?
  });
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

  // AGGRESSIVE VIEWPORT CULLING: Return early for off-screen items to prevent
  // QML overhead. However, we MUST NOT cull essential metadata roles
  // (FilePath, TYPE, etc.) because the PhotoViewer needs these even when
  // the item is off-screen in the grid.
  if (m_viewportCullingEnabled) {
    bool isEssentialRole =
        (role == FilePathRole || role == FileNameRole || role == IsVideoRole ||
         role == IsRawRole || role == ImageIndexRole ||
         role == IsSelectedRole || role == VersionRole);
    if (!isEssentialRole) {
      if (index.row() < m_visibleStartIndex - 12 ||
          index.row() > m_visibleEndIndex + 12) {
        return QVariant();
      }
    }
  }

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
    return FileTypeRouter::isRaw(ext);
  }
  case IsVideoRole: {
    QString ext = QFileInfo(item.path).suffix().toLower();
    return FileTypeRouter::isVideo(ext);
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
  roles[VersionRole] = "version";
  roles[SectionDayRole] = "sectionDay";
  roles[SectionMonthRole] = "sectionMonth";
  roles[SectionYearRole] = "sectionYear";
  roles[SectionWeekRole] = "sectionWeek";
  roles[IsRawRole] = "isRaw";
  roles[IsVideoRole] = "isVideo";
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
    qCritical() << "[ViewportCulling] TOGGLE:" << (enabled ? "ON" : "OFF")
                << "Total items:" << m_items.count();
    m_viewportCullingEnabled = enabled;
    emit viewportCullingEnabledChanged();

    if (enabled) {
      qCritical() << "[ViewportCulling] Canceling pending requests and "
                     "updating visible range";
      cancelPendingRequests();
      updateVisibleRange();
    } else {
      qCritical() << "[ViewportCulling] Loading ALL items (Throttled)";
      m_loadAllIndex = 0;
      m_loadAllTimer->start();
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
  cancelPendingRequests();
  m_items.clear();
  m_totalItems = 0;
  m_remainingItems = 0;
  m_visibleStartIndex = 0;
  m_visibleEndIndex = 0;
  m_activelyRequesting.clear();
  m_pendingLoadedIndices.clear();
  if (m_updateTimer)
    m_updateTimer->stop();
  if (m_forceUpdateTimer)
    m_forceUpdateTimer->stop();
  m_scannedCount = 0;
  m_loadedCount = 0;
  m_isLoading = false;
  endResetModel();
  emit remainingItemsChanged();
  emit scannedCountChanged();
  emit isLoadingChanged();
}

void ScrollBenchImageModel::scanDirectory(const QString &path) {
  clearData(); // Call clearData() to reset model state

  // Always allow new scan, cancelling previous one via Generation ID
  int myGen = ++m_scanGeneration;

  m_isLoading = true;
  emit isLoadingChanged(); // Re-emit to ensure UI knows we are loading

  auto task = [this, path, myGen]() {
    if (myGen != m_scanGeneration.load())
      return;

    QElapsedTimer timer;
    timer.start();

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
      QMetaObject::invokeMethod(this, [this, myGen]() {
        if (myGen != m_scanGeneration.load())
          return;
        m_isLoading = false;
        emit isLoadingChanged();
      });
      return;
    }

    // === NETWORK PATH DETECTION ===
    // Network shares have fundamentally different performance characteristics:
    // - Local MFT scan: microseconds
    // - Network SMB enumeration: seconds/minutes
    // For network paths: disable incremental updates to avoid UI thrashing and
    // race conditions
    bool isNetworkPath = false;

    // Check for UNC path (\\server\share)
    if (cleanPath.startsWith("\\\\")) {
      isNetworkPath = true;
      qDebug() << "[NetworkScan] Detected UNC path:" << cleanPath;
    } else if (cleanPath.length() >= 3 && cleanPath[1] == ':') {
      // Check if drive letter is a network drive
      QStorageInfo storage(cleanPath);
      if (storage.isValid()) {
        // On Windows, network drives show up differently in QStorageInfo
        // Check if the device path suggests network storage
        QString device = storage.device();
        if (device.startsWith("\\\\")) {
          isNetworkPath = true;
          qDebug() << "[NetworkScan] Detected mapped network drive:"
                   << cleanPath << "Device:" << device;
        }
      }
    }

    if (isNetworkPath) {
      qDebug() << "[NetworkScan] Using non-incremental scan strategy for "
                  "network path";
    } else {
      qDebug()
          << "[NetworkScan] Using incremental scan strategy for local path";
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

    QVector<ImageItem> batch;
    batch.reserve(1000);
    int totalFound = 0;
    QRegularExpression dateRegex("(\\d{8})_(\\d{6})");

    // --- Fast MFT Scan Attempt (Local drives only) ---
    bool fastScanSuccess = false;
    if (cleanPath.length() >= 3 && cleanPath[1] == ':' && cleanPath[2] == '/') {
      FastVolumeScanner fastScanner;
      if (fastScanner.scanVolume(cleanPath)) {
        if (myGen != m_scanGeneration.load())
          return;

        qDebug() << "ScrollBench FastScanner: Success! Filtering results...";
        QVector<QString> allFiles = fastScanner.getAllFiles();
        QString searchPrefix = cleanPath;
        if (!searchPrefix.endsWith("/"))
          searchPrefix += "/";

        int fastBatchCount = 0;

        for (const QString &f : allFiles) {
          if (myGen != m_scanGeneration.load())
            return;

          QString normalizedF = QDir::fromNativeSeparators(f);
          if (normalizedF.startsWith(searchPrefix, Qt::CaseInsensitive)) {
            QString ext = QFileInfo(f).suffix().toLower();
            if (extensions.contains(ext)) {
              ImageItem item;
              item.path = QUrl::fromLocalFile(f).toString();
              item.fileName = QFileInfo(f).fileName();
              item.color = "#444444";
              item.isLoaded = false;

              // Basic type detection for Fast Scan (extension only for speed?)
              // VerifyFileType might be too slow for MFT loop if we want
              // blazing start? But user demanded verification. Let's use
              // verifyFileType but with fallback.
              bool isImg = false, isVid = false, isRaw = false;
              FileTypeRouter::verifyFileType(item.path, isImg, isVid, isRaw);

              if (!isImg && !isVid && !isRaw) {
                isVid = FileTypeRouter::isVideo(ext);
                isRaw = FileTypeRouter::isRaw(ext);
              }
              item.isRaw = isRaw;
              item.isVideo = isVid;

              QRegularExpressionMatch match = dateRegex.match(item.fileName);
              if (match.hasMatch()) {
                QString dateStr = match.captured(1) + match.captured(2);
                QDateTime dt = QDateTime::fromString(dateStr, "yyyyMMddHHmmss");
                if (dt.isValid()) {
                  item.date = dt;
                } else {
                  item.date = QFileInfo(f).birthTime();
                }
              } else {
                item.date = QFileInfo(f).birthTime();
              }

              if (!item.date.isValid())
                item.date = QDateTime::currentDateTime();

              batch.append(item);
              totalFound++;

              // Update scannedCount for feedback (both local and network)
              if (totalFound % 100 == 0 || totalFound < 100) {
                QMetaObject::invokeMethod(this, [this, totalFound, myGen]() {
                  if (myGen != m_scanGeneration.load())
                    return;
                  m_scannedCount = totalFound;
                  emit scannedCountChanged();
                });
              }

              // Incremental UI updates for Fast Scan (Local paths only)
              if (!isNetworkPath) {
                fastBatchCount++;
                bool shouldUpdate =
                    (fastBatchCount >= 50 && totalFound < 200) ||
                    (fastBatchCount >= 200 && totalFound < 1000) ||
                    (fastBatchCount >= 1000);

                if (shouldUpdate) {
                  QList<ImageItem> safeBatch = batch; // Copy
                  QMetaObject::invokeMethod(this, [this, safeBatch, myGen]() {
                    if (myGen != m_scanGeneration.load())
                      return;
                    beginResetModel();
                    m_items = safeBatch; // Replace
                    std::sort(m_items.begin(), m_items.end(),
                              [](const ImageItem &a, const ImageItem &b) {
                                return a.date > b.date; // Descending
                              });
                    endResetModel();
                    emit layoutChanged();
                  });
                  fastBatchCount = 0;
                }
              }
            }
          }
        }
        fastScanSuccess = true;
      }
    }

    if (myGen != m_scanGeneration.load())
      return;

    if (!fastScanSuccess) {
      // Fallback
      QDirIterator it(cleanPath, filters, QDir::Files | QDir::Readable,
                      QDirIterator::Subdirectories);

      while (it.hasNext()) {
        if (myGen != m_scanGeneration.load())
          return;

        it.next();                          // Advance iterator
        QFileInfo fileInfo = it.fileInfo(); // Use cached info

        ImageItem item;
        item.fileName = fileInfo.fileName();
        item.path = QUrl::fromLocalFile(fileInfo.absoluteFilePath()).toString();
        item.color = "#444444";
        item.isLoaded = false;

        bool isImg = false, isVid = false, isRaw = false;
        FileTypeRouter::verifyFileType(item.path, isImg, isVid, isRaw);

        // Fallback: If verification failed/indeterminate, trust extension
        if (!isImg && !isVid && !isRaw) {
          QString ext = fileInfo.suffix().toLower();
          isVid = FileTypeRouter::isVideo(ext);
          isRaw = FileTypeRouter::isRaw(ext);
        }

        item.isRaw = isRaw;
        item.isVideo = isVid;

        item.date = fileInfo.birthTime();
        if (!item.date.isValid()) {
          item.date = fileInfo.lastModified();
        }
        if (!item.date.isValid()) {
          item.date = QDateTime::currentDateTime();
        }

        QRegularExpressionMatch match = dateRegex.match(item.fileName);
        if (match.hasMatch()) {
          QString dateStr = match.captured(1) + match.captured(2);
          QDateTime dt = QDateTime::fromString(dateStr, "yyyyMMddHHmmss");
          if (dt.isValid())
            item.date = dt;
        }

        batch.append(item);
        totalFound++;

        // Update scannedCount for feedback (both local and network)
        if (totalFound % 100 == 0 || totalFound < 100) {
          QMetaObject::invokeMethod(this, [this, totalFound, myGen]() {
            if (myGen != m_scanGeneration.load())
              return;
            m_scannedCount = totalFound;
            emit scannedCountChanged();
          });
        }

        // Incremental UI updates for feedback (Local paths only)
        // Network paths: collect all items first, then single update
        if (!isNetworkPath) {
          // Update at 10, 50, 200, 1000 items, then every 2000.
          bool shouldUpdate =
              (totalFound == 10 || totalFound == 50 || totalFound == 200 ||
               totalFound == 1000 || (totalFound % 2000 == 0));

          if (shouldUpdate) {
            // Dispatch update to main thread
            QList<ImageItem> safeBatch =
                batch; // Copy-on-write, relatively cheap
            QMetaObject::invokeMethod(this, [this, safeBatch, myGen]() {
              if (myGen != m_scanGeneration.load())
                return;
              beginResetModel();
              m_items = safeBatch;
              // Sort by Date Descending
              std::sort(m_items.begin(), m_items.end(),
                        [](const ImageItem &a, const ImageItem &b) {
                          return a.date > b.date;
                        });
              endResetModel();
              // Force viewport update
              emit layoutChanged();
            });
          }
        }
      }
    }

    if (myGen != m_scanGeneration.load())
      return;

    if (!batch.isEmpty()) {
      QMetaObject::invokeMethod(
          this, [this, batch, myGen, totalFound, cleanPath]() {
            if (myGen != m_scanGeneration.load())
              return;

            beginResetModel();
            m_items = batch;
            m_loadedCount = 0; // Fresh scan
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
            emit forceUpdateGridView();

            m_isLoading = false;
            m_scannedCount = totalFound;
            m_totalItems = m_items.count();
            m_remainingItems = m_totalItems;
            emit remainingItemsChanged();
            emit isLoadingChanged();
            emit scannedCountChanged();
            emit scanComplete(totalFound);
          });
    }
  };

  TaskScheduler::instance().addTask(task, TaskScheduler::IO_BOUND,
                                    TaskScheduler::Normal);
}

void ScrollBenchImageModel::updateVisibleRange() {
  static constexpr int BUFFER_SIZE =
      50; // Load 50 items ahead/behind for better coverage
  if (!m_viewportCullingEnabled) {
    qDebug() << "[ViewportCulling] DISABLED - skipping range update";
    return;
  }

  qDebug() << "[ViewportCulling] Current settings: m_viewportCullingEnabled="
           << m_viewportCullingEnabled << ", BUFFER_SIZE=" << BUFFER_SIZE;

  int startIdx = qMax(0, m_visibleStartIndex - BUFFER_SIZE);
  int endIdx = qMin(m_items.count() - 1, m_visibleEndIndex + BUFFER_SIZE);
  int rangeSize = endIdx - startIdx + 1;

  qCritical() << "[ViewportCulling] RANGE UPDATE:"
              << "\n  Viewport indices:" << m_visibleStartIndex << "to"
              << m_visibleEndIndex << "("
              << (m_visibleEndIndex - m_visibleStartIndex + 1) << "items)"
              << "\n  Buffered range:" << startIdx << "to" << endIdx << "("
              << rangeSize << "items with BUFFER_SIZE=" << BUFFER_SIZE << ")"
              << "\n  Total items in model:" << m_items.count();

  QSet<QString> visiblePaths;
  for (int i = startIdx; i <= endIdx; ++i) {
    if (i >= 0 && i < m_items.count()) {
      visiblePaths.insert(m_items[i].path);
    }
  }
  qCritical() << "[ViewportCulling] Setting" << visiblePaths.size()
              << "visible paths in VRM";
  VisibleRangeManager::instance().setVisiblePaths(visiblePaths);

  int requestedCount = 0;
  int alreadyLoaded = 0;
  for (int i = startIdx; i <= endIdx; ++i) {
    if (i >= 0 && i < m_items.count()) {
      if (!m_items[i].isLoaded) {
        requestThumbnail(i);
        requestedCount++;
      } else {
        alreadyLoaded++;
      }
    }
  }
  qCritical() << "[ViewportCulling] Requested:" << requestedCount
              << "| Already loaded:" << alreadyLoaded
              << "| Total in range:" << rangeSize;
}

void ScrollBenchImageModel::requestThumbnail(int index) {
  if (index < 0 || index >= m_items.count() || m_items[index].isLoaded) {
    return;
  }
  // Avoid duplicate requests if already pending
  if (m_activelyRequesting.contains(index))
    return;
  m_activelyRequesting.insert(index);

  int myGen = m_scanGeneration.load();
  auto deliverTask = [this, index, myGen]() {
    m_activelyRequesting.remove(index);
    // Generation Check: Ensure this simulator task belongs to the current scan
    if (myGen != m_scanGeneration.load()) {
      return;
    }

    if (index >= 0 && index < m_items.count()) {
      if (!m_items[index].isLoaded) {
        m_items[index].isLoaded = true;
        m_loadedCount++;
        m_remainingItems = qMax(0, m_remainingItems - 1);
        emit remainingItemsChanged();
        emit loadedCountChanged();
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

void ScrollBenchImageModel::cancelScan() {
  m_scanCancelled = true;
  m_scanGeneration++;
}

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

QStringList ScrollBenchImageModel::getSelectedPaths() const {
  QStringList paths;
  for (const auto &item : m_items) {
    if (item.isSelected) {
      paths.append(item.path);
    }
  }
  return paths;
}

qint64 ScrollBenchImageModel::getSelectedTotalSizeBytes() const {
  qint64 totalBytes = 0;
  for (const auto &item : m_items) {
    if (item.isSelected) {
      QFileInfo fi(item.path);
      totalBytes += fi.size();
    }
  }
  return totalBytes;
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

bool ScrollBenchImageModel::rotateImage(int index, int degrees) {
  if (index < 0 || index >= m_items.count())
    return false;

  const ImageItem &item = m_items[index];
  if (!m_imageProcessor) {
    m_imageProcessor = new ImageProcessor(this);
  }

  if (m_imageProcessor->rotateImage(item.path, degrees)) {
    // Increment version
    m_items[index].version++;

    // Force refresh by emitting dataChanged
    QModelIndex modelIndex = createIndex(index, 0);
    emit dataChanged(modelIndex, modelIndex,
                     {Qt::DisplayRole, ColorRole, VersionRole});
    return true;
  }
  return false;
}

void ScrollBenchImageModel::rotateSelected(int degrees) {
  if (!m_imageProcessor) {
    m_imageProcessor = new ImageProcessor(this);
  }

  for (int i = 0; i < m_items.count(); ++i) {
    if (m_items[i].isSelected && !m_items[i].isVideo) {
      if (m_imageProcessor->rotateImage(m_items[i].path, degrees)) {
        m_items[i].version++;
        QModelIndex modelIndex = createIndex(i, 0);
        emit dataChanged(modelIndex, modelIndex,
                         {Qt::DisplayRole, ColorRole, VersionRole});
      }
    }
  }
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
  meta["Type"] = QFileInfo(item.path).suffix().toUpper();

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

int ScrollBenchImageModel::stagedRequestCount() const {
  return AsyncImageProvider::stagedRequestCount();
}
