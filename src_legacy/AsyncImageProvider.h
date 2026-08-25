#ifndef ASYNCIMAGEPROVIDER_H
#define ASYNCIMAGEPROVIDER_H

#include <QCache>
#include <QDebug>
#include <QImageReader>
#include <QMutex>
#include <QObject>
#include <QQuickAsyncImageProvider>
#include <QRunnable>
#include <QThreadPool>
#include <atomic>
#include <memory>
#include <QPointer>

class AsyncImageResponse : public QQuickImageResponse {
  Q_OBJECT
public:
  AsyncImageResponse(const QString &id, const QSize &requestedSize);
  ~AsyncImageResponse() override;
  QQuickTextureFactory *textureFactory() const override;
  void cancel() override;
  QImage image() const { return m_image; }

public slots:
  void handleDone(QImage image);

  friend class AsyncImageProvider;

private:
  QString m_id;
  QSize m_requestedSize;
  QImage m_image;
  std::shared_ptr<std::atomic<bool>> m_cancelled;
};

class AsyncImageProvider : public QQuickAsyncImageProvider {
public:
  QQuickImageResponse *
  requestImageResponse(const QString &id, const QSize &requestedSize) override;

  static void unregisterResponse(AsyncImageResponse *response);

  enum CacheLevel { 
      NotCached = 0, 
      InRamCache = 1,
      OnDisk = 2,
      NotAvailable = 3
  };

  static CacheLevel checkCacheLevel(const QString &id, const QSize &size);

  static QImage getCachedImage(const QString &id, const QSize &size);
  static void insertCachedImage(const QString &id, const QImage &image,
                                const QSize &size);
  static void setCacheMaxCost(int cost);
  static QVariantMap getCacheStats();
  static void clearCache();

  // Internal worker for task scheduling with re-queue support
  static void processImageTask(QString id, QSize requestedSize,
                               std::shared_ptr<std::atomic<bool>> cancelled,
                               AsyncImageResponse *response,
                               bool isLowPriority = false);

  // Background-only crawler: decodes directly to L2 disk cache.
  // NEVER touches m_pendingTasks. Safe to call from any thread.
  static void crawlDecodeToL2(const QString &path, const QSize &requestedSize);
  static void promoteL2ToL1(const QString &path, const QSize &requestedSize);

  static std::atomic<int> s_logLevel;
  static std::atomic<bool> s_accelerateRaw;
  static std::atomic<int> s_l1Hits;
  static std::atomic<int> s_l2Hits;
  static std::atomic<int> s_misses;

private:
  static QCache<QString, QImage> m_cache;
  static QMutex m_mutex;
  static QHash<QString, QList<QPointer<AsyncImageResponse>>> m_pendingTasks;
};

#endif // ASYNCIMAGEPROVIDER_H
