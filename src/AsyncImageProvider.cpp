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
#include <QPainter>
#include <QSettings>
#include <QStandardPaths>
#include <QThread>
#include <QVariant>
#include <QtCore/QEventLoop>
#include <QtCore/QSemaphore>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
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
static QSemaphore
    s_videoSemaphore(1); // Reduce to 1 to prevent GPU TDR/Driver crashes
static QSemaphore s_rawSemaphore(2); // Reduce to 2 to prevent CPU starvation
QCache<QString, QImage> AsyncImageProvider::m_cache;
QMutex AsyncImageProvider::m_mutex;
QMap<QString, QList<AsyncImageResponse *>>
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

static QString normalizeId(const QString &id) {
  QString n = id;
  if (n.startsWith("image://async/"))
    n = n.mid(14);

  // If it's a protocol URI or synthetic path, don't clean it as a path
  if (n.contains("://")) {
    return n.toLower();
  }

  // Process as file path: normalize separators
  n.replace("\\", "/");
  if (n.startsWith("file:///"))
    n = n.mid(8);
  else if (n.startsWith("file://"))
    n = n.mid(7);

  // Still clean local file paths but keep lowercase consistency
  return QDir::cleanPath(n).toLower();
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
  QFileInfo fi(normalizeId(id));
  QString key =
      normalizeId(id) + QString::number(fi.lastModified().toMSecsSinceEpoch()) +
      QString::number(size.width()) + "x" + QString::number(size.height());
  QString hash =
      QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Md5).toHex();
  return cacheDir + "/" + hash + ".jpg";
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

void AsyncImageResponse::handleDone(QImage image) {
  QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
  if (AsyncImageProvider::s_logLevel > 1) {
    qWarning() << "[" << timeStr << "][TRACE] handleDone for" << this
               << "(null=" << image.isNull() << ")";
  }

  auto finishTask = [this, image]() {
    m_image = image;
    emit finished();
  };

  if (AsyncImageProvider::s_frameScheduler) {
    AsyncImageProvider::s_frameScheduler->onTaskCompleted(finishTask);
  } else {
    finishTask();
  }
}

bool AsyncImageProvider::isRequestStillNeeded(const QString &id) {
  QMutexLocker locker(&m_pendingMutex);
  if (!m_pendingResponses.contains(id))
    return false;
  for (auto *resp : m_pendingResponses[id]) {
    if (resp && resp->m_tracker && resp->m_tracker->response.load()) {
      if (!resp->m_cancelled->load())
        return true;
    }
  }
  return false;
}

void AsyncImageProvider::deliverToPending(const QString &id,
                                          const QImage &image, int duration) {
  QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
  QMutexLocker locker(&m_pendingMutex);
  QList<AsyncImageResponse *> listeners = m_pendingResponses.take(id);
  locker.unlock();

  if (s_logLevel > 1) {
    qDebug() << "[" << timeStr << "][TRACE] Delivering result for" << id << "to"
             << listeners.size() << "listeners (took" << duration << "ms)";
  }

  for (AsyncImageResponse *resp : listeners) {
    if (resp && resp->m_tracker && resp->m_tracker->response.load()) {
      resp->m_workDuration = duration;
      QMetaObject::invokeMethod(resp, "handleDone", Qt::QueuedConnection,
                                Q_ARG(QImage, image));
    }
  }
}

#include "VisibleRangeManager.h"

QList<AsyncImageProvider::StagedRequest> AsyncImageProvider::m_stagedRequests;
QMutex AsyncImageProvider::m_stagingMutex;

QQuickImageResponse *
AsyncImageProvider::requestImageResponse(const QString &id,
                                         const QSize &requestedSize) {
  QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
  if (s_logLevel > 1) {
    qDebug() << "[" << timeStr << "][TRACE] requestImageResponse for" << id
             << requestedSize;
  }

  QString normalizedId = normalizeId(id);
  auto *response = new AsyncImageResponse(normalizedId, requestedSize);

  QImage cached = getCachedImage(normalizedId, requestedSize);
  if (!cached.isNull()) {
    QMetaObject::invokeMethod(response, "handleDone", Qt::QueuedConnection,
                              Q_ARG(QImage, cached));
    return response;
  }

  {
    QMutexLocker locker(&m_pendingMutex);
    m_pendingResponses[normalizedId].append(response);
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
  static qint64 lastScheduleTime = 0;
  qint64 now = QDateTime::currentMSecsSinceEpoch();
  if (now - lastScheduleTime > 20) {
    QTimer::singleShot(10, []() { processStagedRequests(); });
    lastScheduleTime = now;
  }
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

  for (const auto &req : batch) {
    if (!isRequestStillNeeded(req.id))
      continue;
    if (vrm.isPathVisible(req.id)) {
      visibleHighPriority.append(req);
    } else if (!lowMemory) {
      offscreen.append(req);
    }
  }

  // LIFO Sort for Visible items
  std::sort(visibleHighPriority.begin(), visibleHighPriority.end(),
            [](const StagedRequest &a, const StagedRequest &b) {
              return a.timestamp > b.timestamp;
            });

  // Limit high-priority dispatch to prevent UI thread flooding
  int dispatched = 0;
  QList<StagedRequest> deferredVisible;

  for (const auto &req : visibleHighPriority) {
    if (dispatched < 40) {
      TaskScheduler::instance().addTask(
          [req]() {
            AsyncImageProvider::processImageTask(req.id, req.requestedSize,
                                                 req.cancelled, req.tracker);
          },
          TaskScheduler::CPU_BOUND, TaskScheduler::Immediate);
      dispatched++;
    } else {
      deferredVisible.append(req);
    }
  }

  // Submit Offscreen (Throttled)
  if (TaskScheduler::instance().activeTaskCount() < 30) {
    for (const auto &req : offscreen) {
      TaskScheduler::instance().addTask(
          [req]() {
            AsyncImageProvider::processImageTask(req.id, req.requestedSize,
                                                 req.cancelled, req.tracker);
          },
          TaskScheduler::CPU_BOUND, TaskScheduler::Normal);
    }
  } else {
    // Re-queue remaining offscreen items
    QMutexLocker lock(&m_stagingMutex);
    for (const auto &req : offscreen) {
      m_stagedRequests.append(req);
    }
  }

  // Re-queue deferred visible items
  if (!deferredVisible.isEmpty()) {
    QMutexLocker lock(&m_stagingMutex);
    m_stagedRequests.append(deferredVisible);
  }

  if (!m_stagedRequests.isEmpty())
    scheduleStagingProcessing();
}

void AsyncImageProvider::processImageTaskInternal(
    QString id, QSize requestedSize, std::shared_ptr<std::atomic<bool>> c,
    std::shared_ptr<ResponseTracker> t) {

  QString timeLogStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
  if (s_logLevel > 0) {
    qDebug() << "[" << timeLogStr
             << "][Worker] processImageTaskInternal START for" << id;
  }

  if (!isRequestStillNeeded(id)) {
    QMutexLocker locker(&m_pendingMutex);
    m_pendingResponses.remove(id);
    return;
  }

  auto shouldAbort = [&]() {
    return (c && c->load()) || !isRequestStillNeeded(id) ||
           TaskScheduler::instance().isPaused() ||
           TaskScheduler::instance().activeTaskCount() > 2000;
  };

  // If paused, abort immediately (faster than waiting)
  if (TaskScheduler::instance().activeTaskCount() > 0 &&
      TaskScheduler::instance().activeTaskCount() % 10 == 0) {
    // Just a throttle check
  }

  if (TaskScheduler::instance().activeTaskCount() > 0 &&
      TaskScheduler::instance().activeTaskCount() % 50 == 0) {
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
  QUrl url(id);
  path = (url.isValid() && url.isLocalFile()) ? url.toLocalFile() : id;
  if (path.startsWith("/") && path.length() > 2 && path[2] == ':')
    path = path.mid(1);
  path = QDir::toNativeSeparators(path);

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
    if (s_useDiskCache) {
      QString dp = getDiskCachePath(id, requestedSize);
      if (QFile::exists(dp)) {
        image.load(dp);
        if (!image.isNull())
          goto deliver;
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

  if (s_logLevel > 0) {
    qDebug() << "[" << timeLogStr
             << "][Worker] processImageTaskInternal DONE for" << id
             << "(null=" << image.isNull() << ")";
  }
  deliverToPending(id, image, workTimer.elapsed());
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
  return m_cache.contains(key) ? *m_cache.object(key) : QImage();
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