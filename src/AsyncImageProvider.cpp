#include "AsyncImageProvider.h"
#include <QDir>
#include <QElapsedTimer>
#include <QImageReader>
#include <QSettings>
#include <QThread>
#include <QVariant>
#include <algorithm>


// Initialize static members
QCache<QString, QImage> AsyncImageProvider::m_cache;
QMutex AsyncImageProvider::m_mutex;
QThreadPool *AsyncImageProvider::m_threadPool = nullptr;
std::atomic<int> AsyncImageProvider::s_requestCounter(0);

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
  if (*m_cancelled) {
    m_signals->deleteLater();
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
    return;
  }

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
  } else {
    reader.setScaledSize(QSize(500, 500));
  }

  if (*m_cancelled) {
    m_signals->deleteLater();
    return;
  }

  QElapsedTimer timer;
  timer.start();

  QImage image;
  if (reader.canRead()) {
    image = reader.read();
    // qDebug() << "[PERF][DECODE]" << path << image.size() << timer.elapsed()
    // << "ms";

    // Insert into Cache
    AsyncImageProvider::insertCachedImage(m_id, image);
  } else {
    // Return placeholder
    image = QImage(100, 100, QImage::Format_RGB32);
    image.fill(Qt::gray);
    qWarning() << "[PERF][FAIL]" << path << reader.errorString();
  }

  if (!*m_cancelled) {
    emit m_signals->done(image);
  }
  m_signals->deleteLater(); // Schedule deletion in main thread
}

// AsyncImageProvider Implementation

QQuickImageResponse *
AsyncImageProvider::requestImageResponse(const QString &id,
                                         const QSize &requestedSize) {
  auto *response = new AsyncImageResponse(id, requestedSize);

  // Check Cache Synchronously
  QImage cached = getCachedImage(id);
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

  if (m_threadPool) {
    m_threadPool->start(runnable, ++s_requestCounter);
  } else {
    QThreadPool::globalInstance()->start(runnable, ++s_requestCounter);
  }
  return response;
}

QImage AsyncImageProvider::getCachedImage(const QString &id) {
  QMutexLocker locker(&m_mutex);
  if (m_cache.contains(id)) {
    return *m_cache.object(id);
  }
  return QImage();
}

void AsyncImageProvider::insertCachedImage(const QString &id,
                                           const QImage &image) {
  QMutexLocker locker(&m_mutex);
  m_cache.insert(id, new QImage(image), image.sizeInBytes() / 1024);
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
