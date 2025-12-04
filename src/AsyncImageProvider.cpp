#include "AsyncImageProvider.h"
#include <QDir>
#include <QElapsedTimer>
#include <QFileIconProvider>
#include <QIcon>
#include <QImageReader>
#include <QPainter>
#include <QSettings>
#include <QThread>
#include <QVariant>
#include <algorithm>

#ifdef Q_OS_WIN
#include <objbase.h>
#endif

#include <QtCore/QEventLoop>
#include <QtCore/QSemaphore>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtMultimedia/QAudioOutput>
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>
#include <libraw/libraw.h>

// Initialize static members
static QSemaphore s_videoSemaphore(1); // Limit to 1 concurrent video extraction
QCache<QString, QImage> AsyncImageProvider::m_cache;
QMutex AsyncImageProvider::m_mutex;
QThreadPool *AsyncImageProvider::m_threadPool = nullptr;
std::atomic<int> AsyncImageProvider::s_requestCounter(0);
std::atomic<int> AsyncImageProvider::s_logLevel(0);

AsyncImageResponse::AsyncImageResponse(const QString &id,
                                       const QSize &requestedSize)
    : m_id(id), m_requestedSize(requestedSize),
      m_cancelled(std::make_shared<std::atomic<bool>>(false)) {
  // Configure thread pool once
  if (!AsyncImageProvider::m_threadPool) {
    AsyncImageProvider::m_threadPool = new QThreadPool();

    // Read thread count from settings, default to ideal - 2
    QSettings settings("SamsungClone", "Gallery");
    int configuredThreads = settings.value("concurrentThreads", 0).toInt();

    int threads = configuredThreads;
    if (threads <= 0) {
      threads = QThread::idealThreadCount() - 2;
      if (threads < 1)
        threads = 1;
    }

    AsyncImageProvider::m_threadPool->setMaxThreadCount(threads);
    qDebug() << "[PERF] Thread Pool Initialized with" << threads << "threads";

    // Set Cache Size from settings or default
    int cacheSizeMB = settings.value("cacheSizeMB", 512).toInt();
    AsyncImageProvider::setCacheMaxCost(cacheSizeMB * 1024);
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

// ImageLoaderRunnable Implementation

ImageLoaderRunnable::ImageLoaderRunnable(
    const QString &id, const QSize &requestedSize, WorkerSignals *workerSignals,
    std::shared_ptr<std::atomic<bool>> cancelled)
    : m_id(id), m_requestedSize(requestedSize), m_signals(workerSignals),
      m_cancelled(cancelled) {
  setAutoDelete(true);
}

ImageLoaderRunnable::~ImageLoaderRunnable() {
  // Do not delete m_signals here, it is owned by main thread
}

void ImageLoaderRunnable::run() {
  QElapsedTimer timer;
  timer.start();

// Initialize COM for Windows Shell operations (QFileIconProvider)
#ifdef Q_OS_WIN
  CoInitialize(NULL);
#endif

  if (*m_cancelled) {
    m_signals->deleteLater();
#ifdef Q_OS_WIN
    CoUninitialize();
#endif
    return;
  }

  // Handle "file:///" prefix if present (QML sends it)
  QString path = m_id;
  if (path.startsWith("file:///"))
    path = path.mid(8);
  else if (path.startsWith("file:"))
    path = path.mid(5);

  // Windows path fix
  if (path.startsWith("/") && path.contains(":"))
    path = path.mid(1);

  if (*m_cancelled) {
    m_signals->deleteLater();
#ifdef Q_OS_WIN
    CoUninitialize();
#endif
    return;
  }

  QImage image;
  bool isVideo = path.endsWith(".mp4", Qt::CaseInsensitive) ||
                 path.endsWith(".mkv", Qt::CaseInsensitive) ||
                 path.endsWith(".avi", Qt::CaseInsensitive) ||
                 path.endsWith(".mov", Qt::CaseInsensitive);

  bool isRaw = path.endsWith(".arw", Qt::CaseInsensitive) ||
               path.endsWith(".cr2", Qt::CaseInsensitive) ||
               path.endsWith(".dng", Qt::CaseInsensitive) ||
               path.endsWith(".nef", Qt::CaseInsensitive);

  if (isRaw) {
    LibRaw RawProcessor;
    // LibRaw expects file path in local 8-bit encoding (usually)
    if (RawProcessor.open_file(path.toLocal8Bit().constData()) ==
        LIBRAW_SUCCESS) {
      // Unpack the raw data
      if (RawProcessor.unpack() == LIBRAW_SUCCESS) {
        // Process data (demosaic, etc.) - default settings
        RawProcessor.dcraw_process();

        // Create memory image
        libraw_processed_image_t *img = RawProcessor.dcraw_make_mem_image();
        if (img) {
          if (img->type == LIBRAW_IMAGE_BITMAP && img->colors == 3) {
            // LibRaw outputs RGB 8-bit per channel packed
            long size = img->width * img->height * 3;
            if (img->data_size >= size) {
              // Create QImage wrapper around data
              QImage rawImg((const uchar *)img->data, img->width, img->height,
                            img->width * 3, QImage::Format_RGB888);
              image = rawImg.copy(); // Deep copy to own the data
            }
          }
          LibRaw::dcraw_clear_mem(img);
        }
      }
    }

    // Fallback: If LibRaw failed, try QImageReader (might find embedded
    // preview)
    if (image.isNull()) {
      QImageReader reader(path);
      if (reader.canRead()) {
        image = reader.read();
      }
    }
  } else if (isVideo) {
    // Try to extract frame using QMediaPlayer (Limit concurrency)
    if (s_videoSemaphore.tryAcquire(1, 50)) {
      QEventLoop loop;
      QMediaPlayer player;
      QAudioOutput audio;
      QVideoSink sink;

      audio.setVolume(0);
      audio.setMuted(true);
      player.setAudioOutput(&audio);
      player.setVideoOutput(&sink);
      player.setSource(QUrl::fromLocalFile(path));

      QObject::connect(&sink, &QVideoSink::videoFrameChanged, &loop,
                       [&](const QVideoFrame &frame) {
                         if (frame.isValid()) {
                           image = frame.toImage();
                           loop.quit();
                         }
                       });

      QObject::connect(
          &player, &QMediaPlayer::errorOccurred, &loop,
          [&](QMediaPlayer::Error, const QString &err) { loop.quit(); });

      // Timeout (1.5s)
      QTimer::singleShot(1500, &loop, &QEventLoop::quit);

      // Seek to 5s
      player.setPosition(5000);
      player.play();
      loop.exec();

      s_videoSemaphore.release();
    }

    // Fallback to system icon if extraction failed
    if (image.isNull()) {
      QFileIconProvider provider;
      QIcon icon = provider.icon(QFileInfo(path));
      QSize size =
          m_requestedSize.isValid() ? m_requestedSize : QSize(256, 256);

      if (!icon.isNull()) {
        image = icon.pixmap(size).toImage();
      }
    }

    // Fallback if system icon is empty or too small/generic (heuristic?)
    // For now, if image is null, draw placeholder
    if (image.isNull()) {
      QSize size =
          m_requestedSize.isValid() ? m_requestedSize : QSize(256, 256);
      image = QImage(size, QImage::Format_ARGB32);
      image.fill(QColor(40, 40, 40));

      QPainter painter(&image);
      painter.setRenderHint(QPainter::Antialiasing);
      painter.setBrush(Qt::white);
      painter.setPen(Qt::NoPen);

      int s = std::min(size.width(), size.height()) / 4;
      int cx = size.width() / 2;
      int cy = size.height() / 2;

      QPoint points[3] = {QPoint(cx - s / 2, cy - s / 2),
                          QPoint(cx - s / 2, cy + s / 2),
                          QPoint(cx + s / 2, cy)};
      painter.drawPolygon(points, 3);
      painter.end();
    }
  } else {
    QImageReader reader(path);

    // Optimize: Scale during decode if size requested
    if (m_requestedSize.isValid()) {
      QSize originalSize = reader.size();
      if (originalSize.isValid()) {
        double scale =
            std::max((double)m_requestedSize.width() / originalSize.width(),
                     (double)m_requestedSize.height() / originalSize.height());

        QSize newSize(originalSize.width() * scale,
                      originalSize.height() * scale);

        reader.setScaledSize(newSize);
      } else {
        reader.setScaledSize(m_requestedSize);
      }
    }

    if (reader.canRead()) {
      image = reader.read();
    } else {
      qWarning() << "[PERF][FAIL]" << path << reader.errorString();
    }
  }

  if (!image.isNull()) {
    // Log if verbose
    if (AsyncImageProvider::s_logLevel >= 2) {
      qDebug() << "[PERF][DECODE]" << path << image.size() << timer.elapsed()
               << "ms";
    }

    // Insert into Cache
    AsyncImageProvider::insertCachedImage(m_id, image, m_requestedSize);
  } else if (!isVideo) { // Only fallback if not video (video already handled or
                         // failed)
    // Return placeholder for failed images
    image = QImage(100, 100, QImage::Format_RGB32);
    image.fill(Qt::gray);
  }

  if (!*m_cancelled) {
    emit m_signals->done(image);
  }
  m_signals->deleteLater(); // Schedule deletion in main thread

#ifdef Q_OS_WIN
  CoUninitialize();
#endif
}

// AsyncImageProvider Implementation

QQuickImageResponse *
AsyncImageProvider::requestImageResponse(const QString &id,
                                         const QSize &requestedSize) {
  auto *response = new AsyncImageResponse(id, requestedSize);

  // Check Cache Synchronously
  QImage cached = getCachedImage(id, requestedSize);
  if (!cached.isNull()) {
    // Emit finished asynchronously to ensure QML has time to connect to the
    // signal
    QMetaObject::invokeMethod(response, "handleDone", Qt::QueuedConnection,
                              Q_ARG(QImage, cached));
    return response;
  }

  auto *workerSignals = new WorkerSignals();
  // Pass the cancellation token from the response to the runnable
  auto *runnable = new ImageLoaderRunnable(id, requestedSize, workerSignals,
                                           response->m_cancelled);

  // Connect signal from runnable to response
  // We use Qt::QueuedConnection because they are in different threads
  QObject::connect(workerSignals, &WorkerSignals::done, response,
                   &AsyncImageResponse::handleDone, Qt::QueuedConnection);

  // Use request counter for priority (LIFO)
  // This ensures that the most recently requested images (visible ones)
  // are processed first, jumping ahead of the queue.
  int priority = s_requestCounter++;

  // Prioritize full-size images (PhotoViewer) over thumbnails (GalleryView)
  // If requestedSize is invalid, it means full size is requested.
  if (!requestedSize.isValid()) {
    priority = 2147483647; // INT_MAX
  }

  if (m_threadPool) {
    m_threadPool->start(runnable, priority);
  } else {
    QThreadPool::globalInstance()->start(runnable, priority);
  }
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
