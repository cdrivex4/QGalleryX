#include "AsyncImageProvider.h"
#include "DesktopHelper.h"
#include "FrameBudgetScheduler.h"
#include "SystemMonitor.h"
#include "TaskScheduler.h"
#include "VideoThumbnailer.h"
#include "FileCacheManager.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QBuffer>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFileIconProvider>
#include "HardwareAccelerationManager.h"
extern "C" {
#include <libavutil/pixfmt.h>
}
#include "HardwareAccelerationManager.h"
extern "C" {
#include <libavutil/pixfmt.h>
}
#include <QIcon>
#include <QImageReader>
#include <QMutex>
#include <QMutexLocker>
#include <QPainter>
#include <QLinearGradient>
#include <QSet>
#include <QSettings>
#include <QStandardPaths>
#include <QThread>
#include <QUrl>
#include <QVariant>
#include <QCoreApplication>
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
static QSemaphore *getVideoSemaphore() {
  static QSemaphore *sem = nullptr;
  static QMutex initMutex;
  QMutexLocker lock(&initMutex);
  if (!sem) {
    int cores = std::thread::hardware_concurrency();
    int threads = 1;
    if (cores >= 8) threads = 3;
    else if (cores >= 6) threads = 2;
    
    // Give a bump if hardware decoding is enabled (offloads CPU)
    if (HardwareAccelerationManager::instance().pixelFormat() != AV_PIX_FMT_NONE) {
      threads += 1;
    }
    // Cap strictly to avoid GPU TDRs and network saturation
    threads = std::clamp(threads, 1, 4);
    sem = new QSemaphore(threads);
  }
  return sem;
}
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
std::atomic<int> AsyncImageProvider::s_cacheHits{0};
std::atomic<int> AsyncImageProvider::s_cacheMisses{0};
std::atomic<int> AsyncImageProvider::s_totalWorkDuration(0);
std::atomic<int> AsyncImageProvider::s_workCount(0);
std::atomic<bool> AsyncImageProvider::s_useDiskCache{false};
std::atomic<int> AsyncImageProvider::s_videoAcceleration{0}; // 0 = Auto/Default
FrameBudgetScheduler *AsyncImageProvider::s_frameScheduler = nullptr;

struct QueueGuardState {
  QString drive;
  QString taskId;
  std::atomic<bool> executed{false};

  QueueGuardState(const QString &id) : taskId(id) {
    drive = AsyncImageProvider::getDriveRoot(id);
  }

  ~QueueGuardState() {
    if (!executed.load()) {
      QMutexLocker lock(&AsyncImageProvider::m_driveStatsMutex);
      int weight = AsyncImageProvider::getTaskWeight(taskId);
      AsyncImageProvider::m_driveStats[drive].activeWeight =
          qMax(0, AsyncImageProvider::m_driveStats[drive].activeWeight - weight);
      
      QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
      qWarning() << "[" << timeStr << "][AdaptiveIO] Leaked weight recovered for cleared task:" << taskId
                 << "Weight:" << weight << "New Active Weight:" << AsyncImageProvider::m_driveStats[drive].activeWeight;
    }
  }
};

// Composite key for coalescing: ensures that a request for a 200x200 thumb
// does not get fulfilled by a 10x10 preview task for the same file.
static QString getCoalesceKey(const QString &normalizedId, const QSize &size) {
  return QString("%1_%2x%3")
      .arg(normalizedId)
      .arg(size.width())
      .arg(size.height());
}

  static QString normalizePath(const QString &p) {
    QString res = p;
    int idx = res.indexOf("?idx=");
    if (idx != -1) res = res.left(idx);
    
    if (res.startsWith("file://")) {
      QUrl url(res);
      return url.toLocalFile().toLower();
    }
    return QDir::toNativeSeparators(res).toLower();
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
  path = path.toLower();
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
      QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
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
  return cacheDir + "/" + hash + "_" + QString::number(size.width()) +
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
      ext == "wmv" || ext == "heic" || ext == "heif") {
    return 4; // Video thumbnailing / HEVC decoding is heavy on CPU/Video engine
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
    } else {
      concurrencyLimit = 4; // Start local drives at 4 (minLimit)
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

  // Periodic timer to rescue stalled tasks (e.g. slow network shares)
  QTimer *stallTimer = new QTimer(qApp);
  QObject::connect(stallTimer, &QTimer::timeout, []() {
    AsyncImageProvider::checkStalls();
  });
  stallTimer->start(5000); // Check every 5 seconds
}

void AsyncImageProvider::setFrameScheduler(FrameBudgetScheduler *s) {
  s_frameScheduler = s;
}

AsyncImageResponse::~AsyncImageResponse() {
  if (m_tracker) {
    QMutexLocker lock(&m_tracker->mutex);
    m_tracker->response = nullptr;
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
    FileCacheManager::instance().initialize();
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

  QPointer<AsyncImageResponse> self(this);
  auto finishTask = [self, image]() {
    if (!self) return;
    if (self->m_cancelled->load()) return;
    self->m_image = image;
    emit self->finished();
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
    if (tracker) {
      QMutexLocker tLocker(&tracker->mutex);
      if (tracker->response && !tracker->response->m_cancelled->load())
        return true;
    }
  }
  return false;
}

bool AsyncImageProvider::abortIfNotNeeded(const QString &coalesceKey) {
  QList<std::shared_ptr<ResponseTracker>> listenersToAbort;
  {
    QMutexLocker locker(&m_pendingMutex);
    if (!m_pendingResponses.contains(coalesceKey))
      return true;

    bool needed = false;
    for (const auto &tracker : m_pendingResponses[coalesceKey]) {
      if (tracker) {
        QMutexLocker tLocker(&tracker->mutex);
        if (tracker->response && !tracker->response->m_cancelled->load()) {
          needed = true;
          break;
        }
      }
    }

    if (needed)
      return false;

    listenersToAbort = m_pendingResponses.take(coalesceKey);
  }

  for (auto &t : listenersToAbort) {
    if (t) {
      QMutexLocker tl(&t->mutex);
      if (t->response) {
        QMetaObject::invokeMethod(t->response, "handleDone", Qt::QueuedConnection, Q_ARG(QImage, QImage()), Q_ARG(int, 0));
      }
    }
  }
  return true;
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
    QMutexLocker tLocker(&tracker->mutex);
    if (tracker->response) {
      QMetaObject::invokeMethod(tracker->response, "handleDone", Qt::QueuedConnection,
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
    s_cacheHits++;
    // Use QueuedConnection so the caller has time to connect to the finished() signal!
    QMetaObject::invokeMethod(response, "handleDone", Qt::QueuedConnection,
                              Q_ARG(QImage, cached), Q_ARG(int, 0));
    return response;
  }

  s_cacheMisses++;

  // Disk Cache check: Moved to Worker thread (processImageTask) to avoid
  // blocking GUI thread on slow drives (like SD cards).
  bool alreadyPending = false;
  {
    QMutexLocker locker(&m_pendingMutex);
    if (m_pendingResponses.contains(cKey) && !m_pendingResponses[cKey].isEmpty()) {
      alreadyPending = true;
    }
    m_pendingResponses[cKey].append(response->m_tracker);
  }

  if (alreadyPending) {
    if (s_logLevel > 0) {
      qDebug() << "[AsyncImageProvider] Coalesced request, already pending/running:" << normalizedId;
    }
    return response;
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

  QDateTime now = QDateTime::currentDateTime();
  const qint64 STAGE_EXPIRY_MS = 5000; // 5 seconds expiry

  for (const auto &req : batch) {
    QString cKey = getCoalesceKey(req.id, req.requestedSize);

    // Age-out check: If it has been stuck in staging for > 5s, drop it.
    if (req.timestamp.msecsTo(now) > STAGE_EXPIRY_MS) {
      deliverToPending(cKey, QImage(), 0);
      continue;
    }

    if (abortIfNotNeeded(cKey)) {
      continue;
    }

    QString drive = getDriveRoot(req.id);
    QMutexLocker statsLock(&m_driveStatsMutex);
    DriveStats &stats = m_driveStats[drive];

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
      // Visible items can use BURST capacity (e.g. +12 for network, +6 for local)
      int burst = stats.isNetwork ? 12 : 6;
      
      // EXPRESS LANE: Give light tasks (JPEGs) an extra massive burst so they
      // don't get stuck behind heavy long-running FFmpeg video decodes.
      if (weight == 1) {
          burst += (stats.isNetwork ? 40 : 20);
      }

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
      else {
        // We are low on memory and it's offscreen. Don't abort it, just re-queue it!
        // If it was actually visible but VRM was lagging, it will be admitted next cycle.
        QMutexLocker lock(&m_stagingMutex);
        m_stagedRequests.append(req);
        continue; // Don't count as drive allocation
      }

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
    {
      QMutexLocker lock(&m_driveStatsMutex);
      m_driveStats[d].activeWeight += weight;
    }
    return std::make_shared<QueueGuardState>(id);
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
    auto guard = incrementActive(req.id);
    TaskScheduler::TaskType targetPool = (getTaskWeight(req.id) >= 4) ? TaskScheduler::CPU_BOUND : TaskScheduler::IO_BOUND;
    
    QString ext = QFileInfo(getRealLocalPath(req.id)).suffix().toLower();
    bool isVideo = (ext == "mp4" || ext == "mov" || ext == "mkv" || ext == "avi" || ext == "wmv");
    TaskScheduler::TaskCategory cat = isVideo ? TaskScheduler::VideoTask : TaskScheduler::ImageTask;

    QString cKey = getCoalesceKey(req.id, req.requestedSize);
        TaskScheduler::instance().addTask(
          TaskScheduler::Task(
              [req, guard]() {
                guard->executed.store(true);
                AsyncImageProvider::processImageTask(req.id, req.requestedSize,
                                                     req.cancelled, req.tracker);
              },
              [cKey]() { return AsyncImageProvider::abortIfNotNeeded(cKey); },
              [cKey]() { /* Abort handled inside abortIfNotNeeded */ }),
          targetPool, TaskScheduler::Immediate, cat);
  }

  // Submit Offscreen
  for (const auto &req : offscreen) {
    auto guard = incrementActive(req.id);
    TaskScheduler::TaskType targetPool = (getTaskWeight(req.id) >= 4) ? TaskScheduler::CPU_BOUND : TaskScheduler::IO_BOUND;

    QString ext = QFileInfo(getRealLocalPath(req.id)).suffix().toLower();
    bool isVideo = (ext == "mp4" || ext == "mov" || ext == "mkv" || ext == "avi" || ext == "wmv");
    TaskScheduler::TaskCategory cat = isVideo ? TaskScheduler::VideoTask : TaskScheduler::ImageTask;

    QString cKey = getCoalesceKey(req.id, req.requestedSize);
    TaskScheduler::instance().addTask(
        TaskScheduler::Task(
          [req, guard]() {
            guard->executed.store(true);
            AsyncImageProvider::processImageTask(req.id, req.requestedSize,
                                                 req.cancelled, req.tracker);
          },
          [cKey]() { return isRequestStillNeeded(cKey); },
          [cKey]() { AsyncImageProvider::deliverToPending(cKey, QImage(), 0); }
        ),
        targetPool, TaskScheduler::Normal, cat);
  }

  // Do NOT blindly schedule staging processing if tasks are queued,
  // this causes a 2ms spin loop on the UI thread when admission control is full.
  // The pump is fully event-driven now via ~DriveConcurrencyGuard!
}

void AsyncImageProvider::processImageTaskInternal(
    QString id, QSize requestedSize, std::shared_ptr<std::atomic<bool>> c,
    std::shared_ptr<ResponseTracker> t) {

  QString cKey = getCoalesceKey(id, requestedSize);

  // CRITICAL FIX: Drive concurrency accounting MUST happen before ANY early returns,
  // otherwise we leak activeWeight and permanently lock the Admission Control queue!
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
      bool wasActive = false;
      {
        QMutexLocker lock(&s_activeTasksMutex);
        wasActive = (s_activeTasksMap.remove(taskId) > 0);
      }
      if (wasActive) {
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

  auto shouldAbort = [&]() {
    // CRITICAL FIX: Do NOT check the initial `c->load()` here!
    // Because requests are coalesced, the FIRST caller (c) might have been 
    // cancelled (scrolled off screen), but a NEW caller (scrolled back on screen) 
    // might be actively waiting for this exact coalesceKey!
    // Note: The scheduler already checks this before admitting the task, but we 
    // check it one last time just in case it was cancelled during lock acquisition.
    if (!isRequestStillNeeded(cKey))
      return true;
      
    if (TaskScheduler::instance().isPaused())
      return true;

    // We no longer abort purely based on >5k tasks because LIFO queues mean
    // we would end up aborting the NEWEST, VISIBLE tasks first!
    // The `isRequestStillNeeded` check above will naturally cull the old offscreen ones.
    return false;
  };

  if (s_logLevel > 0) {

    // If paused, abort immediately (faster than waiting)
    if (TaskScheduler::instance().activeTaskCount() > 0 &&
        TaskScheduler::instance().activeTaskCount() % 10 == 0) {
      // Just a throttle check
    }

    qDebug() << "[AsyncImageProvider] High workload detect. Queue:"
             << TaskScheduler::instance().activeTaskCount();
  }

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
      QByteArray mmapData = FileCacheManager::instance().getCachedData(id, requestedSize);
      if (!mmapData.isEmpty()) {
        if (image.loadFromData(mmapData)) {
          insertCachedImage(id, image, requestedSize);
          driveGuard.commitStats();
          deliverToPending(cKey, image, workTimer.elapsed());
          return;
        }
      } else {
        QString dp = FileCacheManager::instance().getCachedPath(id, requestedSize);
        if (!dp.isEmpty()) {
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
    }

      DesktopHelper::FileType type = DesktopHelper::staticGetFileType(path);
    if (type == DesktopHelper::Raw) {
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
    } else if (type == DesktopHelper::Video || path.endsWith(".heic", Qt::CaseInsensitive) || path.endsWith(".heif", Qt::CaseInsensitive)) {
      QSemaphore *sem = getVideoSemaphore();
      sem->acquire(1);
      QSemaphoreReleaser releaser(sem);
      if (!shouldAbort()) {
        VideoThumbnailer v;
        SettingsHelper::HWAccel accel =
            static_cast<SettingsHelper::HWAccel>(s_videoAcceleration.load());
        // Pass nullptr for the cancellation token. If we pass the original delegate's token,
        // it might abort FFmpeg if that specific delegate is destroyed, even if OTHER delegates
        // are waiting for this coalesced task, resulting in a permanent dead placeholder!
        image = v.extractFrame(path, 0, requestedSize, accel, nullptr);
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
  // Distinguish between genuinely broken files vs. transient decode failures.
  // CRITICAL: Only generate and cache placeholders for genuinely broken files!
  // If we generate a placeholder for a transient failure (OOM, abort, disk hiccup)
  // it gets cached and permanently blocks future valid load attempts.
  bool isGenuinelyBroken = false;
  if (image.isNull() && !shouldAbort()) {
    QString realPath = getRealLocalPath(id);
    if (!QFile::exists(realPath)) {
      // File doesn't exist at all - genuinely broken
      isGenuinelyBroken = true;
    } else {
      // File exists, probe it quickly to distinguish format errors from transient I/O
      QImageReader probe(realPath);
      probe.setDecideFormatFromContent(true);
      if (!probe.canRead()) {
        // Permanent format error (unsupported format, corrupted header)
        // Exclude video/raw formats which we handle ourselves - probe can't read those
        QString ext = QFileInfo(realPath).suffix().toLower();
        bool isMediaFormat = (ext == "mp4" || ext == "mov" || ext == "mkv" || ext == "avi" ||
                              ext == "wmv" || ext == "heic" || ext == "heif" ||
                              ext == "arw" || ext == "cr2" || ext == "dng" || ext == "nef" ||
                              ext == "sr2" || ext == "srf" || ext == "orf" || ext == "rw2" ||
                              ext == "pef" || ext == "raf");
        if (!isMediaFormat) {
          // Standard image format that QImageReader can handle but can't read = corrupt
          isGenuinelyBroken = true;
        }
        // For media formats we handle ourselves, a null image means a transient decode
        // failure - don't mark as broken, let the next request retry.
      }
      // If probe.canRead() == true but image is still null, it was a transient I/O
      // failure during the actual read - also NOT genuinely broken.
    }

    if (s_logLevel > 0) {
      qWarning() << "[AsyncImageProvider] FAILED load. ID:" << id
                 << "Path:" << getRealLocalPath(id)
                 << "GenuinelyBroken:" << isGenuinelyBroken;
    }

    if (isGenuinelyBroken) {
      QSize imgSize = requestedSize.isValid() ? requestedSize : QSize(200, 200);
      QString ext = QFileInfo(getRealLocalPath(id)).suffix().toLower();

      if (ext == "heic" || ext == "heif") {
        // Draw premium HEIC format placeholder
        image = QImage(imgSize, QImage::Format_ARGB32_Premultiplied);

        QLinearGradient gradient(0, 0, 0, imgSize.height());
        gradient.setColorAt(0, QColor(108, 92, 231)); // Purple
        gradient.setColorAt(1, QColor(72, 52, 212));  // Dark Purple

        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(image.rect(), gradient);

        painter.setPen(QPen(QColor(255, 255, 255, 40), 1));
        painter.drawRect(image.rect().adjusted(0, 0, -1, -1));

        painter.setPen(Qt::white);
        QFont font("Segoe UI", qMax(10, qMin(imgSize.width(), imgSize.height()) / 8), QFont::Bold);
        painter.setFont(font);
        painter.drawText(image.rect(), Qt::AlignCenter, "HEIC");

        int iconSize = qMin(imgSize.width(), imgSize.height()) / 5;
        if (iconSize > 10) {
          int cx = imgSize.width() / 2;
          int cy = imgSize.height() / 2 - iconSize - 5;
          QRectF frame(cx - iconSize / 2, cy - iconSize / 2, iconSize, iconSize);
          painter.setPen(QPen(Qt::white, 2));
          painter.setBrush(Qt::NoBrush);
          painter.drawRoundedRect(frame, 3, 3);
          painter.drawEllipse(QPointF(cx - iconSize / 4, cy - iconSize / 4), 2, 2);
        }
      } else {
        // Draw dark grey placeholder with orange warning icon for corrupt/failed files
        image = QImage(imgSize, QImage::Format_ARGB32_Premultiplied);
        image.fill(QColor(44, 44, 44));

        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);

        painter.setPen(QPen(QColor(60, 60, 60), 1));
        painter.drawRect(image.rect().adjusted(0, 0, -1, -1));

        int size = qMin(imgSize.width(), imgSize.height()) * 0.4;
        if (size > 8) {
          int cx = imgSize.width() / 2;
          int cy = imgSize.height() / 2;

          QPolygonF triangle;
          triangle << QPointF(cx, cy - size / 2)
                   << QPointF(cx - size / 2, cy + size / 2)
                   << QPointF(cx + size / 2, cy + size / 2);

          painter.setBrush(QColor(230, 126, 34, 40));
          painter.setPen(QPen(QColor(230, 126, 34), qMax(2, size / 10), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
          painter.drawPolygon(triangle);

          painter.setPen(Qt::NoPen);
          painter.setBrush(QColor(230, 126, 34));

          qreal execWidth = qMax(2.0, size * 0.08);
          qreal execHeight = size * 0.35;
          painter.drawRoundedRect(QRectF(cx - execWidth / 2, cy - size * 0.12, execWidth, execHeight), execWidth / 2, execWidth / 2);

          qreal dotSize = qMax(2.0, size * 0.08);
          painter.drawEllipse(QPointF(cx, cy + size * 0.32), dotSize / 2, dotSize / 2);
        }
      }
    }
    // If NOT genuinely broken, image stays null - caller will get QImage() and can retry
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
    // Only cache real images or genuinely-broken placeholders.
    // NEVER cache transient failures so future requests can retry.
    insertCachedImage(id, image, requestedSize);
    if (isGenuinelyBroken) {
      // Don't write corrupt-file placeholders to disk cache - they waste space
      // and disk cache has no TTL to expire them naturally.
    } else if (s_useDiskCache && !id.startsWith("synthetic:")) {
      QSettings settings("SamsungClone", "Gallery");
      if (settings.value("diskCacheDatabaseType", 0).toInt() == 1) {
        QByteArray ba;
        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "JPG", 80);
        FileCacheManager::instance().registerCachedData(id, requestedSize, ba);
      } else {
        QString dp = getDiskCachePath(id, requestedSize);
        if (image.save(dp, "JPG", 80)) {
          FileCacheManager::instance().registerCacheFile(id, requestedSize, dp, QFile(dp).size());
        }
      }
    }
  }

  int duration = workTimer.elapsed();

  if (!id.startsWith("synthetic:") && !id.startsWith("image://")) {
    s_totalWorkDuration += duration;
    s_workCount++;
  }

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
  FileCacheManager::instance().clearCache();
}