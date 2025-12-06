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

class AsyncImageResponse : public QQuickImageResponse {
  Q_OBJECT
public:
  AsyncImageResponse(const QString &id, const QSize &requestedSize);
  QQuickTextureFactory *textureFactory() const override;
  void cancel() override;

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

  static QImage getCachedImage(const QString &id, const QSize &size);
  static void insertCachedImage(const QString &id, const QImage &image,
                                const QSize &size);
  static void setCacheMaxCost(int cost);
  static QVariantMap getCacheStats();
  static void clearCache();

  // Internal worker for task scheduling with re-queue support
  static void processImageTask(QString id, QSize requestedSize,
                               std::shared_ptr<std::atomic<bool>> cancelled,
                               AsyncImageResponse *response);

  static std::atomic<int> s_logLevel;
  static std::atomic<bool> s_accelerateRaw;

private:
  static QCache<QString, QImage> m_cache;
  static QMutex m_mutex;
};

#endif // ASYNCIMAGEPROVIDER_H
