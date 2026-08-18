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

ImageModel::ImageModel(QObject *parent) : QAbstractListModel(parent) {
  m_precacheTimer = new QTimer(this);
  connect(m_precacheTimer, &QTimer::timeout, this, &ImageModel::processPrecacheTick);
  m_precacheTimer->start(100);

  connect(&m_folderWatcher, &QFileSystemWatcher::directoryChanged, this, &ImageModel::onDirectoryChanged);

  connect(&FileCacheManager::instance(), &FileCacheManager::cacheCleared, this, [this]() {
    m_crawlWorkQueue.clear();
    m_crawlQueueIndex.store(0);
    m_crawledCount.store(0);
    m_crawlPassComplete = false;
    emit crawlerProgressChanged();
  });
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

  // --- STEP 1: INSTANT CACHED LOAD FROM FOLDER DB (0ms UI Display) ---
  QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/folder_caches";
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
      instantCachedItems.append(info);
    }
    cacheFile.close();
  }

  if (!instantCachedItems.isEmpty()) {
    m_allItems = instantCachedItems;
    m_totalCount = m_allItems.size();
    m_scanProgress = m_allItems.size();
    m_scanMethod = "Folder DB (Instant Cached)";
    emit totalCountChanged();
    emit scanProgressChanged();
    emit scanMethodChanged();
    applyFilter();
    emit itemsPopulated(m_scanId);
    emit passOneCompleted(m_scanId);

    // Build crawler work queue and launch RAM promotion immediately at t=0ms
    int res = m_loadingResolution;
    QSize thumbSize(res, res);
    QList<QString> missing;
    missing.reserve(m_allItems.size());
    for (const auto &item : m_allItems) {
      if (!FileCacheManager::instance().isCached(item.filePath, thumbSize))
        missing.append(item.filePath);
    }
    m_crawlWorkQueue = std::move(missing);
    m_crawlQueueIndex.store(0);
    m_crawledCount.store(0);
    m_crawlPassComplete = m_crawlWorkQueue.isEmpty();
    m_crawlDbFull = false;
    emit crawlerProgressChanged();

    // Start background promotion of L2 disk mmap to L1 RAM immediately
    QThreadPool::globalInstance()->start([items = m_allItems, res]() {
      QSize sz(res, res);
      for (const auto &item : items) {
        AsyncImageProvider::promoteL2ToL1(item.filePath, sz);
      }
    });

    qDebug() << "[ImageModel] Loaded" << m_allItems.size() << "cached items instantly for:" << cleanPath
             << "(" << m_crawlWorkQueue.size() << "uncached)";
  } else {
    // Clear current images only if no cache exists (Cold Path)
    beginResetModel();
    m_allItems.clear();
    m_images.clear();
    endResetModel();
    m_totalCount = 0;
    m_scanProgress = 0;
    emit totalCountChanged();
    emit scanProgressChanged();
    m_crawlWorkQueue.clear();
    m_crawlQueueIndex.store(0);
    m_crawledCount.store(0);
    m_crawlPassComplete = false;
  }

  // Clear previous pending worker tasks (keep RAM cache warm)
  TaskScheduler::instance().clear();

  // Run live MFT / filesystem scanning via TaskScheduler
  TaskScheduler::instance().addTask(
      [this, cleanPath, currentGen, cachePath]() {
        QElapsedTimer timer;
        timer.start();

        if (!QDir(cleanPath).exists()) {
          qWarning() << "Directory does not exist:" << cleanPath;
          QMetaObject::invokeMethod(this, [this]() {
            m_isLoading = false;
            emit isLoadingChanged();
          });
          return;
        }

        qDebug() << "Scanning directory:" << cleanPath;

        QMetaObject::invokeMethod(this, [this, cleanPath]() {
          if (!m_folderWatcher.directories().isEmpty()) {
            m_folderWatcher.removePaths(m_folderWatcher.directories());
          }
          m_folderWatcher.addPath(cleanPath);
        });

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
              if (m_scanGeneration != currentGen) return;
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
                    fastItems.append(info);
                  }
                }
              }
            }
            fastScanSuccess = true;
            qDebug() << "[FastScanner] MFT populated" << fastItems.size() << "items in" << timer.elapsed() << "ms";
          }
        }

        // === Fallback to QDirIterator if MFT unavailable ===
        if (!fastScanSuccess) {
          QDirIterator it(cleanPath, nameFilters, QDir::Files, QDirIterator::Subdirectories);

          int localScanCount = 0;
          int nextBatchThreshold = 40;
          while (it.hasNext()) {
            if (m_scanGeneration != currentGen) {
                qDebug() << "[ImageModel] Scan cancelled for" << cleanPath;
                return;
            }
            it.next();

            QString filePath = it.filePath();
            // Filter out developer / package manager non-media folders
            if (filePath.contains("/node_modules/") || filePath.contains("\\node_modules\\") ||
                filePath.contains("/.git/") || filePath.contains("\\.git\\") ||
                filePath.contains("/.vscode/") || filePath.contains("\\.vscode\\") ||
                filePath.contains("/build/") || filePath.contains("\\build\\")) {
              continue;
            }

            localScanCount++;

            ImageInfo info;
            info.filePath = QDir::toNativeSeparators(filePath);
            info.fileName = it.fileName();
            info.size = 0; // Deferred
            info.date = QDateTime(); // Deferred if regex fails

            // Optimize: Try parsing filename for date (instant CPU task)
            QRegularExpressionMatch match = dateRegex.match(info.fileName);
            if (match.hasMatch()) {
              QString dateStr = match.captured(1) + match.captured(2);
              QDateTime dt = QDateTime::fromString(dateStr, "yyyyMMddHHmmss");
              if (dt.isValid())
                info.date = dt;
            }
            fastItems.append(info);

            // Stream items to UI as discovered
            if (localScanCount >= nextBatchThreshold) {
                if (nextBatchThreshold == 40) nextBatchThreshold = 150;
                else if (nextBatchThreshold == 150) nextBatchThreshold = 500;
                else if (nextBatchThreshold == 500) nextBatchThreshold = 1500;
                else if (nextBatchThreshold == 1500) nextBatchThreshold = 5000;
                else if (nextBatchThreshold == 5000) nextBatchThreshold = 15000;
                else nextBatchThreshold += 20000;

                QList<ImageInfo> batchItems = fastItems;
                QMetaObject::invokeMethod(this, [this, batchItems, localScanCount, currentGen]() {
                    if (m_scanGeneration != currentGen) return;
                    m_allItems = batchItems;
                    applyFilter();
                    m_scanProgress = localScanCount;
                    emit scanProgressChanged();
                    emit itemsPopulated(m_scanId);
                }, Qt::QueuedConnection);
            } else if (localScanCount % 200 == 0) {
                QMetaObject::invokeMethod(this, [this, localScanCount, currentGen]() {
                    if (m_scanGeneration != currentGen) return;
                    m_scanProgress = localScanCount;
                    emit scanProgressChanged();
                }, Qt::QueuedConnection);
            }
          }
        }

        QMetaObject::invokeMethod(this, [this, fastItems, currentGen]() {
            if (m_scanGeneration != currentGen) return;
            m_totalCount = fastItems.size();
            emit totalCountChanged();
        });

        // Universal Folder Metadata Database (both local & network)
        QHash<QString, QPair<qint64, QDateTime>> cachedData;
        QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/folder_caches";
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

        // Send skeleton grid to UI instantly
        QMetaObject::invokeMethod(this, [this, fastItems, currentGen, method, scanDuration]() {
          if (m_scanGeneration != currentGen) return;
          m_allItems = fastItems;
          m_scanMethod = method;
          m_scanDurationMs = scanDuration;
          emit scanMethodChanged();
          applyFilter();
          emit itemsPopulated(m_scanId);
          emit passOneCompleted(m_scanId);
        }, Qt::QueuedConnection);

        // === PASS 2: METADATA FILL ===
        bool cacheChanged = false;
        if (!fullyCached && !fastScanSuccess) {
          for (int i = 0; i < fastItems.size(); ++i) {
            if (m_scanGeneration != currentGen) return;

            while (TaskScheduler::instance().isPaused() && TaskScheduler::instance().isRunning()) {
              QThread::msleep(50);
            }
            if (!TaskScheduler::instance().isRunning()) break;

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
        if ((cacheChanged || !fullyCached || fastScanSuccess) && TaskScheduler::instance().isRunning() && m_scanGeneration == currentGen) {
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

        if (m_scanGeneration != currentGen) return;

        // Final perfect sort
        std::sort(fastItems.begin(), fastItems.end(),
                  [](const ImageInfo &a, const ImageInfo &b) {
                    return a.date > b.date;
                  });

        // Final UI commit & reconciliation
        QMetaObject::invokeMethod(this, [this, cleanPath, fastItems, timer, currentGen, fastScanSuccess, method]() {
          if (m_scanGeneration != currentGen) return;

          bool filesChanged = true;
          if (fastItems.size() == m_allItems.size()) {
            filesChanged = false;
            for (int i = 0; i < fastItems.size(); ++i) {
              if (fastItems[i].filePath != m_allItems[i].filePath) {
                filesChanged = true;
                break;
              }
            }
          }

          if (filesChanged) {
            m_allItems = fastItems;
            applyFilter();
            m_totalCount = m_allItems.size();
            m_scanProgress = m_allItems.size();
            emit totalCountChanged();
            emit scanProgressChanged();
            emit itemsPopulated(m_scanId);
          }

          m_isLoading = false;
          m_scanMethod = fastScanSuccess ? "Direct MFT (Verified)" : method;
          m_scanDurationMs = (int)timer.elapsed();
          emit isLoadingChanged();
          emit scanMethodChanged();

          qDebug() << "[ImageModel] Verified directory scan in" << timer.elapsed() << "ms. Total items:" << m_allItems.size();

          // Build precise work queue: ask the mmap index which files are genuinely absent.
          // This is O(N) hash lookups with zero disk I/O — ground truth, no cursor math.
          int res = m_loadingResolution;
          QSize thumbSize(res, res);
          QList<QString> missing;
          missing.reserve(fastItems.size());
          for (const auto &item : fastItems) {
            if (!FileCacheManager::instance().isCached(item.filePath, thumbSize))
              missing.append(item.filePath);
          }
          m_crawlWorkQueue = std::move(missing);
          m_crawlQueueIndex.store(0);
          m_crawledCount.store(0);
          m_crawlPassComplete = m_crawlWorkQueue.isEmpty();
          m_crawlDbFull = false;
          qDebug() << "[Crawler] Work queue ready:" << m_crawlWorkQueue.size()
                   << "uncached /" << fastItems.size() << "total.";
          emit crawlerProgressChanged();

          // Reconcile DB against actual filesystem:
          // Only prune entries in cleanPath whose source files were deleted
          QThreadPool::globalInstance()->start([cleanPath, fastItems, res]() {
            QSet<QString> validPaths;
            validPaths.reserve(fastItems.size());
            for (const auto &item : fastItems)
              validPaths.insert(item.filePath);
            FileCacheManager::instance().pruneStaleEntries(cleanPath, validPaths, QSize(res, res));
          });

          // Promote already-cached files into L1 RAM simultaneously
          QThreadPool::globalInstance()->start([fastItems, res]() {
            QSize sz(res, res);
            qDebug() << "[Promotion] Starting background L2 -> L1 RAM promotion for" << fastItems.size() << "items...";
            int count = 0;
            for (const auto &item : fastItems) {
              AsyncImageProvider::promoteL2ToL1(item.filePath, sz);
              count++;
            }
            qDebug() << "[Promotion] Finished background RAM promotion for" << count << "items.";
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

  // If the drive is congested (>150ms spike), inject a breather delay to yield bus to UI
  static QElapsedTimer s_crawlerThrottleTimer;
  if (throttleDelay > 0) {
    if (!s_crawlerThrottleTimer.isValid()) {
      s_crawlerThrottleTimer.start();
    } else if (s_crawlerThrottleTimer.elapsed() < throttleDelay) {
      return; // Breather delay active: 100% bandwidth given to UI
    }
  }
  s_crawlerThrottleTimer.restart();

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
          AsyncImageProvider::promoteL2ToL1(path, thumbSize);
        }
        continue;
      }
      m_crawlInflight.fetch_add(1, std::memory_order_relaxed);
      QSize sz = thumbSize;
      bool added = TaskScheduler::instance().addTask([this, path, sz, currentGen]() {
        if (this->m_scanGeneration.load() != currentGen) {
          this->m_crawlInflight.fetch_sub(1, std::memory_order_relaxed);
          return;
        }
        AsyncImageProvider::crawlDecodeToL2(path, sz);
        this->m_crawlInflight.fetch_sub(1, std::memory_order_relaxed);
      }, TaskScheduler::CPU_BOUND, TaskScheduler::Normal);

      if (!added) {
        m_crawlInflight.fetch_sub(1, std::memory_order_relaxed);
        break;
      }
    }
    return;
  }

  // ----------------------------------------------------------------
  // Mode 2 (Red): Aggressive full-model sequential background crawler.
  // Routed at Low priority via TaskScheduler with starvation protection.
  // ----------------------------------------------------------------
  // ----------------------------------------------------------------
  // Mode 2 (Red): Drain the mmap-backed work queue.
  // m_crawlWorkQueue was built at scan completion: it contains ONLY the
  // files provably absent from the database. No cursor drift possible.
  // ----------------------------------------------------------------
  if (m_precacheMode == 2) {
    int queueSize = m_crawlWorkQueue.size();
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

      const QString path = m_crawlWorkQueue[i];

      m_crawlInflight.fetch_add(1, std::memory_order_relaxed);
      QSize sz = thumbSize;
      bool added = TaskScheduler::instance().addTask([this, path, sz, currentGen]() {
        if (this->m_scanGeneration.load() != currentGen) {
          this->m_crawlInflight.fetch_sub(1, std::memory_order_relaxed);
          return;
        }
        AsyncImageProvider::crawlDecodeToL2(path, sz);
        this->m_crawledCount.fetch_add(1, std::memory_order_relaxed);
        this->m_crawlInflight.fetch_sub(1, std::memory_order_relaxed);
        emit this->crawlerProgressChanged();
      }, TaskScheduler::CPU_BOUND, TaskScheduler::Low);

      if (!added) {
        // Task rejected — back off the index so this slot is retried next tick
        m_crawlQueueIndex.fetch_sub(1, std::memory_order_relaxed);
        m_crawlInflight.fetch_sub(1, std::memory_order_relaxed);
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

  // Single-shot debounced scan to prevent recursive re-scans during batch file drops
  static QTimer *debounceTimer = nullptr;
  if (!debounceTimer) {
    debounceTimer = new QTimer(this);
    debounceTimer->setSingleShot(true);
  }
  debounceTimer->disconnect();
  connect(debounceTimer, &QTimer::timeout, this, [this]() {
    if (!m_isLoading && !m_currentPath.isEmpty()) {
      qDebug() << "[DifferentialScan] Debounced live directory update for:" << m_currentPath;
      scanDirectory(m_currentPath);
    }
  });
  debounceTimer->start(500); // 500ms debounce
}

void ImageModel::reCrawl() {
  // Queue will be rebuilt after the next scanDirectory completes.
  m_crawlWorkQueue.clear();
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
