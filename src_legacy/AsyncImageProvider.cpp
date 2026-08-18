#include "AsyncImageProvider.h"
#include "../src/FileCacheManager.h"
#include "../src/PassiveReadLatencyGuard.h"
#include "DesktopHelper.h"
#include "SystemMonitor.h"
#include "TaskScheduler.h"
#include "VideoThumbnailer.h"
#include "ViewportGovernor.h"
#include <QBuffer>
#include <QElapsedTimer>
#include <QFileIconProvider>
#include <QIcon>
#include <QImageReader>
#include <QPainter>
#include <QSettings>
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
#include <libraw/libraw.h>


#ifdef Q_OS_WIN
#include <objbase.h>
#endif

// Initialize static members
static QSemaphore s_videoSemaphore(4);
static QSemaphore s_rawSemaphore(2); // Limit to 2 concurrent RAW loads
QCache<QString, QImage> AsyncImageProvider::m_cache;
QMutex AsyncImageProvider::m_mutex;
QHash<QString, QList<QPointer<AsyncImageResponse>>>
    AsyncImageProvider::m_pendingTasks;
std::atomic<int> AsyncImageProvider::s_logLevel(0);
std::atomic<bool> AsyncImageProvider::s_accelerateRaw(true);
std::atomic<int> AsyncImageProvider::s_l1Hits(0);
std::atomic<int> AsyncImageProvider::s_l2Hits(0);
std::atomic<int> AsyncImageProvider::s_misses(0);

AsyncImageResponse::AsyncImageResponse(const QString &id,
                                       const QSize &requestedSize)
    : m_id(id), m_requestedSize(requestedSize),
      m_cancelled(std::make_shared<std::atomic<bool>>(false)) {

  // Ensure cache size is initialized (lazy init)
  static bool configured = false;
  if (!configured) {
    QSettings settings("SamsungClone", "Gallery");
    // Reduce default cache to 256MB to be safer with RAWs
    int cacheSizeMB = settings.value("cacheSizeMB", 256).toInt();
    // Enforce a hard limit for stability
    if (cacheSizeMB <= 0)
      cacheSizeMB = 256;

    // Note: Cost is sizeInBytes() / 1024. Wait, let's check insertion.
    // Insertion says: m_cache.insert(key, new QImage(image),
    // image.sizeInBytes() / 1024); So cost is in KB. So setCacheMaxCost should
    // be in KB. cacheSizeMB * 1024 = KB.
    AsyncImageProvider::setCacheMaxCost(cacheSizeMB * 1024);
    configured = true;
  }
}

QQuickTextureFactory *AsyncImageResponse::textureFactory() const {
  return QQuickTextureFactory::textureFactoryForImage(m_image);
}

void AsyncImageResponse::cancel() { *m_cancelled = true; }

void AsyncImageResponse::handleDone(QImage image) {
  m_image = image;
  emit finished();
}

void AsyncImageProvider::crawlDecodeToL2(const QString &path,
                                         const QSize &requestedSize) {
  // Lane 2: Background-only. Never touches m_pendingTasks.
  // Guard: skip if already in L2 disk cache.
  if (!FileCacheManager::instance().getCachedData(path, requestedSize).isEmpty()) {
    return;
  }

#ifdef Q_OS_WIN
  CoInitialize(NULL);
#endif

  QImage image;
  DesktopHelper::FileType type = DesktopHelper::staticGetFileType(path);
  bool isHeic = path.endsWith(".heic", Qt::CaseInsensitive) || path.endsWith(".heif", Qt::CaseInsensitive);
  bool isVideo = (type == DesktopHelper::Video) || isHeic;
  bool isRaw = (type == DesktopHelper::Raw);

  if (isRaw) {
    s_rawSemaphore.acquire(1);
    QSemaphoreReleaser releaser(s_rawSemaphore);
    try {
      LibRaw RawProcessor;
      struct RawCleanup { LibRaw &p; ~RawCleanup() { p.recycle(); } } cleanup{RawProcessor};
      if (RawProcessor.open_file(path.toLocal8Bit().constData()) == LIBRAW_SUCCESS) {
        if (s_accelerateRaw && RawProcessor.unpack_thumb() == LIBRAW_SUCCESS) {
          libraw_processed_image_t *thumb = RawProcessor.dcraw_make_mem_thumb();
          if (thumb) {
            struct ThumbCleanup { libraw_processed_image_t *t; ~ThumbCleanup() { if (t) LibRaw::dcraw_clear_mem(t); } } tc{thumb};
            if (thumb->type == LIBRAW_IMAGE_JPEG)
              image.loadFromData((const uchar *)thumb->data, thumb->data_size, "JPEG");
            else if (thumb->type == LIBRAW_IMAGE_BITMAP) {
              QImage rawImg((const uchar *)thumb->data, thumb->width, thumb->height,
                            thumb->width * 3, QImage::Format_RGB888);
              image = rawImg.copy();
            }
          }
        }
      }
    } catch (...) {}
  } else if (isVideo) {
    s_videoSemaphore.acquire(1);
    QSemaphoreReleaser releaser(s_videoSemaphore);
    auto dummyCancelled = std::make_shared<std::atomic<bool>>(false);
    VideoThumbnailer thumbnailer;
    image = thumbnailer.extractFrame(path, 0, requestedSize, dummyCancelled.get());

    if (image.isNull() && isHeic) {
      QImageReader reader(path);
      if (requestedSize.isValid()) reader.setScaledSize(requestedSize);
      if (reader.canRead()) image = reader.read();
    }
  } else {
    QImageReader reader(path);
    if (requestedSize.isValid()) {
      QSize orig = reader.size();
      if (orig.isValid()) {
        double scale = std::max((double)requestedSize.width() / orig.width(),
                                (double)requestedSize.height() / orig.height());
        reader.setScaledSize(QSize(orig.width() * scale, orig.height() * scale));
      } else {
        reader.setScaledSize(requestedSize);
      }
    }
    if (reader.canRead()) image = reader.read();
    if (image.isNull()) {
      // Fallback: FFmpeg can decode formats that QImageReader cannot (e.g. old-style JPEG, TIFF embedded)
      VideoThumbnailer thumbnailer;
      image = thumbnailer.extractFrame(path, 0, requestedSize, nullptr);
    }
  }

  // Fallback for null/failed extractions — create placeholder tile so L2 persists it
  if (image.isNull()) {
    QSize sz = requestedSize.isValid() ? requestedSize : QSize(200, 200);
    image = QImage(sz, QImage::Format_RGB32);
    image.fill(QColor("#222222"));
    QPainter p(&image);
    p.setPen(QColor("#888888"));
    p.setFont(QFont("Segoe UI", 10, QFont::Bold));
    p.drawText(image.rect(), Qt::AlignCenter, isVideo ? "▶ VID" : "ERR");
  }

  if (!image.isNull()) {
    if (requestedSize.isValid() && !requestedSize.isEmpty()) {
      if (image.width() > requestedSize.width() || image.height() > requestedSize.height())
        image = image.scaled(requestedSize, Qt::KeepAspectRatio, Qt::FastTransformation);
    }
    if (requestedSize.isValid()) {
      QByteArray ba;
      QBuffer buffer(&ba);
      buffer.open(QIODevice::WriteOnly);
      image.save(&buffer, "JPG", 85);
      FileCacheManager::instance().registerCachedData(path, requestedSize, ba);
      insertCachedImage(path, image, requestedSize); // Direct promotion to L1 RAM!
    }
  }

#ifdef Q_OS_WIN
  CoUninitialize();
#endif
}

QQuickImageResponse *
AsyncImageProvider::requestImageResponse(const QString &id,
                                         const QSize &requestedSize) {
  QString cleanId = id;

  // QML percent-encodes paths (e.g. %5C instead of \)
  cleanId = QUrl::fromPercentEncoding(cleanId.toUtf8());

  int modelIdx = -1;
  int qIdx = cleanId.indexOf('?');
  if (qIdx != -1) {
    int idxPos = cleanId.indexOf("idx=");
    if (idxPos != -1) {
      modelIdx = cleanId.mid(idxPos + 4).toInt();
    }
    cleanId = cleanId.left(qIdx);
  }

  // Canonicalize file:/// URIs to uniform local path
  if (cleanId.startsWith("file:", Qt::CaseInsensitive)) {
    QUrl url(cleanId);
    if (url.isLocalFile()) {
      cleanId = url.toLocalFile();
    } else if (cleanId.startsWith("file:///", Qt::CaseInsensitive)) {
      cleanId = cleanId.mid(8);
    } else if (cleanId.startsWith("file://", Qt::CaseInsensitive)) {
      cleanId = cleanId.mid(7);
    } else if (cleanId.startsWith("file:", Qt::CaseInsensitive)) {
      cleanId = cleanId.mid(5);
    }
  }

  if (cleanId.startsWith("/") && cleanId.length() > 2 && cleanId[2] == ':') {
    cleanId = cleanId.mid(1);
  }
  cleanId = QDir::cleanPath(cleanId);
  cleanId = QDir::toNativeSeparators(cleanId);

  if (cleanId.isEmpty() || cleanId == "." || cleanId == "/" || cleanId == "\\") {
    auto *response = new AsyncImageResponse(cleanId, requestedSize);
    QMetaObject::invokeMethod(response, "handleDone", Qt::QueuedConnection,
                              Q_ARG(QImage, QImage()));
    return response;
  }

  auto *response = new AsyncImageResponse(cleanId, requestedSize);

  // 1. Check L1 RAM Cache Synchronously (0ms instant hit)
  QImage cached = getCachedImage(cleanId, requestedSize);
  if (!cached.isNull()) {
    int hits = s_l1Hits.fetch_add(1, std::memory_order_relaxed) + 1;
    if (hits % 50 == 0 || hits <= 5) {
      qDebug() << "[Cache] L1 RAM HIT (" << hits << "hits)" << QFileInfo(cleanId).fileName();
    }
    QMetaObject::invokeMethod(response, "handleDone", Qt::QueuedConnection,
                              Q_ARG(QImage, cached));
    return response;
  }

  // 2. Check L2 Memory-Mapped Disk Cache Synchronously (0ms instant hit from crawler for thumbnails ONLY!)
  // Full photo viewer requests (size is invalid, empty, or >512px) must decode the full-resolution original image.
  if (requestedSize.isValid() && !requestedSize.isEmpty() && requestedSize.width() <= 512 && requestedSize.height() <= 512) {
    QByteArray mmapData = FileCacheManager::instance().getCachedData(cleanId, requestedSize);
    if (!mmapData.isEmpty()) {
      QImage diskImg;
      if (diskImg.loadFromData(mmapData)) {
        int hits = s_l2Hits.fetch_add(1, std::memory_order_relaxed) + 1;
        if (hits % 25 == 0 || hits <= 5) {
          qDebug() << "[Cache] L2 MMAP DISK HIT (" << hits << "hits)" << QFileInfo(cleanId).fileName()
                   << "(" << mmapData.size() / 1024 << "KB slice)";
        }
        insertCachedImage(cleanId, diskImg, requestedSize);
        QMetaObject::invokeMethod(response, "handleDone", Qt::QueuedConnection,
                                  Q_ARG(QImage, diskImg));
        return response;
      }
    }
  }

  QString key = cleanId + "_" + QString::number(requestedSize.width()) + "x" +
                QString::number(requestedSize.height());
  {
    QMutexLocker locker(&m_mutex);
    if (m_pendingTasks.contains(key)) {
      // Deduplication: Task is already running!
      // Just attach our response to the list and return.
      m_pendingTasks[key].append(response);
      return response;
    }
    // Not running, we will start it.
    m_pendingTasks.insert(key, QList<QPointer<AsyncImageResponse>>()
                                   << response);
  }

  // --- PRIORITIZATION LOGIC ---
  TaskScheduler::Priority priority = TaskScheduler::Immediate;
  if (!requestedSize.isValid() ||
      (requestedSize.width() > 1000 || requestedSize.height() > 1000)) {
    // Full screen viewer requests are always immediate
    priority = TaskScheduler::Immediate;
  } else if (modelIdx >= 0) {
    // Viewport-aware priority dispatch
    if (ViewportGovernor::instance().isLookaheadTile(modelIdx)) {
      priority = TaskScheduler::Immediate;
    } else if (ViewportGovernor::instance().isOutOfLookaheadBounds(modelIdx)) {
      priority = TaskScheduler::Low;
    } else {
      priority = TaskScheduler::Immediate;
    }
  } else {
    priority = TaskScheduler::Immediate;
  }

  std::shared_ptr<std::atomic<bool>> cancelled = response->m_cancelled;

  bool added = TaskScheduler::instance().addTask(
      [cleanId, requestedSize, cancelled, response]() {
        AsyncImageProvider::processImageTask(cleanId, requestedSize, cancelled,
                                             response);
      },
      TaskScheduler::CPU_BOUND, priority, cleanId);

  if (!added) {
    // Task rejected (e.g. queue full or shutdown).
    // Clean up m_pendingTasks and deliver null QImage to prevent deadlock.
    QList<QPointer<AsyncImageResponse>> responses;
    {
      QMutexLocker locker(&m_mutex);
      responses = m_pendingTasks.take(key);
    }
    for (QPointer<AsyncImageResponse> r : responses) {
      if (r && !r->m_cancelled->load()) {
        QMetaObject::invokeMethod(r.data(), "handleDone", Qt::QueuedConnection,
                                  Q_ARG(QImage, QImage()));
      }
    }
  }

  return response;
}

static QString normalizeRamKey(const QString &id, const QSize &size) {
  QString cleanId = id;
  cleanId = QUrl::fromPercentEncoding(cleanId.toUtf8());
  int qIdx = cleanId.indexOf('?');
  if (qIdx != -1) cleanId = cleanId.left(qIdx);

  if (cleanId.startsWith("file:", Qt::CaseInsensitive)) {
    QUrl url(cleanId);
    if (url.isLocalFile()) cleanId = url.toLocalFile();
  }
  if (cleanId.startsWith("/") && cleanId.length() > 2 && cleanId[2] == ':') {
    cleanId = cleanId.mid(1);
  }
  cleanId = QDir::cleanPath(cleanId);
  cleanId = QDir::toNativeSeparators(cleanId).toLower();

  if (size.isValid() && !size.isEmpty() && size.width() <= 512 && size.height() <= 512) {
    return cleanId + "_thumb";
  }
  if (size.isValid() && !size.isEmpty()) {
    return cleanId + "_" + QString::number(size.width()) + "x" + QString::number(size.height());
  }
  return cleanId + "_full";
}

QImage AsyncImageProvider::getCachedImage(const QString &id,
                                          const QSize &size) {
  QMutexLocker locker(&m_mutex);
  QString key = normalizeRamKey(id, size);
  if (m_cache.contains(key)) {
    QImage *img = m_cache.object(key);
    if (img && !img->isNull()) {
      return img->copy(); // Deep copy prevents crash if m_cache is cleared on another thread
    }
  }
  return QImage();
}

void AsyncImageProvider::insertCachedImage(const QString &id,
                                           const QImage &image,
                                           const QSize &size) {
  QMutexLocker locker(&m_mutex);
  QString key = normalizeRamKey(id, size);
  m_cache.insert(key, new QImage(image), image.sizeInBytes() / 1024);
}

void AsyncImageProvider::promoteL2ToL1(const QString &path,
                                       const QSize &requestedSize) {
  QString key = normalizeRamKey(path, requestedSize);
  {
    QMutexLocker locker(&m_mutex);
    if (m_cache.contains(key))
      return; // Already in L1 RAM
    
    // Hard Capacity Guard: Prevent background promotions from saturating the LRU cache
    if (m_cache.totalCost() >= m_cache.maxCost())
      return;
  }

#ifdef Q_OS_WIN
  // Dynamic Host RAM Guard: Halt bulk background promotion if host physical RAM < 400 MB
  MEMORYSTATUSEX memInfo;
  memInfo.dwLength = sizeof(MEMORYSTATUSEX);
  if (GlobalMemoryStatusEx(&memInfo)) {
    if (memInfo.ullAvailPhys < (400ULL * 1024ULL * 1024ULL)) {
      return; // Protect system against OOM
    }
  }
#endif

  QByteArray mmapData =
      FileCacheManager::instance().getCachedData(path, requestedSize);
  if (!mmapData.isEmpty()) {
    QImage diskImg;
    if (diskImg.loadFromData(mmapData)) {
      insertCachedImage(path, diskImg, requestedSize);
    }
  }
}

void AsyncImageProvider::setCacheMaxCost(int cost) {
  QMutexLocker locker(&m_mutex);
  m_cache.setMaxCost(cost);
}

AsyncImageProvider::CacheLevel
AsyncImageProvider::checkCacheLevel(const QString &id, const QSize &size) {
  QString key = normalizeRamKey(id, size);
  {
    QMutexLocker locker(&m_mutex);
    if (m_cache.contains(key)) {
      return InRamCache;
    }
  }

  // Check Disk Cache
  QString path = id;
  if (path.startsWith("file:", Qt::CaseInsensitive)) {
    QUrl url(path);
    if (url.isLocalFile()) path = url.toLocalFile();
  }
  if (path.startsWith("/") && path.length() > 2 && path[2] == ':') {
    path = path.mid(1);
  }
  path = QDir::toNativeSeparators(path);

  if (!FileCacheManager::instance().getCachedData(path, size).isEmpty()) {
    return OnDisk;
  }
  return NotAvailable;
}

QVariantMap AsyncImageProvider::getCacheStats() {
  QMutexLocker locker(&m_mutex);
  QVariantMap stats;
  stats["totalCost"] = m_cache.totalCost();
  stats["maxCost"] = m_cache.maxCost();
  stats["l1Hits"] = s_l1Hits.load(std::memory_order_relaxed);
  stats["l2Hits"] = s_l2Hits.load(std::memory_order_relaxed);
  stats["misses"] = s_misses.load(std::memory_order_relaxed);
  return stats;
}

void AsyncImageProvider::clearCache() {
  QMutexLocker locker(&m_mutex);
  m_cache.clear();
  qDebug() << "[AsyncImageProvider] RAM cache cleared.";
}

void AsyncImageProvider::processImageTask(
    QString id, QSize requestedSize,
    std::shared_ptr<std::atomic<bool>> cancelled, AsyncImageResponse *response,
    bool isLowPriority) {

  // Clean UNC network paths and file:/// URIs
  QString path = id;
  int qIdx = path.indexOf('?');
  if (qIdx != -1) {
    path = path.left(qIdx);
  }

  if (path.startsWith("file:", Qt::CaseInsensitive)) {
    QUrl url(path);
    if (url.isLocalFile()) {
      path = url.toLocalFile();
    }
  }

  // Handle leading slash for local drive paths (e.g. /C:/Users -> C:/Users)
  if (path.startsWith("/") && path.length() > 2 && path[2] == ':') {
    path = path.mid(1);
  }

  path = QDir::cleanPath(path);
  path = QDir::toNativeSeparators(path);

  QString taskKey = path + "_" + QString::number(requestedSize.width()) + "x" +
                    QString::number(requestedSize.height());

  if (cancelled && cancelled->load() && !isLowPriority) {
    QMutexLocker locker(&m_mutex);
    m_pendingTasks.remove(taskKey);
    return;
  }

  // RAII Guard guarantees m_pendingTasks cleanup and response delivery
  // regardless of how this function exits (cancellation, exceptions, early return).
  struct DeliveryGuard {
    std::function<void(const QImage &)> deliverFn;
    bool delivered = false;

    ~DeliveryGuard() {
      if (!delivered && deliverFn) {
        qWarning() << "[DeliveryGuard] Task scope ended without delivery - triggering fallback cleanup.";
        deliverFn(QImage());
      }
    }
  } guard;

  guard.deliverFn = [taskKey](const QImage &finalImage) {
    QList<QPointer<AsyncImageResponse>> responses;
    {
      QMutexLocker locker(&m_mutex);
      responses = m_pendingTasks.take(taskKey);
    }
    for (QPointer<AsyncImageResponse> r : responses) {
      if (r && !r->m_cancelled->load()) {
        QMetaObject::invokeMethod(r.data(), "handleDone", Qt::QueuedConnection,
                                  Q_ARG(QImage, finalImage));
      }
    }
  };

  auto deliverResult = [&guard](const QImage &finalImage) {
    guard.delivered = true;
    guard.deliverFn(finalImage);
  };

#ifdef Q_OS_WIN
  // Required for QImageReader (WIC) on Windows
  CoInitialize(NULL);
#endif

  // --- Disk Cache Hit Check ---
  QByteArray mmapData =
      FileCacheManager::instance().getCachedData(path, requestedSize);
  if (!mmapData.isEmpty()) {
    QImage cachedImg;
    if (cachedImg.loadFromData(mmapData)) {
      s_l2Hits.fetch_add(1, std::memory_order_relaxed);
      qDebug() << "[Cache] L2 HIT" << QFileInfo(path).fileName() << "("
               << mmapData.size() / 1024 << "KB disk)";
      insertCachedImage(id, cachedImg, requestedSize);
      deliverResult(cachedImg);
#ifdef Q_OS_WIN
      CoUninitialize();
#endif
      return;
    }
  }

  // Pause check: Wait while background tasks are paused (e.g. video playback)
  while (TaskScheduler::instance().isPaused() &&
         TaskScheduler::instance().isRunning()) {
    QThread::msleep(50);
  }

  QImage image;
  bool cacheable = true;
  PassiveReadLatencyGuard::ReadScope latencyScope =
      PassiveReadLatencyGuard::instance().startRead(path,
                                                    QFileInfo(path).size());

  DesktopHelper::FileType type = DesktopHelper::staticGetFileType(path);
  bool isHeic = path.endsWith(".heic", Qt::CaseInsensitive) || path.endsWith(".heif", Qt::CaseInsensitive);
  bool isVideo = (type == DesktopHelper::Video) || isHeic;
  bool isRaw = (type == DesktopHelper::Raw);

  s_misses.fetch_add(1, std::memory_order_relaxed);
  qDebug() << "[Cache] MISS - decoding"
           << (isRaw     ? "RAW"
               : isVideo ? "Video/HEIC"
                         : "JPEG")
           << QFileInfo(path).fileName();

  // --- Loading Logic ---
  if (isRaw) {
    s_rawSemaphore.acquire(1);
    QSemaphoreReleaser releaser(s_rawSemaphore);

    try {
      LibRaw RawProcessor;
      // RAII Cleanup helper
      struct RawCleanup {
        LibRaw &processor;
        ~RawCleanup() { processor.recycle(); }
      } cleanup{RawProcessor};

      // 1. Open File
      if (RawProcessor.open_file(path.toLocal8Bit().constData()) ==
          LIBRAW_SUCCESS) {

        // 2. Try Embedded Preview (FAST)
        bool loaded = false;
        if (s_accelerateRaw) {
          if (RawProcessor.unpack_thumb() == LIBRAW_SUCCESS) {
            libraw_processed_image_t *thumb =
                RawProcessor.dcraw_make_mem_thumb();
            if (thumb) {
              struct ThumbCleanup {
                libraw_processed_image_t *t;
                ~ThumbCleanup() {
                  if (t)
                    LibRaw::dcraw_clear_mem(t);
                }
              } tc{thumb};

              if (thumb->type == LIBRAW_IMAGE_JPEG) {
                image.loadFromData((const uchar *)thumb->data, thumb->data_size,
                                   "JPEG");
                loaded = !image.isNull();
              } else if (thumb->type == LIBRAW_IMAGE_BITMAP) {
                QImage rawImg((const uchar *)thumb->data, thumb->width,
                              thumb->height, thumb->width * 3,
                              QImage::Format_RGB888);
                image = rawImg.convertToFormat(QImage::Format_RGB32);
                loaded = !image.isNull();
              }
            }
          }
        }

        // 3. Fallback: Full Decode (SLOW) or Half Decode (Medium)
        if (!loaded) {
          // OOM PROTECTION & PERFORMANCE
          // Always use half-size decoding for fallback. A full decode of a 40MP
          // RAW takes 3-5 seconds and yields a 12000x8000 image, which we scale
          // down to 4096 anyway. Half-size decoding is 4x faster and yields
          // 6000x4000, which is plenty for 4K displays.
          if (s_accelerateRaw) {
            RawProcessor.imgdata.params.half_size = 1;
            RawProcessor.imgdata.params.use_camera_wb = 1;
            qWarning() << "[LibRaw] Thumbnail failed, forced fallback to "
                          "half-size decode:"
                       << path;
          }

          if (RawProcessor.unpack() == LIBRAW_SUCCESS) {
            RawProcessor.dcraw_process();
            libraw_processed_image_t *img = RawProcessor.dcraw_make_mem_image();
            if (img) {
              if (img->type == LIBRAW_IMAGE_BITMAP && img->colors == 3) {
                if (img->width <= 12000 && img->height <= 12000) {
                  long size = img->width * img->height * 3;
                  if (img->data_size >= size) {
                    QImage rawImg((const uchar *)img->data, img->width,
                                  img->height, img->width * 3,
                                  QImage::Format_RGB888);
                    image = rawImg.convertToFormat(QImage::Format_RGB32);
                  }
                }
              }
              LibRaw::dcraw_clear_mem(img);
            }
          }
        }
      }
    } catch (...) {
      qCritical() << "[CRASH PREVENTED] LibRaw crashed on:" << path;
    }

    if (image.isNull()) {
      QImageReader reader(path);
      if (reader.canRead())
        image = reader.read();
    }
  } else if (isVideo) {
    s_videoSemaphore.acquire(1);
    QSemaphoreReleaser releaser(s_videoSemaphore);

    VideoThumbnailer thumbnailer;
    image = thumbnailer.extractFrame(path, 0, requestedSize, cancelled.get());

    if (image.isNull() && isHeic) {
      QImageReader reader(path);
      if (requestedSize.isValid()) reader.setScaledSize(requestedSize);
      if (reader.canRead()) image = reader.read();
    }

    if (image.isNull()) {
      cacheable = false;
      try {
        QFileIconProvider provider;
        QIcon icon = provider.icon(QFileInfo(path));
        if (!icon.isNull()) {
          QSize s = requestedSize.isValid() ? requestedSize : QSize(256, 256);
          image = icon.pixmap(s).toImage();
        }
      } catch (...) {
        qWarning() << "[AsyncImageProvider] Icon provider failed for" << path;
      }
    }
  } else {
    // Standard Image
    QImageReader reader(path);
    reader.setAutoTransform(true);
    if (requestedSize.isValid()) {
      QSize originalSize = reader.size();
      if (originalSize.isValid()) {
        double scale =
            std::max((double)requestedSize.width() / originalSize.width(),
                     (double)requestedSize.height() / originalSize.height());
        QSize newSize(originalSize.width() * scale,
                      originalSize.height() * scale);
        reader.setScaledSize(newSize);
      } else {
        reader.setScaledSize(requestedSize);
      }
    } else {
      // Full Resolution - Limit to 4K to prevent OOM / massive CPU stall on
      // huge images
      QSize originalSize = reader.size();
      if (originalSize.isValid()) {
        const int maxDim = 4096;
        if (originalSize.width() > maxDim || originalSize.height() > maxDim) {
          QSize scaledSize = originalSize;
          scaledSize.scale(maxDim, maxDim, Qt::KeepAspectRatio);
          reader.setScaledSize(scaledSize);
        }
      }
    }

    if (reader.canRead()) {
      image = reader.read();
    }

    if (image.isNull()) {
      // Fallback: Certain PNGs, palette/transparent images fail when
      // setScaledSize is pre-configured. Re-read full image without
      // setScaledSize (downscaling will happen in post-processing).
      QImageReader fallbackReader(path);
      if (fallbackReader.canRead()) {
        image = fallbackReader.read();
      }
      if (image.isNull()) {
        // Fallback: FFmpeg can decode camera formats that QImageReader cannot (e.g. old-style JPEG, raw streams)
        VideoThumbnailer thumbnailer;
        image = thumbnailer.extractFrame(path, 0, requestedSize, cancelled.get());
        if (image.isNull()) {
          qWarning() << "[AsyncImageProvider] QImageReader read failed:" << path
                     << "Error:" << fallbackReader.errorString();
        }
      }
    }
  }

  PassiveReadLatencyGuard::instance().endRead(latencyScope);

  // --- Post Processing & Downscaling ---
  if (image.isNull()) {
    QSize sz = requestedSize.isValid() ? requestedSize : QSize(200, 200);
    image = QImage(sz, QImage::Format_RGB32);
    image.fill(QColor("#222222"));
    QPainter p(&image);
    p.setPen(QColor("#888888"));
    p.setFont(QFont("Segoe UI", 10, QFont::Bold));
    p.drawText(image.rect(), Qt::AlignCenter, isVideo ? "▶ VID" : "ERR");
    cacheable = true;
  }

  QSettings settings("SamsungClone", "VirtualRotations");
  int virtualRot = settings.value(path, 0).toInt();
  if (virtualRot != 0) {
    QTransform t;
    t.rotate(virtualRot);
    image = image.transformed(t, Qt::FastTransformation);
  }

  // OPTIMIZATION: Aggressive Downscaling
  if (requestedSize.isValid() && !requestedSize.isEmpty()) {
    if (image.width() > requestedSize.width() ||
        image.height() > requestedSize.height()) {
      image = image.scaled(requestedSize, Qt::KeepAspectRatio,
                           Qt::FastTransformation);
    }
  } else {
    // Absolute safety cap for full-screen images to prevent RAM/VRAM exhaustion
    const int maxDim = 4096;
    if (image.width() > maxDim || image.height() > maxDim) {
      image = image.scaled(maxDim, maxDim, Qt::KeepAspectRatio,
                           Qt::FastTransformation);
    }
  }
  AsyncImageProvider::insertCachedImage(id, image, requestedSize);

  // Write to Disk Cache ONLY for thumbnails, not for full resolution images
  if (cacheable && requestedSize.isValid()) {
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "JPG", 85);
    FileCacheManager::instance().registerCachedData(path, requestedSize, ba);
  }

#ifdef Q_OS_WIN
  CoUninitialize();
#endif

  deliverResult(image);
}
