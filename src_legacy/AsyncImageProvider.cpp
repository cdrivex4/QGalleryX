#include "AsyncImageProvider.h"
#include "DesktopHelper.h"
#include "../src/PassiveReadLatencyGuard.h"
#include "../src/FileCacheManager.h"
#include <QBuffer>
#include "SystemMonitor.h"
#include "TaskScheduler.h"
#include "VideoThumbnailer.h"
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
std::atomic<int> AsyncImageProvider::s_logLevel(0);
std::atomic<bool> AsyncImageProvider::s_accelerateRaw(true);

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

QQuickImageResponse *
AsyncImageProvider::requestImageResponse(const QString &id,
                                         const QSize &requestedSize) {
  auto *response = new AsyncImageResponse(id, requestedSize);

  // Check Cache Synchronously
  QImage cached = getCachedImage(id, requestedSize);
  if (!cached.isNull()) {
    QMetaObject::invokeMethod(response, "handleDone", Qt::QueuedConnection,
                              Q_ARG(QImage, cached));
    return response;
  }

  // --- PRIORITIZATION LOGIC ---
  // Viewer requests (invalid size or large) -> Immediate
  // Grid requests (small size) -> Low
  TaskScheduler::Priority priority = TaskScheduler::Low;
  if (!requestedSize.isValid() ||
      (requestedSize.width() > 1000 || requestedSize.height() > 1000)) {
    priority = TaskScheduler::Immediate;
  }

  std::shared_ptr<std::atomic<bool>> cancelled = response->m_cancelled;

  TaskScheduler::instance().addTask(
      [id, requestedSize, cancelled, response]() {
        AsyncImageProvider::processImageTask(id, requestedSize, cancelled,
                                             response);
      },
      TaskScheduler::CPU_BOUND, priority);

  return response;
}

QImage AsyncImageProvider::getCachedImage(const QString &id,
                                          const QSize &size) {
  QMutexLocker locker(&m_mutex);
  QString key = id + "_" + QString::number(size.width()) + "x" +
                QString::number(size.height());
  if (m_cache.contains(key)) {
    return *m_cache.object(key);
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
  qWarning() << "[ResourceControl] System Memory > 2.5GB. Cache Cleared.";
}

void AsyncImageProvider::processImageTask(
    QString id, QSize requestedSize,
    std::shared_ptr<std::atomic<bool>> cancelled,
    AsyncImageResponse *response) {

  // 1. Check Cancellation (Early Exit)
  if (*cancelled)
    return;

  // --- MEMORY LIMIT ENFORCEMENT (User Request: 2.5GB Max) ---
  // (REVERTED due to Lock Contention)

#ifdef Q_OS_WIN
  // Required for QImageReader (WIC) on Windows
  CoInitialize(NULL);
#endif

  // Handle "file:///" via QUrl
  QString path;
  QUrl url(id);
  if (url.isValid() && url.isLocalFile()) {
    path = url.toLocalFile();
  } else {
    path = id;
  }

  // Handle odd edge case: /C:/Users...
  if (path.startsWith("/") && path.length() > 2 && path[2] == ':') {
    path = path.mid(1);
  }

  path = QDir::toNativeSeparators(path);

  // --- Disk Cache Hit Check ---
  QByteArray mmapData = FileCacheManager::instance().getCachedData(path, requestedSize);
  if (!mmapData.isEmpty()) {
    QImage cachedImg;
    if (cachedImg.loadFromData(mmapData)) {
      insertCachedImage(id, cachedImg, requestedSize);
      QMetaObject::invokeMethod(response, "handleDone", Qt::QueuedConnection,
                                Q_ARG(QImage, cachedImg));
#ifdef Q_OS_WIN
      CoUninitialize();
#endif
      return;
    }
  }

  if (*cancelled) {
#ifdef Q_OS_WIN
    CoUninitialize();
#endif
    return;
  }

  QImage image;
  PassiveReadLatencyGuard::ReadScope latencyScope =
      PassiveReadLatencyGuard::instance().startRead(path, QFileInfo(path).size());

  DesktopHelper::FileType type = DesktopHelper::staticGetFileType(path);
  bool isVideo = (type == DesktopHelper::Video) || path.endsWith(".heic", Qt::CaseInsensitive) || path.endsWith(".heif", Qt::CaseInsensitive);
  bool isRaw = (type == DesktopHelper::Raw);

  // --- Loading Logic ---
  if (isRaw) {
    // NON-BLOCKING CHECK
    if (!s_rawSemaphore.tryAcquire(1)) {
      // BUSY LOOP PREVENTION
      QThread::msleep(20);
      TaskScheduler::instance().addTask(
          [=]() { processImageTask(id, requestedSize, cancelled, response); },
          TaskScheduler::CPU_BOUND, TaskScheduler::Low);
#ifdef Q_OS_WIN
      CoUninitialize();
#endif
      return;
    }

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
                image = rawImg.copy();
                loaded = !image.isNull();
              }
            }
          }
        }

        // 3. Fallback: Full Decode (SLOW) or Half Decode (Medium)
        if (!loaded) {
          // OOM PROTECTION & PERFORMANCE
          bool isThumbnail =
              (requestedSize.width() > 0 && requestedSize.width() < 1000) ||
              (requestedSize.height() > 0 && requestedSize.height() < 1000);

          if (s_accelerateRaw && isThumbnail) {
            // Enable half-size decoding for faster fallback
            RawProcessor.imgdata.params.half_size = 1;
          } else if (s_accelerateRaw) {
            qWarning()
                << "[LibRaw] Thumbnail failed, forced fallback to full decode:"
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
                    image = rawImg.copy();
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
    s_rawSemaphore.release(1);

    if (image.isNull()) {
      QImageReader reader(path);
      if (reader.canRead())
        image = reader.read();
    }
  } else if (isVideo) {
    // NON-BLOCKING VIDEO CHECK
    if (!s_videoSemaphore.tryAcquire(1)) {
      QThread::msleep(20);
      TaskScheduler::instance().addTask(
          [=]() { processImageTask(id, requestedSize, cancelled, response); },
          TaskScheduler::CPU_BOUND, TaskScheduler::Low);
#ifdef Q_OS_WIN
      CoUninitialize();
#endif
      return;
    }

    VideoThumbnailer thumbnailer;
    image = thumbnailer.extractFrame(path, 0, requestedSize, cancelled.get());
    s_videoSemaphore.release(1);

    if (image.isNull()) {
      QFileIconProvider provider;
      QIcon icon = provider.icon(QFileInfo(path));
      if (!icon.isNull()) {
        QSize s = requestedSize.isValid() ? requestedSize : QSize(256, 256);
        image = icon.pixmap(s).toImage();
      }
    }
  } else {
    // Standard Image
    QImageReader reader(path);
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
    }
    if (reader.canRead()) {
      image = reader.read();
      if (image.isNull()) {
        qWarning() << "[AsyncImageProvider] QImageReader read failed:" << path
                   << "Error:" << reader.errorString();
      }
    } else {
      qWarning()
          << "[AsyncImageProvider] QImageReader canRead() returned false:"
          << path << "Error:" << reader.errorString()
          << "Formats:" << QImageReader::supportedImageFormats();
    }
  }

  PassiveReadLatencyGuard::instance().endRead(latencyScope);

  // --- Post Processing & Downscaling ---
  if (!image.isNull()) {
    // OPTIMIZATION: Aggressive Downscaling
    if (requestedSize.isValid() && !requestedSize.isEmpty()) {
      if (image.width() > requestedSize.width() ||
          image.height() > requestedSize.height()) {
        image = image.scaled(requestedSize, Qt::KeepAspectRatio,
                             Qt::SmoothTransformation);
      }
    }
    AsyncImageProvider::insertCachedImage(id, image, requestedSize);

    // Write to Disk Cache
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "JPG", 85);
    FileCacheManager::instance().registerCachedData(path, requestedSize, ba);

    QMetaObject::invokeMethod(response, "handleDone", Qt::QueuedConnection,
                              Q_ARG(QImage, image));
  } else if (!isVideo) {
    // Placeholder
    image = QImage(100, 100, QImage::Format_RGB32);
    image.fill(Qt::yellow);
    QPainter p(&image);
    p.drawText(image.rect(), Qt::AlignCenter, "ERR");
  }

#ifdef Q_OS_WIN
  CoUninitialize();
#endif

  if (!*cancelled) {
    // Deliver result to main thread
    QMetaObject::invokeMethod(response, "handleDone", Qt::QueuedConnection,
                              Q_ARG(QImage, image));
  }
}
