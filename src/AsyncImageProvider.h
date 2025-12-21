#ifndef ASYNCIMAGEPROVIDER_H
#define ASYNCIMAGEPROVIDER_H

#include <QCache>
#include <QDebug>
#include <QImageReader>
#include <QMutex>
#include <QObject>
#include <QQuickAsyncImageProvider>
#include <QQuickImageResponse>
#include <QRunnable>
#include <QSize>
#include <atomic>
#include <memory>

// Safe tracker to prevent background threads from using deleted response
// objects
struct ResponseTracker {
  std::atomic<class AsyncImageResponse *> response;
  explicit ResponseTracker(AsyncImageResponse *r) : response(r) {}
};

class AsyncImageResponse : public QQuickImageResponse, public QRunnable {
  Q_OBJECT
public:
  AsyncImageResponse(const QString &id, const QSize &requestedSize);
  ~AsyncImageResponse() override;
  void run() override;
  QQuickTextureFactory *textureFactory() const override;
  void cancel() override;

public slots:
  void handleDone(QImage image);

public:
  QString m_id;
  QSize m_requestedSize;
  QImage m_image;
  std::shared_ptr<std::atomic<bool>> m_cancelled;
  std::shared_ptr<ResponseTracker> m_tracker;
};

class AsyncImageProvider : public QQuickAsyncImageProvider {
public:
  AsyncImageProvider();

  static std::atomic<bool> s_disableVideo;
  static std::atomic<bool> s_disableRaw;

  QQuickImageResponse *requestImageResponse(const QString &id,
                                            const QSize &requestedSize);

  static QImage getCachedImage(const QString &id, const QSize &size);
  static void insertCachedImage(const QString &id, const QImage &image,
                                const QSize &size);
  static void setCacheMaxCost(int cost);
  static QVariantMap getCacheStats();
  static void clearCache();

  // Internal worker for task scheduling with re-queue support
  static void processImageTask(QString id, QSize requestedSize,
                               std::shared_ptr<std::atomic<bool>> cancelled,
                               std::shared_ptr<ResponseTracker> tracker);

  static std::atomic<int> s_logLevel;
  static std::atomic<bool> s_accelerateRaw;

private:
  static QCache<QString, QImage> m_cache;
  static QMutex m_mutex;
};

#endif // ASYNCIMAGEPROVIDER_H
