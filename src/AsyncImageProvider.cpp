#include "AsyncImageProvider.h"
#include "DesktopHelper.h"
#include "FrameBudgetScheduler.h"
#include "SystemMonitor.h"
#include "TaskScheduler.h"
#include "VideoThumbnailer.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFileIconProvider>
#include <QIcon>
#include <QImageReader>
#include <QMutex>
#include <QMutexLocker>
#include <QPainter>
#include <QSet>
#include <QSettings>
#include <QStandardPaths>
#include <QThread>
#include <QUrl>
#include <QVariant>
#include <QtCore/QEventLoop>
#include <QtCore/QSemaphore>
#include <QtCore/QTimer>
#include <QtMultimedia/QAudioOutput>
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>
#include <algorithm>
#include <cstdio>
#include <libraw/libraw.h>

#ifdef Q_OS_WIN
#include <objbase.h>
#endif

// Initialize static members
static QSemaphore s_videoSemaphore(
    2); // Increased to 2 for better throughput, but shielded by TDR logic
static QSemaphore
    s_rawSemaphore(3); // Increased to 3 to improve high-res loading speed
QCache<QString, QImage> AsyncImageProvider::m_cache;
QMutex AsyncImageProvider::m_mutex;
QMap<QString, QList<std::shared_ptr<ResponseTracker>>>
    AsyncImageProvider::m_pendingResponses;
QMutex AsyncImageProvider::m_pendingMutex;

// Initialize statics
std::atomic<int> AsyncImageProvider::s_logLevel(1);
std::atomic<bool> AsyncImageProvider::s_accelerateRaw(true);
std::atomic<bool> AsyncImageProvider::s_disableVideo{false};
std::atomic<bool> AsyncImageProvider::s_disableRaw{false};
std::atomic<bool> AsyncImageProvider::s_useDiskCache{false};
std::atomic<int> AsyncImageProvider::s_videoAcceleration{0}; // 0 = Auto/Default
FrameBudgetScheduler *AsyncImageProvider::s_frameScheduler = nullptr;

// Composite key for coalescing: ensures that a request for a 200x200 thumb
// does not get fulfilled by a 10x10 preview task for the same file.
static QString getCoalesceKey(const QString &normalizedId, const QSize &size) {
  return QString("%1_%2x%3")
      .arg(normalizedId)
      .arg(size.width())
      .arg(size.height());
}

static QString normalizeId(const QString &id) {
  if (id.startsWith("synthetic:"))
    return id;

  QString input = id;
  if (input.startsWith("image://async/"))
    input = input.mid(14);

  // Use QUrl for robust parsing of file URLs and UNC paths
  QUrl url(input);
  QString path;
  QString query;

  if (url.scheme() == "file" || url.isLocalFile()) {
    path = url.toLocalFile();
    // Preserve query parameters for cache-busting/coalescing
    int qIdx = input.indexOf('?');
    if (qIdx != -1)
      query = input.mid(qIdx);
  } else {
    // Fallback for raw paths or missing local file metadata
    path = input;
    int qIdx = path.indexOf('?');
    if (qIdx != -1) {
      query = path.mid(qIdx);
      path = path.left(qIdx);
    }

    if (path.startsWith("file:///"))
      path = path.mid(8);
    else if (path.startsWith("file://"))
      path = path.mid(7);

    path = QUrl::fromPercentEncoding(path.toUtf8());
  }

  path = QDir::cleanPath(path);

#ifdef Q_OS_WIN
  if (path.length() >= 2 && path[1] == ':') {
    path[0] = path[0].toUpper();
  }
#endif

  return path + query;
}

static QString getRealLocalPath(const QString &normalizedId) {
  QString p = normalizedId;
  int qIdx = p.indexOf('?');
  if (qIdx != -1)
    p = p.left(qIdx);

  // If it still has a protocol, it's not a local file
  if (p.contains("://"))
    return QString();
  return p;
}

static QString getDiskCachePath(const QString &id, const QSize &size) {
  QString cacheDir =
      QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
      "/thumbnails";
  static bool dirCreated = false;
  if (!dirCreated) {
    QDir().mkpath(cacheDir);
    dirCreated = true;
  }

  QString nId = normalizeId(id);
  QString localP = getRealLocalPath(nId);
  QFileInfo fi(localP);

  // Hash includes LastModified to invalidate cache when file changes!
  QString key =
      nId + "_" + QString::number(fi.lastModified().toMSecsSinceEpoch()) + "_" +
      QString::number(size.width()) + "x" + QString::number(size.height());

  QString hash =
      QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Md5).toHex();
  return cacheDir + "/" + fi.baseName() + "_" + QString::number(size.width()) +
         "x" + QString::number(size.height()) + ".jpg";
}

int AsyncImageProvider::getTaskWeight(const QString &id) {
  QString path = normalizeId(id);
  QString ext = QFileInfo(path).suffix().toLower();

  // HEAVY weighting for compute-intensive decodes
  if (ext == "arw" || ext == "cr2" || ext == "dng" || ext == "nef" ||
      ext == "sr2" || ext == "srf" || ext == "orf" || ext == "rw2" ||
      ext == "pef" || ext == "raf") {
    return 4; // RAW/DNG decodes are heavy on CPU
  }

  if (ext == "mp4" || ext == "mov" || ext == "mkv" || ext == "avi" ||
      ext == "wmv") {
    return 4; // Video thumbnailing is heavy on CPU/Video engine
  }

  return 1; // Standard JPEGs/PNGs
}

QMap<QString, AsyncImageProvider::DriveStats> AsyncImageProvider::m_driveStats;
QMutex AsyncImageProvider::m_driveStatsMutex;

// Debugging for stalls
// Debugging for stalls
static QMap<QString, qint64> s_activeTasksMap;
static QMutex s_activeTasksMutex;

QStringList AsyncImageProvider::getActiveTaskIds() {
  QMutexLocker lock(&s_activeTasksMutex);
  return s_activeTasksMap.keys();
}

QString AsyncImageProvider::getDriveRoot(const QString &id) {
  // Normalize first so we are looking at the same path format as adaptive
  // stats
  QString path = normalizeId(id);

  if (path.length() >= 2 && path[1] == ':') {
    return path.left(2).toUpper() + "/";
  }
  if (path.startsWith("//") || path.startsWith("\\\\")) {
    int shareEnd = path.indexOf('/', 2);
    if (shareEnd == -1)
      shareEnd = path.indexOf('\\', 2);
    if (shareEnd != -1)
      return path.left(shareEnd);
    return path;
  }
  return "ROOT";
}

void AsyncImageProvider::checkStalls() {
  QMutexLocker lock(&s_activeTasksMutex);
  qint64 now = QDateTime::currentMSecsSinceEpoch();
  int rescueCount = 0;

  auto i = s_activeTasksMap.begin();
  while (i != s_activeTasksMap.end()) {
    if (now - i.value() > 15000) { // 15 seconds
      qWarning() << "[AsyncImageProvider] STALL DETECTED for:" << i.key()
                 << "Duration:" << (now - i.value()) << "ms";

      QString drive = getDriveRoot(i.key());
      {
        QMutexLocker statsLock(&m_driveStatsMutex);
        if (m_driveStats.contains(drive)) {
          int weight = getTaskWeight(i.key());
          m_driveStats[drive].activeWeight =
              qMax(0, m_driveStats[drive].activeWeight - weight);
          qInfo() << "[AsyncImageProvider] Forcing slots free for drive:"
                  << drive << "Weight recovered:" << weight
                  << "New Active Weight:" << m_driveStats[drive].activeWeight;
        }
      }

      i = s_activeTasksMap.erase(i);
      rescueCount++;
    } else {
      ++i;
    }
  }

  if (rescueCount > 0) {
    // TaskScheduler::instance().expandIOPool(rescueCount);
    qInfo()
        << "[AsyncImageProvider] Skipped pool expansion (stability check) for"
        << rescueCount << "stalled tasks.";
  }
}

void AsyncImageProvider::DriveStats::update(const QString &drive,
                                            int loadTimeMs) {
  if (sampleCount == 0) {
    avgLoadTimeMs = loadTimeMs;
  } else {
    // Proportional alpha: heavier weighting for slow tasks (spikes)
    double alpha = (loadTimeMs > 500) ? 0.4 : 0.1;
    avgLoadTimeMs = (avgLoadTimeMs * (1.0 - alpha)) + (loadTimeMs * alpha);
  }
  sampleCount++;

  // Tapered Logging: High resolution for first 100 samples, then 1-in-50
  // overhead.
  if (sampleCount <= 100 || sampleCount % 50 == 0) {
    qWarning() << "[AdaptiveIO] Stats: " << drive << " Avg: " << avgLoadTimeMs
               << " Limit: " << concurrencyLimit
               << " ActiveWeight: " << activeWeight;
  }

  if (lastAdjustment.isValid() && lastAdjustment.elapsed() < 1000)
    return;

  // Initialize network detection once per drive root
  if (!initialized) {
#ifdef Q_OS_WIN
    std::string driveStr = drive.toStdString();
    UINT type = GetDriveTypeA(driveStr.c_str());
    if (type == DRIVE_REMOTE || drive.startsWith("//") ||
        drive.startsWith("\\\\")) {
      isNetwork = true;
    }
#else
    if (drive.startsWith("//") || drive.startsWith("\\\\")) {
      isNetwork = true;
    }
#endif
    if (isNetwork) {
      concurrencyLimit = 8; // Higher starting point for network
      qWarning() << "[AdaptiveIO] Network Drive detected: " << drive
                 << " using burst mode.";
    }
    initialized = true;
  }

  int oldLimit = concurrencyLimit;

  // Refined Sliding Scale Bands (Optimized for High-Throughput SSD/Network)
  int minLimit = isNetwork ? 6 : 4; // Local drives start bit higher now
  int maxLimit =
      isNetwork ? 20 : 16; // Boost local ceiling to closer to network

  if (avgLoadTimeMs > 2000) {
    concurrencyLimit = minLimit;
  } else if (avgLoadTimeMs > 800) {
    if (concurrencyLimit > minLimit)
      concurrencyLimit--;
  } else if (avgLoadTimeMs < 100) {
    if (concurrencyLimit < maxLimit)
      concurrencyLimit += isNetwork ? 4 : 2; // Faster ramp up for network
  } else if (avgLoadTimeMs < 250) {
    if (concurrencyLimit < maxLimit - 2)
      concurrencyLimit += isNetwork ? 2 : 1;
  }

  // CPU AWARE BACKOFF: If system is pinned, drastically cut limits
  if (auto *mon = SystemMonitor::instance()) {
    double cpu = mon->systemCpuUsage();
    if (cpu > 85.0) {
      concurrencyLimit = qMax(minLimit, concurrencyLimit / 2);
    } else if (cpu > 70.0) {
      concurrencyLimit = qMax(minLimit, (int)(concurrencyLimit * 0.75));
    }
  }

  if (oldLimit != concurrencyLimit) {
    if (s_logLevel > 0) {
      qWarning() << "[AdaptiveIO] TUNING: " << drive << " Limit " << oldLimit
                 << " -> " << concurrencyLimit << " (Latency: " << avgLoadTimeMs
                 << "ms)";
    }
    lastAdjustment.restart();
  }
}

AsyncImageProvider::AsyncImageProvider() : QQuickAsyncImageProvider() {
  s_disableVideo = false;
  s_disableRaw = false;
}

void AsyncImageProvider::setFrameScheduler(FrameBudgetScheduler *s) {
  s_frameScheduler = s;
}

AsyncImageResponse::~AsyncImageResponse() {
  if (m_tracker) {
    m_tracker->response.store(nullptr);
  }
}

AsyncImageResponse::AsyncImageResponse(const QString &id,
                                       const QSize &requestedSize)
    : m_id(id), m_requestedSize(requestedSize),
      m_cancelled(std::make_shared<std::atomic<bool>>(false)),
      m_tracker(std::make_shared<ResponseTracker>(this)) {
  static bool configured = false;
  if (!configured) {
    QSettings settings("SamsungClone", "Gallery");
    int cacheSizeMB = settings.value("cacheSizeMB", 512).toInt();
    AsyncImageProvider::setCacheMaxCost(qBound(128, cacheSizeMB, 4096) * 1024);
    configured = true;
  }
}

QQuickTextureFactory *AsyncImageResponse::textureFactory() const {
  return QQuickTextureFactory::textureFactoryForImage(m_image);
}

void AsyncImageResponse::cancel() { *m_cancelled = true; }
void AsyncImageResponse::run() {}

void AsyncImageResponse::handleDone(QImage image, int duration) {
  if (m_cancelled->load())
    return;

  if (AsyncImageProvider::s_logLevel > 0) {
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qWarning() << "[" << timeStr << "][TRACE] handleDone for" << this
               << "ID:" << m_id << "(null=" << image.isNull() << ")";
  }

  m_workDuration = duration;

  auto finishTask = [this, image]() {
    m_image = image;
    emit finished();
  };

  if (AsyncImageProvider::s_frameScheduler) {
    auto scheduler = AsyncImageProvider::s_frameScheduler;
    QMetaObject::invokeMethod(
        scheduler,
        [scheduler, finishTask]() { scheduler->onTaskCompleted(finishTask); },
        Qt::QueuedConnection);
  } else {
    QMetaObject::invokeMethod(this, finishTask, Qt::QueuedConnection);
  }
}

bool AsyncImageProvider::isRequestStillNeeded(const QString &coalesceKey) {
  QMutexLocker locker(&m_pendingMutex);
  if (!m_pendingResponses.contains(coalesceKey))
    return false;
  for (const auto &tracker : m_pendingResponses[coalesceKey]) {
    if (tracker && tracker->response.load()) {
      auto *resp = tracker->response.load();
      if (resp && !resp->m_cancelled->load())
        return true;
    }
  }
  return false;
}

void AsyncImageProvider::deliverToPending(const QString &coalesceKey,
                                          const QImage &image, int duration) {
  QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
  QMutexLocker locker(&m_pendingMutex);
  QList<std::shared_ptr<ResponseTracker>> listeners =
      m_pendingResponses.take(coalesceKey);
  locker.unlock();

  if (s_logLevel > 0) {
    qWarning() << "[" << timeStr << "][TRACE] Delivering result for"
               << coalesceKey << "to" << listeners.size() << "listeners (took"
               << duration << "ms)";
  }

  for (const auto &tracker : listeners) {
    auto *resp = tracker->response.load(); // THREAD-SAFE LOAD
    if (resp) {
      QMetaObject::invokeMethod(resp, "handleDone", Qt::QueuedConnection,
                                Q_ARG(QImage, image), Q_ARG(int, duration));
    }
  }

  // Trigger next batch of staged requests since a slot has opened up
  scheduleStagingProcessing();
}

#include "VisibleRangeManager.h"

QList<AsyncImageProvider::StagedRequest> AsyncImageProvider::m_stagedRequests;
QMutex AsyncImageProvider::m_stagingMutex;

int AsyncImageProvider::stagedRequestCount() {
  QMutexLocker lock(&m_stagingMutex);
  return m_stagedRequests.size();
}

QQuickImageResponse *
AsyncImageProvider::requestImageResponse(const QString &id,
                                         const QSize &requestedSize) {
  if (s_logLevel > 0) {
    qDebug() << "[AsyncImageProvider] REQUEST:" << id
             << (requestedSize.isValid() ? QString("%1x%2")
                                               .arg(requestedSize.width())
                                               .arg(requestedSize.height())
                                         : "Original");
  }

  QString normalizedId = normalizeId(id);
  QString cKey = getCoalesceKey(normalizedId, requestedSize);
  auto *response = new AsyncImageResponse(normalizedId, requestedSize);

  // 1. RAM Cache check (Instant)
  QImage cached = getCachedImage(normalizedId, requestedSize);
  if (!cached.isNull()) {
    QMetaObject::invokeMethod(response, "handleDone", Qt::QueuedConnection,
                              Q_ARG(QImage, cached), Q_ARG(int, 0));
    return response;
  }

  // Disk Cache check: Moved to Worker thread (processImageTask) to avoid
  // blocking GUI thread on slow drives (like SD cards).
  {
    QMutexLocker locker(&m_pendingMutex);
    m_pendingResponses[cKey].append(response->m_tracker);
  }

  if (s_logLevel > 0) {
    qDebug() << "[AsyncImageProvider] REQUEST:" << normalizedId
             << "Key:" << cKey;
  }

  queueRequest(normalizedId, requestedSize, response->m_cancelled,
               response->m_tracker);
  return response;
}

void AsyncImageProvider::queueRequest(const QString &id, const QSize &size,
                                      std::shared_ptr<std::atomic<bool>> c,
                                      std::shared_ptr<ResponseTracker> t) {
  QMutexLocker lock(&m_stagingMutex);
  m_stagedRequests.append({id, size, QDateTime::currentDateTime(), c, t});
  scheduleStagingProcessing();
}

void AsyncImageProvider::scheduleStagingProcessing() {
  static std::atomic<bool> scheduled{false};
  if (scheduled.exchange(true))
    return;

  // Use invokeMethod to ensure this runs on the GUI thread (where QTimer
  // works) or at least in a thread with an event loop.
  QMetaObject::invokeMethod(
      qApp,
      []() {
        QTimer::singleShot(2, []() {
          scheduled.store(false);
          AsyncImageProvider::processStagedRequests();
        });
      },
      Qt::QueuedConnection);
}

void AsyncImageProvider::processStagedRequests() {
  QList<StagedRequest> batch;
  {
    QMutexLocker lock(&m_stagingMutex);
    if (m_stagedRequests.isEmpty())
      return;
    batch = m_stagedRequests;
    m_stagedRequests.clear();
  }

  bool lowMemory = false;
  if (auto *mon = SystemMonitor::instance()) {
    double freeMem = mon->availableSystemMemoryMB();
    if (freeMem > 0 && freeMem < 1000.0)
      lowMemory = true;
  }

  QList<StagedRequest> visibleHighPriority;
  QList<StagedRequest> offscreen;
  auto &vrm = VisibleRangeManager::instance();

  // ADAPTIVE ADMISSION CONTROL
  QMap<QString, int>
      driveAllocations; // How many new tasks per drive this cycle

  // Sort batch so we process newest/most important first for admission check
  std::sort(batch.begin(), batch.end(),
            [](const StagedRequest &a, const StagedRequest &b) {
              return a.timestamp > b.timestamp; // DESC timestamp = newest first
            });

  for (const auto &req : batch) {
    QString cKey = getCoalesceKey(req.id, req.requestedSize);
    if (!isRequestStillNeeded(cKey))
      continue;

    QString drive = getDriveRoot(req.id);
    QMutexLocker statsLock(&m_driveStatsMutex);
    DriveStats &stats = m_driveStats[drive];
    if (stats.lastAdjustment.isValid() == false)
      stats.lastAdjustment.start(); // Init timer

    // If visible, we prioritize it, but we still respect a hard ceiling to
    // avoid death? Actually, for visible, we should probably bypass limit if
    // it's small, but the user asked to "back off". Let's count active tasks
    // + allocated this cycle

    int weight = getTaskWeight(req.id);
    int currentWeight = stats.activeWeight + driveAllocations.value(drive, 0);

    bool isVisible = vrm.isPathVisible(req.id);

    // Base limit (conservative)
    int limit =
        stats.isNetwork ? (stats.concurrencyLimit + 4) : stats.concurrencyLimit;

    bool canAdmit = false;
    if (isVisible) {
      // Visible items can use BURST capacity (e.g. +12 for network, +6 for
      // local)
      int burst = stats.isNetwork ? 12 : 6;
      if (currentWeight + weight <= limit + burst)
        canAdmit = true;
    } else {
      // Offscreen items strictly respect the base limit
      if (currentWeight + weight <= limit)
        canAdmit = true;
    }

    // ADMISSION DEBUGGING (Max log level only)
    if (isVisible && !canAdmit && s_logLevel > 0) {
      qDebug() << "[Admission] Deferred visible item due to load:" << req.id
               << "Weight:" << weight << "CurrentWeight:" << currentWeight
               << "Limit:" << stats.concurrencyLimit << " (+ burst)";
    }

    if (canAdmit) {
      if (isVisible)
        visibleHighPriority.append(req);
      else if (!lowMemory)
        offscreen.append(req);
      // Only deliver empty for offscreen items when OOM
      else
        deliverToPending(req.id, QImage(), 0);

      driveAllocations[drive] += weight;
    } else {
      // Re-queue everything else
      QMutexLocker lock(&m_stagingMutex);
      m_stagedRequests.append(req); // Will be retried next cycle
    }
  }

  // Commit allocations to Real Active Tasks as we dispatch
  auto incrementActive = [&](const QString &id) {
    QString d = getDriveRoot(id);
    int weight = getTaskWeight(id);
    QMutexLocker lock(&m_driveStatsMutex);
    m_driveStats[d].activeWeight += weight;
  };

  // Sort ASC (Oldest First).
  // Because TaskScheduler::addTask uses Prepend(), adding Oldest->Newest
  // results in [Newest, ..., Oldest] in the queue (LIFO).
  std::sort(visibleHighPriority.begin(), visibleHighPriority.end(),
            [](const StagedRequest &a, const StagedRequest &b) {
              return a.timestamp < b.timestamp;
            });

  // Limit high-priority dispatch to prevent UI thread flooding
  int dispatched = 0;
  QList<StagedRequest> deferredVisible;

  // Submit Visible - Now filtered by Admission Control
  for (const auto &req : visibleHighPriority) {
    incrementActive(req.id);
    TaskScheduler::instance().addTask(
        [req]() {
          AsyncImageProvider::processImageTask(req.id, req.requestedSize,
                                               req.cancelled, req.tracker);
        },
        TaskScheduler::IO_BOUND, TaskScheduler::Immediate);
  }

  // Submit Offscreen
  for (const auto &req : offscreen) {
    incrementActive(req.id);
    TaskScheduler::instance().addTask(
        [req]() {
          AsyncImageProvider::processImageTask(req.id, req.requestedSize,
                                               req.cancelled, req.tracker);
        },
        TaskScheduler::IO_BOUND, TaskScheduler::Normal);
  }

  if (!m_stagedRequests.isEmpty())
    scheduleStagingProcessing();
}
void AsyncImageProvider::processImageTaskInternal(
    QString id, QSize requestedSize, std::shared_ptr<std::atomic<bool>> c,
    std::shared_ptr<ResponseTracker> t) {

  QString cKey = getCoalesceKey(id, requestedSize);

  auto shouldAbort = [&]() {
    if (c && c->load())
      return true;
    if (!isRequestStillNeeded(cKey))
      return true;
    if (TaskScheduler::instance().isPaused())
      return true;

    // Only abort if the system is totally overwhelmed (>5k tasks)
    if (TaskScheduler::instance().activeTaskCount() > 5000)
      return true;

    return false;
  };

  if (s_logLevel > 0) {
    if (!isRequestStillNeeded(cKey)) {
      QMutexLocker locker(&m_pendingMutex);
      m_pendingResponses.remove(cKey);
      return;
    }

    // If paused, abort immediately (faster than waiting)
    if (TaskScheduler::instance().activeTaskCount() > 0 &&
        TaskScheduler::instance().activeTaskCount() % 10 == 0) {
      // Just a throttle check
    }

    qDebug() << "[AsyncImageProvider] High workload detect. Queue:"
             << TaskScheduler::instance().activeTaskCount();
  }

  // RAII Guard to ensure we always decrement activeTasks, even on early
  // return
  struct DriveConcurrencyGuard {
    QString drive;
    QString taskId;
    QElapsedTimer timer;
    bool committed = false;

    DriveConcurrencyGuard(const QString &id) {
      drive = getDriveRoot(id);
      taskId = id;
      timer.start();
      QMutexLocker lock(&s_activeTasksMutex);
      s_activeTasksMap.insert(taskId, QDateTime::currentMSecsSinceEpoch());
    }

    ~DriveConcurrencyGuard() {
      {
        QMutexLocker lock(&s_activeTasksMutex);
        s_activeTasksMap.remove(taskId);
      }
      {
        QMutexLocker lock(&m_driveStatsMutex);
        int weight = getTaskWeight(taskId);
        m_driveStats[drive].activeWeight =
            qMax(0, m_driveStats[drive].activeWeight - weight);
      }
      // Trigger pump to pick up next task
      AsyncImageProvider::processStagedRequests();
    }

    void commitStats() {
      QMutexLocker lock(&m_driveStatsMutex);
      m_driveStats[drive].update(drive, timer.elapsed());
      committed = true;
    }
  };
  DriveConcurrencyGuard driveGuard(id);

  QElapsedTimer workTimer;
  workTimer.start();

  QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");

#ifdef Q_OS_WIN
  CoInitialize(NULL);
#endif

  QString path;
  path = getRealLocalPath(id);
  if (path.isEmpty())
    path = id; // Fallback for synthetic/remote

  QImage image;

  if (id.startsWith("synthetic:")) {
    int index = 0;
    QRegularExpression re("test_image_([0-9]+)");
    auto match = re.match(id);
    if (match.hasMatch())
      index = match.captured(1).toInt();

    int w = requestedSize.width() > 0 ? requestedSize.width() : 400;
    int h = requestedSize.height() > 0 ? requestedSize.height() : 400;
    image = QImage(w, h, QImage::Format_RGB32);
    image.fill(QColor::fromHsv((index * 137) % 360, 150, 200));
    QPainter p(&image);
    p.setPen(Qt::white);
    p.setFont(QFont("Arial", 20, QFont::Bold));
    p.drawText(image.rect(), Qt::AlignCenter, QString::number(index));
  } else {
    // Strip query parameters for file access (but 'id' remains the cache key)
    int queryIdx = path.lastIndexOf('?');
    if (queryIdx != -1) {
      path = path.left(queryIdx);
    }

    // IO Serializer REMOVED - Replaced by Adaptive throttling in
    // processStagedRequests

    QElapsedTimer ioTimer;
    ioTimer.start();

    // 1. Worker-side Disk Cache check (Optimized hit)
    if (s_useDiskCache && !id.startsWith("synthetic:")) {
      QString dp = getDiskCachePath(id, requestedSize);
      if (QFile::exists(dp)) {
        if (image.load(dp)) {
          // Update RAM cache with consistent key
          insertCachedImage(id, image, requestedSize);
          // Disk Cache is also I/O!
          driveGuard.commitStats();
          deliverToPending(cKey, image, workTimer.elapsed());
          return;
        }
      }
    }

    DesktopHelper::FileType type = DesktopHelper::staticGetFileType(path);
    if (type == DesktopHelper::Raw) {
      // Hardware Throttle: If we're offscreen and busy, DON'T even try to
      // decode RAW. RAW decoding is too expensive to do speculatively on slow
      // SD cards.
      // NOTE: We check 'path' (cleaned) for visibility, not 'id' (with
      // queries)
      if (!VisibleRangeManager::instance().isPathVisible(path) &&
          TaskScheduler::instance().activeTaskCount() > 30) {
        goto deliver; // FINISH with null instead of hanging
      }

      s_rawSemaphore.acquire(1);
      QSemaphoreReleaser releaser(s_rawSemaphore);
      if (!shouldAbort()) {
        try {
          // ... (libraw remains same)
          LibRaw RawProcessor;
          if (RawProcessor.open_file(path.toLocal8Bit().constData()) ==
              LIBRAW_SUCCESS) {
            bool loaded = false;
            if (s_accelerateRaw &&
                RawProcessor.unpack_thumb() == LIBRAW_SUCCESS) {
              libraw_processed_image_t *thumb =
                  RawProcessor.dcraw_make_mem_thumb();
              if (thumb) {
                if (thumb->type == LIBRAW_IMAGE_JPEG)
                  image.loadFromData((const uchar *)thumb->data,
                                     thumb->data_size, "JPEG");
                LibRaw::dcraw_clear_mem(thumb);
                loaded = !image.isNull();
              }
            }
            if (!loaded && !shouldAbort() &&
                RawProcessor.unpack() == LIBRAW_SUCCESS) {
              RawProcessor.dcraw_process();
              libraw_processed_image_t *full =
                  RawProcessor.dcraw_make_mem_image();
              if (full) {
                QImage img((const uchar *)full->data, full->width, full->height,
                           full->width * 3, QImage::Format_RGB888);
                image = img.copy();
                LibRaw::dcraw_clear_mem(full);
              }
            }
          }
        } catch (...) {
        }
      }
    } else if (type == DesktopHelper::Video) {
      s_videoSemaphore.acquire(1);
      QSemaphoreReleaser releaser(s_videoSemaphore);
      if (!shouldAbort()) {
        VideoThumbnailer v;
        SettingsHelper::HWAccel accel =
            static_cast<SettingsHelper::HWAccel>(s_videoAcceleration.load());
        image = v.extractFrame(path, 0, requestedSize, accel,
                               c ? c.get() : nullptr);
      }
    } else {
      QImageReader reader(path);
      if (requestedSize.isValid())
        reader.setScaledSize(requestedSize);
      if (reader.canRead())
        image = reader.read();
    }
  }

deliver:
  // DEBUG: If image failed to load, return a Red placeholder so we know it's
  // not just "stuck"
  if (image.isNull() && !shouldAbort()) {
    QString realPath = getRealLocalPath(id);
    bool exists = QFile::exists(realPath);
    QString readerError;
    if (exists) {
      QImageReader reader(realPath);
      reader.canRead();
      readerError = reader.errorString();
    }

    if (s_logLevel > 0) {
      qWarning() << "[AsyncImageProvider] FAILED load. ID:" << id
                 << "Path:" << realPath << "Exists:" << exists
                 << "ReaderError:" << readerError;
    }

    image = QImage(requestedSize.isValid() ? requestedSize : QSize(100, 100),
                   QImage::Format_RGB32);
    image.fill(Qt::red);
  }

  if (!image.isNull() && requestedSize.isValid() &&
      (image.width() > requestedSize.width() ||
       image.height() > requestedSize.height())) {
    image = image.scaled(requestedSize, Qt::KeepAspectRatio,
                         Qt::SmoothTransformation);
  }

#ifdef Q_OS_WIN
  CoUninitialize();
#endif

  if (!image.isNull()) {
    insertCachedImage(id, image, requestedSize);
    if (s_useDiskCache && !id.startsWith("synthetic:")) {
      image.save(getDiskCachePath(id, requestedSize), "JPG", 80);
    }
  }

  int duration = workTimer.elapsed();

  if (s_logLevel > 0) {
    qDebug() << "[AdaptiveIO] DONE:" << id << "(null=" << image.isNull()
             << ") took" << duration << "ms Key:" << cKey;
  }

  // UPDATE ADAPTIVE STATS (if we actually did I/O)
  if (!id.startsWith("synthetic:") && !id.startsWith("image://")) {
    driveGuard.commitStats();
  }

  if (duration > 500) {
    qWarning() << "[Performance] SLOW LOAD detected:" << duration << "ms for"
               << id;
  }

  deliverToPending(cKey, image, duration);
}

void AsyncImageProvider::processImageTask(QString id, QSize requestedSize,
                                          std::shared_ptr<std::atomic<bool>> c,
                                          std::shared_ptr<ResponseTracker> t) {
  processImageTaskInternal(id, requestedSize, c, t);
}

QImage AsyncImageProvider::getCachedImage(const QString &id,
                                          const QSize &size) {
  QMutexLocker locker(&m_mutex);
  QString key = id + "_" + QString::number(size.width()) + "x" +
                QString::number(size.height());
  if (m_cache.contains(key)) {
    return *m_cache.object(key); // Standard copy (atomic ref-count)
  }
  return QImage();
}

void AsyncImageProvider::insertCachedImage(const QString &id,
                                           const QImage &image,
                                           const QSize &size) {
  QMutexLocker locker(&m_mutex);
  QString key = id + "_" + QString::number(size.width()) + "x" +
                QString::number(size.height());
  m_cache.insert(key, new QImage(image), image.sizeInBytes() / 1024);
}

void AsyncImageProvider::setCacheMaxCost(int cost) {
  QMutexLocker locker(&m_mutex);
  m_cache.setMaxCost(cost);
}

QVariantMap AsyncImageProvider::getCacheStats() {
  QMutexLocker locker(&m_mutex);
  QVariantMap stats;
  stats["totalCost"] = m_cache.totalCost();
  stats["maxCost"] = m_cache.maxCost();
  return stats;
}

void AsyncImageProvider::clearCache() {
  QMutexLocker locker(&m_mutex);
  m_cache.clear();
}

void AsyncImageProvider::clearDiskCache() {
  QString cacheDir =
      QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
      "/thumbnails";
  QDir dir(cacheDir);
  if (dir.exists())
    dir.removeRecursively();
}