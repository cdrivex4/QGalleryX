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
  int m_workDuration = 0; // Pure decoding time
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
  static void clearDiskCache();
  static void setFrameScheduler(class FrameBudgetScheduler *s);

  // STAGING QUEUE for "Ground Up" Prioritization
  struct StagedRequest {
    QString id;
    QSize requestedSize;
    QDateTime timestamp;
    std::shared_ptr<std::atomic<bool>> cancelled;
    std::shared_ptr<ResponseTracker> tracker;
  };

  static void processStagedRequests();

  // Internal worker for task scheduling with re-queue support
  static void processImageTask(QString id, QSize requestedSize,
                               std::shared_ptr<std::atomic<bool>> cancelled,
                               std::shared_ptr<ResponseTracker> tracker);

  static bool isRequestStillNeeded(const QString &id);
  static void deliverToPending(const QString &id, const QImage &image,
                               int duration);
  static void processImageTaskInternal(
      QString id, QSize requestedSize,
      std::shared_ptr<std::atomic<bool>> cancelled = nullptr,
      std::shared_ptr<ResponseTracker> tracker = nullptr);

  static std::atomic<int> s_logLevel;
  static std::atomic<bool> s_accelerateRaw;
  static std::atomic<bool> s_useDiskCache;
  static std::atomic<int> s_videoAcceleration;

private:
  friend class AsyncImageResponse;
  static QCache<QString, QImage> m_cache;
  static QMutex m_mutex;

  // Coalescing: Track active tasks to avoid duplicate work
  static QMap<QString, QList<AsyncImageResponse *>> m_pendingResponses;
  static QMutex m_pendingMutex;

  // Staging Queue Members
  static QList<StagedRequest> m_stagedRequests;
  static QMutex m_stagingMutex;
  static void queueRequest(const QString &id, const QSize &size,
                           std::shared_ptr<std::atomic<bool>> c,
                           std::shared_ptr<ResponseTracker> t);
  static void scheduleStagingProcessing();
  static class FrameBudgetScheduler *s_frameScheduler;
};

#endif // ASYNCIMAGEPROVIDER_H
