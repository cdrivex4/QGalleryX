#include "DesktopHelper.h"
#include "ImageModel.h"
#include "TaskScheduler.h"
#include "AsyncImageProvider.h"
#include "../src/FileCacheManager.h"
#include "../src/PassiveReadLatencyGuard.h"
#include "../src/FastVolumeScanner.h"
#include <QBuffer>
#include <QDataStream>
#include <QDate>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFile>
#include <QSaveFile>
#include <QFileInfo>
#include <QImageReader>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QtConcurrent>
#include <QStorageInfo>
#include <algorithm>
#include <libraw/libraw.h>

static QList<ImageInfo> filterImageList(const QList<ImageInfo> &items, const QString &query) {
  if (query.isEmpty()) {
    return items;
  }
  QList<ImageInfo> result;
  result.reserve(items.size());
  QString lowerQuery = query.toLower();
  for (const auto &item : items) {
    if (item.fileName.toLower().contains(lowerQuery) || item.filePath.toLower().contains(lowerQuery)) {
      result.append(item);
    }
  }
  return result;
}

ImageModel::ImageModel(QObject *parent) : QAbstractListModel(parent) {
  m_precacheTimer = new QTimer(this);
  connect(m_precacheTimer, &QTimer::timeout, this, &ImageModel::processPrecacheTick);
  m_precacheTimer->start(100);

  m_debounceTimer.setSingleShot(true); // BUG FIX C1: per-instance debounce for onDirectoryChanged

  connect(&m_folderWatcher, &QFileSystemWatcher::directoryChanged, this, &ImageModel::onDirectoryChanged);

  // Pass 'this' as context object so Qt automatically unregisters connection on destruction
  connect(&FileCacheManager::instance(), &FileCacheManager::cacheCleared, this, [this]() {
    // Detect that the active DB was nuked/cleared and immediately reset the aggressive crawler
    if (!m_allItems.isEmpty()) {
      int res = m_loadingResolution;
      QSize thumbSize(res, res);
      QList<QString> missing;
      missing.reserve(m_allItems.size());
      for (const auto &item : m_allItems) {
        if (!FileCacheManager::instance().isCached(item.filePath, thumbSize))
          missing.append(item.filePath);
      }
      {
        QMutexLocker lock(&m_crawlMutex);
        m_crawlWorkQueue = std::move(missing);
      }
      m_crawlQueueIndex.store(0);
      m_crawledCount.store(0);
      m_crawlPassComplete = m_crawlWorkQueue.isEmpty();
      m_crawlDbFull = false;
      qDebug() << "[ImageModel] Cache nuked! Automatically re-armed aggressive crawler with"
               << m_crawlWorkQueue.size() << "items for current folder/drive.";
    } else {
      QMutexLocker lock(&m_crawlMutex);
      m_crawlWorkQueue.clear();
      m_crawlQueueIndex.store(0);
      m_crawledCount.store(0);
      m_crawlPassComplete = false;
    }
    emit crawlerProgressChanged();
  });
}

ImageModel::~ImageModel() {
  m_aliveToken->store(false);
  m_scanGeneration++;
  m_precacheGeneration++;
  m_filterGeneration++;

  if (m_precacheTimer) {
    m_precacheTimer->stop();
  }

  if (!m_folderWatcher.directories().isEmpty()) {
    m_folderWatcher.removePaths(m_folderWatcher.directories());
  }

  QMutexLocker lock(&m_crawlMutex);
  m_crawlWorkQueue.clear();
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
    return DesktopHelper::staticGetFileType(info.filePath) == DesktopHelper::Raw;
  }
  case IsVideoRole:
    return info.isVideo;
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
  roles[IsVideoRole] = "isVideo";
  return roles;
}

qint64 ImageModel::getGroupKey(int index, int role) const {
  if (index < 0 || index >= m_images.size()) return -1;
  const QDate &d = m_images[index].date.date();
  switch (role) {
    case SectionYearRole:
      return d.year();
    case SectionMonthRole:
      return (qint64)d.year() * 100 + d.month();
    case SectionWeekRole:
      return (qint64)d.year() * 100 + d.weekNumber();
    case SectionDayRole:
    case DateSectionRole:
    default:
      return d.toJulianDay();
  }
}

#include "../src/FileCacheManager.h"

void ImageModel::scanDirectory(const QString &path) {
  uint64_t currentGen = ++m_scanGeneration;
  m_scanId++;
  m_isLoading = true;
  emit isLoadingChanged();

  QString cleanPath;
  QUrl url(path);
  if (url.isValid() && url.isLocalFile()) {
    cleanPath = url.toLocalFile();
  } else {
    cleanPath = path;
  }

  // Handle odd edge case: /C:/Users... which QUrl might return
  if (cleanPath.startsWith("/") && cleanPath.length() > 2 && cleanPath[2] == ':') {
    cleanPath = cleanPath.mid(1);
  }

  auto alive = m_aliveToken;
  QPointer<ImageModel> safeThis(this);

  cleanPath = QDir::cleanPath(cleanPath);
  cleanPath = QDir::toNativeSeparators(cleanPath);

  if (cleanPath.length() > 1 && cleanPath[1] == ':') {
    cleanPath[0] = cleanPath[0].toUpper();
  }
  // Drive roots (e.g. "X:\", "C:\") MUST keep their trailing slash on Windows
  // otherwise Win32 treats "X:" as a relative path to the drive's CWD!
  if (cleanPath.length() == 2 && cleanPath[1] == ':') {
    cleanPath += "\\";
  } else if (cleanPath.length() > 3 && cleanPath.endsWith('\\')) {
    cleanPath.chop(1);
  }

  // BUG FIX L1: Assign m_currentPath so onDirectoryChanged() can trigger re-scans.
  // Without this, the live QFileSystemWatcher fires but the handler immediately returns
  // because m_currentPath.isEmpty() was always true.
  m_currentPath = cleanPath;

  // --- STEP 1: INSTANT CACHED LOAD FROM FOLDER DB (0ms UI Display) ---
  QString cacheDir;
  QStringList args = QCoreApplication::arguments();
  int cacheDirIdx = args.indexOf("--cache-dir");
  if (cacheDirIdx != -1 && cacheDirIdx + 1 < args.size()) {
      cacheDir = args.at(cacheDirIdx + 1) + "/folder_caches";
  } else {
      cacheDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/folder_caches";
  }
  QDir().mkpath(cacheDir);
  QString pathHash = QString(QCryptographicHash::hash(cleanPath.toLower().toUtf8(), QCryptographicHash::Md5).toHex());
  QString cachePath = cacheDir + "/" + pathHash + ".bin";

  QList<ImageInfo> instantCachedItems;
  QFile cacheFile(cachePath);
  if (cacheFile.open(QIODevice::ReadOnly)) {
    QDataStream in(&cacheFile);
    int count = 0;
    in >> count;
    instantCachedItems.reserve(count);
    for (int i = 0; i < count; ++i) {
      QString p;
      qint64 s;
      QDateTime d;
      in >> p >> s >> d;
      ImageInfo info;
      info.filePath = p;
      int slashIdx = p.lastIndexOf('/');
      if (slashIdx < 0) slashIdx = p.lastIndexOf('\\');
      info.fileName = slashIdx >= 0 ? p.mid(slashIdx + 1) : p;
      info.size = s;
      info.date = d;
      info.isVideo = (DesktopHelper::staticGetFileType(p) == DesktopHelper::Video);
      instantCachedItems.append(info);
    }
    cacheFile.close();
  }

  // FIX 4: Spot-check .bin validity — reject stale cache if >2 out of 5 random entries are missing.
  // This catches the case where a user deleted files since the last scan and the .bin
  // is serving ghost entries that produce placeholder tiles.
  if (!instantCachedItems.isEmpty() && instantCachedItems.size() >= 5) {
    int missingCount = 0;
    int step = instantCachedItems.size() / 5;
    for (int probe = 0; probe < 5; ++probe) {
      if (!QFile::exists(instantCachedItems[probe * step].filePath))
        ++missingCount;
    }
    if (missingCount > 2) {
      qWarning() << "[ImageModel] .bin folder cache is stale (" << missingCount << "/5 spot-check entries missing)."
                 << "Discarding instant cache and falling through to cold MFT scan.";
      instantCachedItems.clear();
    }
  }

  if (!instantCachedItems.isEmpty()) {
    beginResetModel();
    m_allItems = std::move(instantCachedItems);
    if (m_filterQuery.isEmpty()) {
      m_images = m_allItems;
    } else {
      m_images = filterImageList(m_allItems, m_filterQuery);
    }
    endResetModel();

    m_totalCount = m_allItems.size();
    m_scanProgress = m_allItems.size();
    m_scanMethod = "Folder DB (Instant Cached)";
    emit totalCountChanged();
    emit scanProgressChanged();
    emit scanMethodChanged();
    emit itemsPopulated(m_scanId);
    emit passOneCompleted(m_scanId);

    // Build crawler work queue in background thread so UI thread is never blocked
    int res = m_loadingResolution;
    uint64_t curGen = m_scanGeneration.load();
    QThreadPool::globalInstance()->start([alive, safeThis, items = m_allItems, res, curGen]() {
      if (!alive->load() || !safeThis || safeThis->m_scanGeneration != curGen) return;
      QSize thumbSize(res, res);
      QList<QString> missing;
      missing.reserve(items.size());
      for (const auto &item : items) {
        if (!FileCacheManager::instance().isCached(item.filePath, thumbSize))
          missing.append(item.filePath);
      }
      if (!alive->load() || !safeThis || safeThis->m_scanGeneration != curGen) return;
      {
        QMutexLocker lock(&safeThis->m_crawlMutex);
        safeThis->m_crawlWorkQueue = std::move(missing);
      }
      safeThis->m_crawlQueueIndex.store(0);
      safeThis->m_crawledCount.store(0);
      safeThis->m_crawlPassComplete = safeThis->m_crawlWorkQueue.isEmpty();
      safeThis->m_crawlDbFull = false;
      QMetaObject::invokeMethod(safeThis, [alive, safeThis]() {
        if (!alive->load() || !safeThis) return;
        emit safeThis->crawlerProgressChanged();
      }, Qt::QueuedConnection);
    });

    // Viewport-bounded L2->L1 promotion
    int visStart = m_visibleStartIndex;
    int visEnd   = m_visibleEndIndex;
    QThreadPool::globalInstance()->start([alive, items = m_allItems, res, visStart, visEnd]() {
      if (!alive->load()) return;
      QSize sz(res, res);
      int start = std::max(0, visStart - 500);
      int end   = std::min((int)items.size() - 1, visEnd + 500);
      for (int i = start; i <= end; ++i) {
        if (!alive->load()) return;
        AsyncImageProvider::promoteL2ToL1(items[i].filePath, sz);
      }
    });

    qDebug() << "[ImageModel] Loaded" << m_allItems.size() << "cached items instantly for:" << cleanPath;
  } else {
    // Cold Path: no .bin cache exists (or it was discarded as stale)
    beginResetModel();
    m_allItems.clear();
    m_images.clear();
    endResetModel();
    m_totalCount = 0;
    m_scanProgress = 0;
    emit totalCountChanged();
    emit scanProgressChanged();
    {
      QMutexLocker lock(&m_crawlMutex);
      m_crawlWorkQueue.clear();
    }
    m_crawlQueueIndex.store(0);
    m_crawledCount.store(0);
    m_crawlPassComplete = false;
  }

  bool wasCached = !instantCachedItems.isEmpty();

  // Clear previous pending worker tasks (keep RAM cache warm)
  TaskScheduler::instance().clear();

  // Run live MFT / filesystem scanning via TaskScheduler
  TaskScheduler::instance().addTask(
      [alive, safeThis, cleanPath, currentGen, cachePath, wasCached]() mutable {
        if (!alive->load() || !safeThis) return;
        QElapsedTimer timer;
        timer.start();

        if (!QDir(cleanPath).exists()) {
          qWarning() << "Directory does not exist:" << cleanPath;
          QMetaObject::invokeMethod(safeThis, [alive, safeThis]() {
            if (!alive->load() || !safeThis) return;
            safeThis->m_isLoading = false;
            emit safeThis->isLoadingChanged();
          }, Qt::QueuedConnection);
          return;
        }

        qDebug() << "Scanning directory:" << cleanPath;

        QMetaObject::invokeMethod(safeThis, [alive, safeThis, cleanPath]() {
          if (!alive->load() || !safeThis) return;
          if (!safeThis->m_folderWatcher.directories().isEmpty()) {
            safeThis->m_folderWatcher.removePaths(safeThis->m_folderWatcher.directories());
          }
          safeThis->m_folderWatcher.addPath(cleanPath);
        }, Qt::QueuedConnection);

        bool isNetworkPath = DesktopHelper::staticIsNetworkPath(cleanPath);
        qDebug() << "[Scan]" << (isNetworkPath ? "Network path:" : "Local path:") << cleanPath;

        const QStringList &extensions = DesktopHelper::supportedExtensions();
        const QStringList &nameFilters = DesktopHelper::supportedNameFilters();

        QList<ImageInfo> fastItems;
        QRegularExpression dateRegex("(\\d{8})_(\\d{6})");

        QSettings appSettings("SamsungClone", "Gallery");
        int scanEngineMode = appSettings.value("scanEngineMode", 0).toInt(); // 0=Auto, 1=Force MFT, 2=Force QDirIterator, 3=Force DB

        // --- Fast MFT Scan Attempt (Direct NTFS Master File Table Read) ---
        bool fastScanSuccess = false;
        if (scanEngineMode != 2 && scanEngineMode != 3 && cleanPath.length() >= 2 && cleanPath[1] == ':' && !isNetworkPath) {
          FastVolumeScanner fastScanner;
          if (fastScanner.scanVolume(cleanPath)) {
            qDebug() << "[FastScanner] MFT Direct Read Succeeded for volume:" << cleanPath;
            QVector<ScannedFile> scannedFiles = fastScanner.getScannedFiles();
            QString searchPrefix = QDir::fromNativeSeparators(cleanPath).toLower();
            if (!searchPrefix.endsWith("/")) searchPrefix += "/";

            for (const ScannedFile &sf : scannedFiles) {
              if (!alive->load() || !safeThis || safeThis->m_scanGeneration != currentGen) return;
              QString sfNorm = QDir::fromNativeSeparators(sf.path).toLower();
              if (sfNorm.startsWith(searchPrefix)) {
                int dotIdx = sfNorm.lastIndexOf('.');
                if (dotIdx > 0) {
                  QString ext = sfNorm.mid(dotIdx + 1);
                  if (extensions.contains(ext)) {
                    ImageInfo info;
                    info.filePath = QDir::toNativeSeparators(sf.path);
                    int slashIdx = sf.path.lastIndexOf('/');
                    if (slashIdx < 0) slashIdx = sf.path.lastIndexOf('\\');
                    info.fileName = slashIdx >= 0 ? sf.path.mid(slashIdx + 1) : sf.path;
                    info.size = sf.size;
                    if (sf.creationTime > 0) {
                      qint64 msecsSince1601 = sf.creationTime / 10000;
                      qint64 msecsSinceEpoch = msecsSince1601 - 11644473600000LL;
                      info.date = QDateTime::fromMSecsSinceEpoch(msecsSinceEpoch);
                    } else {
                      info.date = QDateTime::currentDateTime();
                    }
                    info.isVideo = (DesktopHelper::staticGetFileType(info.filePath) == DesktopHelper::Video);
                    fastItems.append(info);
                  }
                }
              }
            }
            if (!fastItems.isEmpty()) {
              fastScanSuccess = true;
            }
            qDebug() << "[FastScanner] MFT populated" << fastItems.size() << "items in" << timer.elapsed() << "ms";
          }
        }

        // === Fallback to QDirIterator if MFT unavailable ===
        if (!fastScanSuccess) {
          QDirIterator it(cleanPath, nameFilters, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);

          int localScanCount = 0;
          int nextBatchThreshold = 40;
          while (it.hasNext()) {
            if (!alive->load() || !safeThis || safeThis->m_scanGeneration != currentGen) {
                qDebug() << "[ImageModel] Scan cancelled for" << cleanPath;
                return;
            }
            it.next();

            QString filePath = it.filePath();
            // Filter out developer / package manager / OS system non-media folders
            if (filePath.contains("/node_modules/", Qt::CaseInsensitive) || filePath.contains("\\node_modules\\", Qt::CaseInsensitive) ||
                filePath.contains("/.git/", Qt::CaseInsensitive) || filePath.contains("\\.git\\", Qt::CaseInsensitive) ||
                filePath.contains("/.vscode/", Qt::CaseInsensitive) || filePath.contains("\\.vscode\\", Qt::CaseInsensitive) ||
                filePath.contains("/build/", Qt::CaseInsensitive) || filePath.contains("\\build\\", Qt::CaseInsensitive) ||
                filePath.contains("/AppData/Local/Packages/", Qt::CaseInsensitive) || filePath.contains("\\AppData\\Local\\Packages\\", Qt::CaseInsensitive) ||
                filePath.contains("/AppData/Local/Temp/", Qt::CaseInsensitive) || filePath.contains("\\AppData\\Local\\Temp\\", Qt::CaseInsensitive) ||
                filePath.contains("/Windows/WinSxS/", Qt::CaseInsensitive) || filePath.contains("\\Windows\\WinSxS\\", Qt::CaseInsensitive) ||
                filePath.contains("/Windows/System32/", Qt::CaseInsensitive) || filePath.contains("\\Windows\\System32\\", Qt::CaseInsensitive)) {
              continue;
            }

            auto fileType = DesktopHelper::staticGetFileType(filePath);
            if (fileType == DesktopHelper::Unknown) {
              continue;
            }

            localScanCount++;

            QFileInfo fi = it.fileInfo();
            ImageInfo info;
            info.filePath = QDir::toNativeSeparators(filePath);
            info.fileName = it.fileName();
            info.size = fi.size();
            info.date = fi.lastModified();
            info.isVideo = (fileType == DesktopHelper::Video);

            // Optimize: Prefer camera filename timestamp (e.g. 20240101_120000) if present
            QRegularExpressionMatch match = dateRegex.match(info.fileName);
            if (match.hasMatch()) {
              QString dateStr = match.captured(1) + match.captured(2);
              QDateTime dt = QDateTime::fromString(dateStr, "yyyyMMddHHmmss");
              if (dt.isValid())
                info.date = dt;
            }
            fastItems.append(info);

            // Stream items to UI as discovered
            if (!wasCached && localScanCount >= nextBatchThreshold) {
                if (nextBatchThreshold == 40) nextBatchThreshold = 150;
                else if (nextBatchThreshold == 150) nextBatchThreshold = 500;
                else if (nextBatchThreshold == 500) nextBatchThreshold = 1500;
                else if (nextBatchThreshold == 1500) nextBatchThreshold = 5000;
                else nextBatchThreshold += 5000;

                QList<ImageInfo> batchItems = fastItems;
                QMetaObject::invokeMethod(safeThis, [alive, safeThis, batchItems, localScanCount, currentGen]() {
                    if (!alive->load() || !safeThis || safeThis->m_scanGeneration != currentGen) return;
                    safeThis->m_allItems = batchItems;
                    safeThis->applyFilter();
                    safeThis->m_scanProgress = localScanCount;
                    emit safeThis->scanProgressChanged();
                    emit safeThis->itemsPopulated(safeThis->m_scanId);
                }, Qt::QueuedConnection);
            } else if (localScanCount % 200 == 0) {
                QMetaObject::invokeMethod(safeThis, [alive, safeThis, localScanCount, currentGen]() {
                    if (!alive->load() || !safeThis || safeThis->m_scanGeneration != currentGen) return;
                    safeThis->m_scanProgress = localScanCount;
                    emit safeThis->scanProgressChanged();
                }, Qt::QueuedConnection);
            }
          }
        }

        QMetaObject::invokeMethod(safeThis, [alive, safeThis, fastItems, currentGen]() {
            if (!alive->load() || !safeThis || safeThis->m_scanGeneration != currentGen) return;
            safeThis->m_totalCount = fastItems.size();
            emit safeThis->totalCountChanged();
        }, Qt::QueuedConnection);

        // Universal Folder Metadata Database (both local & network)
        QHash<QString, QPair<qint64, QDateTime>> cachedData;
        QString cacheDir;
        QStringList args = QCoreApplication::arguments();
        int cacheDirIdx = args.indexOf("--cache-dir");
        if (cacheDirIdx != -1 && cacheDirIdx + 1 < args.size()) {
            cacheDir = args.at(cacheDirIdx + 1) + "/folder_caches";
        } else {
            cacheDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/folder_caches";
        }
        QDir().mkpath(cacheDir);
        QString pathHash = QString(QCryptographicHash::hash(cleanPath.toLower().toUtf8(), QCryptographicHash::Md5).toHex());
        QString cachePath = cacheDir + "/" + pathHash + ".bin";
        
        bool fullyCached = false;
        QFile cacheFile(cachePath);
        if (cacheFile.open(QIODevice::ReadOnly)) {
            QDataStream in(&cacheFile);
            int count = 0;
            in >> count;
            if (count == fastItems.size()) {
                for (int i = 0; i < count; ++i) {
                    QString p;
                    qint64 s;
                    QDateTime d;
                    in >> p >> s >> d;
                    cachedData.insert(p, qMakePair(s, d));
                }
                fullyCached = true;
            } else if (count > 0 && count < fastItems.size() * 2) {
                for (int i = 0; i < count; ++i) {
                    QString p;
                    qint64 s;
                    QDateTime d;
                    in >> p >> s >> d;
                    cachedData.insert(p, qMakePair(s, d));
                }
            }
        }

        // Fast initial sort based on Regex / MFT dates
        std::sort(fastItems.begin(), fastItems.end(),
                  [](const ImageInfo &a, const ImageInfo &b) {
                    return a.date > b.date;
                  });

        QString method;
        if (fastScanSuccess) {
          method = "Direct MFT (NTFS)";
        } else if (fullyCached) {
          method = "Folder DB (Cached)";
        } else if (isNetworkPath) {
          method = "SMB Network Scan";
        } else {
          method = "QDirIterator (Recursive)";
        }
        int scanDuration = (int)timer.elapsed();

        // Pre-filter skeleton on background worker thread
        QString filter = safeThis ? safeThis->m_filterQuery : QString();
        QList<ImageInfo> filteredSkeleton = filterImageList(fastItems, filter);

        // Send skeleton grid to UI instantly (only reset model if cold uncached scan)
        std::shared_ptr<std::atomic<bool>> skelAlive = alive;
        QPointer<ImageModel> skelSafeThis = safeThis;
        QMetaObject::invokeMethod(skelSafeThis, [skelAlive, skelSafeThis, fastItems, filteredSkeleton = std::move(filteredSkeleton), currentGen, method, scanDuration, wasCached]() mutable {
          if (!skelAlive->load() || !skelSafeThis || skelSafeThis->m_scanGeneration != currentGen) return;
          if (!wasCached) {
            skelSafeThis->beginResetModel();
            skelSafeThis->m_allItems = std::move(fastItems);
            skelSafeThis->m_images = std::move(filteredSkeleton);
            skelSafeThis->endResetModel();
          }

          skelSafeThis->m_totalCount = skelSafeThis->m_allItems.size();
          skelSafeThis->m_scanProgress = skelSafeThis->m_allItems.size();
          skelSafeThis->m_scanMethod = method;
          skelSafeThis->m_scanDurationMs = scanDuration;
          emit skelSafeThis->totalCountChanged();
          emit skelSafeThis->scanProgressChanged();
          emit skelSafeThis->scanMethodChanged();
          emit skelSafeThis->itemsPopulated(skelSafeThis->m_scanId);
          emit skelSafeThis->passOneCompleted(skelSafeThis->m_scanId);
        }, Qt::QueuedConnection);

        // === PASS 2: METADATA FILL ===
        bool cacheChanged = false;
        if (!fullyCached && !fastScanSuccess) {
          for (int i = 0; i < fastItems.size(); ++i) {
            if (!alive->load() || !safeThis || safeThis->m_scanGeneration != currentGen) return;

            while (TaskScheduler::instance().isPaused() && TaskScheduler::instance().isRunning()) {
              QThread::msleep(50);
            }
            if (!TaskScheduler::instance().isRunning()) break;

            if (i > 0 && i % 500 == 0) {
              QThread::msleep(1); // Yield IO to foreground image decoders
            }

            if (cachedData.contains(fastItems[i].filePath)) {
                fastItems[i].size = cachedData[fastItems[i].filePath].first;
                fastItems[i].date = cachedData[fastItems[i].filePath].second;
                continue;
            }

            if (fastItems[i].date.isNull() || fastItems[i].size == 0) {
              QFileInfo fi(fastItems[i].filePath);
              fastItems[i].size = fi.size();
              if (fastItems[i].date.isNull()) {
                fastItems[i].date = fi.birthTime();
              }
              cacheChanged = true;
            }
          }
        } else if (fullyCached) {
          for (int i = 0; i < fastItems.size(); ++i) {
            if (cachedData.contains(fastItems[i].filePath)) {
              fastItems[i].size = cachedData[fastItems[i].filePath].first;
              fastItems[i].date = cachedData[fastItems[i].filePath].second;
            }
          }
        }

        // Persist universal folder cache database
        if ((cacheChanged || !fullyCached || fastScanSuccess) && TaskScheduler::instance().isRunning() && alive->load() && safeThis && safeThis->m_scanGeneration == currentGen) {
            QSaveFile outCache(cachePath);
            if (outCache.open(QIODevice::WriteOnly)) {
                QDataStream out(&outCache);
                out << fastItems.size();
                for (const auto &item : fastItems) {
                    out << item.filePath << item.size << item.date;
                }
                outCache.commit();
            }
        }

        if (!alive->load() || !safeThis || safeThis->m_scanGeneration != currentGen) return;

        // Final perfect sort (on background IO worker thread)
        std::sort(fastItems.begin(), fastItems.end(),
                  [](const ImageInfo &a, const ImageInfo &b) {
                    return a.date > b.date;
                  });

        // Build crawler work queue on background thread
        int res = safeThis ? safeThis->m_loadingResolution : 200;
        QSize thumbSize(res, res);
        QList<QString> missing;
        missing.reserve(fastItems.size());
        for (const auto &item : fastItems) {
          if (!FileCacheManager::instance().isCached(item.filePath, thumbSize))
            missing.append(item.filePath);
        }

        // Pre-filter on background worker thread
        QString currentFilter = safeThis ? safeThis->m_filterQuery : QString();
        QList<ImageInfo> finalFiltered = filterImageList(fastItems, currentFilter);

        // Final UI commit & reconciliation
        std::shared_ptr<std::atomic<bool>> uiAlive = alive;
        QPointer<ImageModel> uiSafeThis = safeThis;
        QMetaObject::invokeMethod(uiSafeThis, [uiAlive, uiSafeThis, cleanPath, fastItems = std::move(fastItems), finalFiltered = std::move(finalFiltered), missing = std::move(missing), timer, currentGen, fastScanSuccess, method, res]() mutable {
          if (!uiAlive->load() || !uiSafeThis || uiSafeThis->m_scanGeneration != currentGen) return;

          bool filesChanged = true;
          if (fastItems.size() == uiSafeThis->m_allItems.size()) {
            filesChanged = false;
            for (int i = 0; i < fastItems.size(); ++i) {
              if (fastItems[i].filePath != uiSafeThis->m_allItems[i].filePath) {
                filesChanged = true;
                break;
              }
            }
          }

          if (filesChanged) {
            uiSafeThis->beginResetModel();
            uiSafeThis->m_allItems = std::move(fastItems);
            uiSafeThis->m_images = std::move(finalFiltered);
            uiSafeThis->endResetModel();

            uiSafeThis->m_totalCount = uiSafeThis->m_allItems.size();
            uiSafeThis->m_scanProgress = uiSafeThis->m_allItems.size();
            emit uiSafeThis->totalCountChanged();
            emit uiSafeThis->scanProgressChanged();
            emit uiSafeThis->itemsPopulated(uiSafeThis->m_scanId);
          }

          uiSafeThis->m_isLoading = false;
          uiSafeThis->m_scanMethod = fastScanSuccess ? "Direct MFT (Verified)" : method;
          uiSafeThis->m_scanDurationMs = (int)timer.elapsed();
          emit uiSafeThis->isLoadingChanged();
          emit uiSafeThis->scanMethodChanged();

          qDebug() << "[ImageModel] Verified directory scan in" << timer.elapsed() << "ms. Total items:" << uiSafeThis->m_allItems.size();

          {
            QMutexLocker lock(&uiSafeThis->m_crawlMutex);
            uiSafeThis->m_crawlWorkQueue = std::move(missing);
          }
          uiSafeThis->m_crawlQueueIndex.store(0);
          uiSafeThis->m_crawledCount.store(0);
          uiSafeThis->m_crawlPassComplete = uiSafeThis->m_crawlWorkQueue.isEmpty();
          uiSafeThis->m_crawlDbFull = false;
          qDebug() << "[Crawler] Work queue ready:" << uiSafeThis->m_crawlWorkQueue.size()
                   << "uncached /" << uiSafeThis->m_allItems.size() << "total.";
          emit uiSafeThis->crawlerProgressChanged();

          // Reconcile DB against actual filesystem:
          // Only prune entries in cleanPath whose source files were deleted
          QList<ImageInfo> allScannedItems = uiSafeThis->m_allItems;
          QThreadPool::globalInstance()->start([cleanPath, items = allScannedItems, res]() {
            QSet<QString> validPaths;
            validPaths.reserve(items.size());
            for (const auto &item : items)
              validPaths.insert(item.filePath);
            FileCacheManager::instance().pruneStaleEntries(cleanPath, validPaths, QSize(res, res));
          });

          // FIX 3: Viewport-bounded L2->L1 promotion.
          // Only warm the viewport window (±500 items) to avoid blowing out QCache
          // with thousands of entries that get evicted before scrolling reaches them.
          int visStart = uiSafeThis->m_visibleStartIndex;
          int visEnd   = uiSafeThis->m_visibleEndIndex;
          std::shared_ptr<std::atomic<bool>> promoAlive = uiAlive;
          QThreadPool::globalInstance()->start([promoAlive, allScannedItems, res, visStart, visEnd]() {
            if (!promoAlive->load()) return;
            QSize sz(res, res);
            int start = std::max(0, visStart - 500);
            int end   = std::min((int)allScannedItems.size() - 1, visEnd + 500);
            qDebug() << "[Promotion] Viewport-bounded L2->L1 promotion:" << (end - start + 1) << "items.";
            for (int i = start; i <= end; ++i) {
              if (!promoAlive->load()) return;
              AsyncImageProvider::promoteL2ToL1(allScannedItems[i].filePath, sz);
            }
          });
        }, Qt::QueuedConnection);
      },
      TaskScheduler::IO_BOUND, TaskScheduler::Immediate);
}

void ImageModel::pauseBackgroundTasks() {
  TaskScheduler::instance().pause();
}

void ImageModel::resumeBackgroundTasks() {
  TaskScheduler::instance().resume();
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

    if (m_filterQuery.isEmpty()) {
      beginResetModel();
      m_images = m_allItems;
      endResetModel();
      return;
    }

    uint64_t curGen = ++m_filterGeneration;
    // BUG FIX C2: Capture alive token + QPointer to guard against model destruction
    // before the threadpool task or inner QueuedConnection callback execute.
    auto alive = m_aliveToken;
    QPointer<ImageModel> safeThis(this);
    QThreadPool::globalInstance()->start([alive, safeThis, items = m_allItems, query = m_filterQuery, curGen]() {
      if (!alive->load() || !safeThis) return;
      if (safeThis->m_filterGeneration.load() != curGen) return;
      QList<ImageInfo> filtered = filterImageList(items, query);
      if (!alive->load() || !safeThis) return;
      if (safeThis->m_filterGeneration.load() != curGen) return;

      QMetaObject::invokeMethod(safeThis, [alive, safeThis, filtered = std::move(filtered), curGen]() mutable {
        if (!alive->load() || !safeThis) return;
        if (safeThis->m_filterGeneration.load() != curGen) return;
        safeThis->beginResetModel();
        safeThis->m_images = std::move(filtered);
        safeThis->endResetModel();
      }, Qt::QueuedConnection);
    });
  }
}

void ImageModel::applyFilter() {
  beginResetModel();
  if (m_filterQuery.isEmpty()) {
    m_images = m_allItems;
  } else {
    m_images = filterImageList(m_allItems, m_filterQuery);
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

void ImageModel::selectRange(int fromIndex, int toIndex, bool isSelected) {
  int total = static_cast<int>(m_images.count());
  int start = std::max(0, std::min(fromIndex, toIndex));
  int end = std::min(total - 1, std::max(fromIndex, toIndex));
  bool changed = false;

  for (int i = start; i <= end; ++i) {
    if (m_images[i].isSelected != isSelected) {
      m_images[i].isSelected = isSelected;
      changed = true;
    }
  }

  if (changed) {
    emit dataChanged(createIndex(start, 0), createIndex(end, 0), {IsSelectedRole});
    emit selectedCountChanged();
  }
}

void ImageModel::toggleSelection(int index) {
  if (index >= 0 && index < m_images.count()) {
    m_images[index].isSelected = !m_images[index].isSelected;
    emit dataChanged(createIndex(index, 0), createIndex(index, 0), {IsSelectedRole});
    emit selectedCountChanged();
  }
}

void ImageModel::deleteSelected() {
  // BUG FIX C4: Must remove from BOTH m_images AND m_allItems.
  // Previously only m_images was updated, so deleted items reappeared after
  // any subsequent applyFilter() or filter query change.
  QSet<QString> deletedPaths;

  for (int i = m_images.count() - 1; i >= 0; --i) {
    if (m_images[i].isSelected) {
      deletedPaths.insert(m_images[i].filePath);
      beginRemoveRows(QModelIndex(), i, i);
      m_images.removeAt(i);
      endRemoveRows();
    }
  }

  // Remove matching entries from m_allItems too (the unfiltered master list)
  if (!deletedPaths.isEmpty()) {
    for (int i = m_allItems.count() - 1; i >= 0; --i) {
      if (deletedPaths.contains(m_allItems[i].filePath)) {
        m_allItems.removeAt(i);
      }
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
  dirs.reserve(1024);
  for (const auto &item : m_images) {
    int lastSlash = item.filePath.lastIndexOf('/');
    if (lastSlash < 0) lastSlash = item.filePath.lastIndexOf('\\');
    QString dirPath = (lastSlash >= 0) ? item.filePath.left(lastSlash) : item.filePath;
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

int ImageModel::indexOfPath(const QString& path) const {
  if (path.isEmpty()) return -1;
  QString target = QDir::fromNativeSeparators(path);
  for (int i = 0; i < m_images.count(); ++i) {
    if (m_images[i].filePath.compare(target, Qt::CaseInsensitive) == 0 ||
        QDir::fromNativeSeparators(m_images[i].filePath).compare(target, Qt::CaseInsensitive) == 0) {
      return i;
    }
  }
  return -1;
}

void ImageModel::processPrecacheTick() {
  if (m_precacheMode == 0 || m_images.isEmpty()) {
    return; // Battery Saver: do nothing
  }
  if (!TaskScheduler::instance().isRunning() || TaskScheduler::instance().isPaused())
    return;
  if (m_isLoading.load() || m_images.isEmpty() || m_precacheMode == 0)
    return;

  // L2 Disk Cache Storage Capacity Guard
  {
    QSettings settings("SamsungClone", "Gallery");
    qint64 limitBytes = qint64(settings.value("diskCacheSizeMB", 4096).toInt()) * 1024LL * 1024LL;
    qint64 usedBytes = FileCacheManager::instance().getTrackedRootPathStats()
                           .value("__total__").toMap().value("bytes", 0LL).toLongLong();
    if (limitBytes > 0 && usedBytes >= qint64(limitBytes * 0.95)) {
      if (!m_crawlDbFull) {
        m_crawlDbFull = true;
        qDebug() << "[Crawler] L2 disk cache at capacity (" << usedBytes / (1024*1024)
                 << "MB /" << limitBytes / (1024*1024) << "MB). Crawl suspended.";
      }
      return;
    }
    m_crawlDbFull = false;
  }


  const QSize thumbSize(m_loadingResolution, m_loadingResolution);

  // Dynamic I/O Bandwidth Back-Off: Query hardware latency guard
  int maxInflight = PassiveReadLatencyGuard::instance().recommendedConcurrency();
  int throttleDelay = PassiveReadLatencyGuard::instance().throttleDelayMs();

  // If the drive is congested (>150ms spike), inject a breather delay to yield bus to UI.
  // CRITICAL: Use per-instance m_crawlerThrottleTimer — NOT a static local.
  // A static timer would cause Window 2's viewport scroll to stall Window 1's background crawl.
  if (throttleDelay > 0) {
    if (!m_crawlerThrottleTimer.isValid()) {
      m_crawlerThrottleTimer.start();
    } else if (m_crawlerThrottleTimer.elapsed() < throttleDelay) {
      // Emit throttled state once (not every tick) to avoid spamming the toast
      if (!m_crawlerThrottledState) {
        m_crawlerThrottledState = true;
        emit crawlerThrottled(true);
        emit crawlerStatusChanged("⏸ Background crawl paused — yielding I/O to viewport", false);
      }
      return;
    }
  }

  // Crawler is running — clear throttled state and notify UI on transition
  if (m_crawlerThrottledState) {
    m_crawlerThrottledState = false;
    emit crawlerThrottled(false);
    emit crawlerStatusChanged("▶ Background crawl resumed", false);
  }
  m_crawlerThrottleTimer.restart();

  // ----------------------------------------------------------------
  // Mode 1 (Yellow): Lookahead window — ±50 items around viewport.
  // ----------------------------------------------------------------
  if (m_precacheMode == 1) {
    if (m_crawlInflight.load() >= maxInflight) return;

    int start = std::max(0, m_visibleStartIndex - 50);
    int end   = std::min((int)m_images.count() - 1, m_visibleEndIndex + 50);
    uint64_t currentGen = m_scanGeneration.load();

    for (int i = start; i <= end && m_crawlInflight.load() < maxInflight; ++i) {
      if (m_scanGeneration.load() != currentGen) return;
      const QString path = m_images[i].filePath;
      auto lvl = AsyncImageProvider::checkCacheLevel(path, thumbSize);
      if (lvl != AsyncImageProvider::NotAvailable) {
        if (lvl == AsyncImageProvider::OnDisk) {
          QSize sz = thumbSize;
          TaskScheduler::instance().addTask([path, sz]() {
            AsyncImageProvider::promoteL2ToL1(path, sz);
          }, TaskScheduler::CPU_BOUND, TaskScheduler::Low);
        }
        continue;
      }

      auto inflightToken = std::shared_ptr<void>(nullptr, [this](void*) {
        this->m_crawlInflight.fetch_sub(1, std::memory_order_relaxed);
      });
      m_crawlInflight.fetch_add(1, std::memory_order_relaxed);
      QSize sz = thumbSize;
      bool added = TaskScheduler::instance().addTask([this, path, sz, currentGen, inflightToken]() {
        if (this->m_scanGeneration.load() != currentGen) {
          return;
        }
        AsyncImageProvider::crawlDecodeToL2(path, sz);
      }, TaskScheduler::CPU_BOUND, TaskScheduler::Normal);

      if (!added) {
        break;
      }
    }
    return;
  }

  // ----------------------------------------------------------------
  // Mode 2 (Red): Aggressive full-model sequential background crawler.
  // Drain the mmap-backed work queue.
  // ----------------------------------------------------------------
  if (m_precacheMode == 2) {
    int queueSize = 0;
    {
      QMutexLocker lock(&m_crawlMutex); // FIX 2: read queue size safely
      queueSize = m_crawlWorkQueue.size();
    }
    if (queueSize == 0) {
      // Nothing missing — queue was empty at scan time (all cached already)
      if (!m_crawlPassComplete) {
        m_crawlPassComplete = true;
        qDebug() << "[Crawler] Work queue empty at start — all files already cached. Idle.";
        emit crawlerProgressChanged();
      }
      return;
    }

    int idx = m_crawlQueueIndex.load();
    if (idx >= queueSize && m_crawlInflight.load() == 0) {
      // All items dispatched and inflight tasks finished — truly done.
      if (!m_crawlPassComplete) {
        m_crawlPassComplete = true;
        qDebug() << "[Crawler] Work queue exhausted (" << queueSize << "files processed). Idle.";
        emit crawlerProgressChanged();
      }
      return;
    }

    int maxConcurrency = std::max(8, (int)std::thread::hardware_concurrency() * 2);
    if (m_crawlInflight.load() >= maxConcurrency) return;

    int batchLimit = maxConcurrency * 2;
    int submitted = 0;
    uint64_t currentGen = m_scanGeneration.load();

    while (submitted < batchLimit && m_crawlInflight.load() < maxConcurrency) {
      if (m_scanGeneration.load() != currentGen) return;
      int i = m_crawlQueueIndex.fetch_add(1, std::memory_order_relaxed);
      if (i >= queueSize) break;

      QString path;
      {
        QMutexLocker lock(&m_crawlMutex); // FIX 2: guard item read
        if (i >= m_crawlWorkQueue.size()) {
          // Queue was replaced concurrently — back off this slot
          m_crawlQueueIndex.fetch_sub(1, std::memory_order_relaxed);
          break;
        }
        path = m_crawlWorkQueue[i];
      }

      auto inflightToken = std::shared_ptr<void>(nullptr, [this](void*) {
        this->m_crawlInflight.fetch_sub(1, std::memory_order_relaxed);
      });
      m_crawlInflight.fetch_add(1, std::memory_order_relaxed);
      QSize sz = thumbSize;
      auto alive = m_aliveToken;
      bool added = TaskScheduler::instance().addTask([this, alive, path, sz, currentGen, inflightToken]() {
        if (!alive->load() || this->m_scanGeneration.load() != currentGen) {
          return;
        }
        AsyncImageProvider::crawlDecodeToL2(path, sz);
        this->m_crawledCount.fetch_add(1, std::memory_order_relaxed);
        // BUG FIX R1: emit must happen on the GUI thread. Direct emit from a worker
        // thread is undefined behaviour — it can corrupt QML property bindings.
        QMetaObject::invokeMethod(this, [this, alive]() {
          if (alive->load()) emit this->crawlerProgressChanged();
        }, Qt::QueuedConnection);
      }, TaskScheduler::CPU_BOUND, TaskScheduler::Low);

      if (!added) {
        // Task rejected — back off the index so this slot is retried next tick
        m_crawlQueueIndex.fetch_sub(1, std::memory_order_relaxed);
        break;
      }
      ++submitted;
    }
    emit crawlerProgressChanged();
  }

}

void ImageModel::processMetadataTick() {}
void ImageModel::setVisibleEndIndex(int idx) { m_visibleEndIndex = idx; emit visibleIndicesChanged(); }
void ImageModel::setVisibleStartIndex(int idx) { m_visibleStartIndex = idx; emit visibleIndicesChanged(); }
void ImageModel::setLoadingResolution(int res) { m_loadingResolution = res; emit loadingResolutionChanged(); }

void ImageModel::setPrecacheMode(int mode) {
  if (m_precacheMode != mode) {
    m_precacheMode = mode;
    // Don't destroy the work queue when switching modes — it reflects ground truth.
    // Just reset the position so Red mode resumes from the start of the missing list.
    m_crawlQueueIndex.store(0);
    m_crawledCount.store(0);
    m_crawlDbFull = false;
    m_crawlPassComplete = false;
    qDebug() << "[Crawler] Mode changed to" << mode
             << (mode == 0 ? "(Battery Saver)" : mode == 1 ? "(Yellow/Lookahead)" : "(Red/Aggressive)")
             << "| Work queue:" << m_crawlWorkQueue.size() << "uncached files.";
    emit precacheModeChanged();
    emit crawlerProgressChanged();
  }
}

void ImageModel::onDirectoryChanged(const QString &path) {
  Q_UNUSED(path);
  if (m_isLoading || m_currentPath.isEmpty()) return;

  // BUG FIX C1: Was using 'static QTimer*' shared across ALL ImageModel instances.
  // In multi-window mode this caused the second window to fire callbacks into the
  // first window's destroyed ImageModel (use-after-free). Now uses per-instance member.
  m_debounceTimer.disconnect();
  connect(&m_debounceTimer, &QTimer::timeout, this, [this]() {
    if (!m_isLoading && !m_currentPath.isEmpty()) {
      qDebug() << "[DifferentialScan] Debounced live directory update for:" << m_currentPath;
      scanDirectory(m_currentPath);
    }
  });
  m_debounceTimer.start(500); // 500ms debounce
}

void ImageModel::reCrawl() {
  // Queue will be rebuilt after the next scanDirectory completes.
  {
    QMutexLocker lock(&m_crawlMutex); // FIX 2: guard queue clear
    m_crawlWorkQueue.clear();
  }
  m_crawlQueueIndex.store(0);
  m_crawledCount.store(0);
  m_crawlDbFull = false;
  m_crawlPassComplete = false;
  AsyncImageProvider::clearCache();
  emit crawlerProgressChanged();
  qDebug() << "[ImageModel] Crawler reset. Queue will be rebuilt on next scan completion.";
}

QVariantMap ImageModel::validateCacheCoverage() const {
  QReadLocker lock(&m_modelLock);
  int total = m_images.count();
  int cached = 0;
  const QSize sz(m_loadingResolution, m_loadingResolution);
  for (const auto &item : m_images) {
    if (AsyncImageProvider::checkCacheLevel(item.filePath, sz) != AsyncImageProvider::NotAvailable) {
      cached++;
    }
  }
  QVariantMap result;
  result["total"] = total;
  result["cached"] = cached;
  result["missing"] = total - cached;
  result["coveragePercent"] = total > 0 ? (double(cached) / total) * 100.0 : 100.0;
  return result;
}
