#ifndef SETTINGSHELPER_H
#define SETTINGSHELPER_H

#include <QObject>
#include <QSettings>
#include <QVariantMap>

class SettingsHelper : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString graphicsApi READ graphicsApi NOTIFY graphicsApiChanged)
  Q_PROPERTY(
      QString graphicsDriver READ graphicsDriver NOTIFY graphicsDriverChanged)
  Q_PROPERTY(QString graphicsProfile READ graphicsProfile NOTIFY
                 graphicsProfileChanged)
  Q_PROPERTY(int selectedApi READ selectedApi WRITE setSelectedApi NOTIFY
                 selectedApiChanged)

  // Tunable Settings
  Q_PROPERTY(int thumbnailSize READ thumbnailSize WRITE setThumbnailSize NOTIFY
                 thumbnailSizeChanged)
  Q_PROPERTY(
      int gridSize READ gridSize WRITE setGridSize NOTIFY gridSizeChanged)
  Q_PROPERTY(int cacheSizeMB READ cacheSizeMB WRITE setCacheSizeMB NOTIFY
                 cacheSizeMBChanged)
  Q_PROPERTY(int concurrentThreads READ concurrentThreads WRITE
                 setConcurrentThreads NOTIFY concurrentThreadsChanged)

public:
  explicit SettingsHelper(QObject *parent = nullptr);

  QString graphicsApi() const;
  QString graphicsDriver() const;
  QString graphicsProfile() const;

  int selectedApi() const;
  void setSelectedApi(int api);

  int thumbnailSize() const;
  void setThumbnailSize(int size);

  int gridSize() const;
  void setGridSize(int size);

  int cacheSizeMB() const;
  void setCacheSizeMB(int sizeMB);

  int concurrentThreads() const;
  void setConcurrentThreads(int count);

  Q_INVOKABLE void restartApp();
  Q_INVOKABLE bool isApiSupported(int apiValue);
  Q_INVOKABLE QVariantMap getCacheStats();

  // Placeholder for GPU info (implementation moved to SystemMonitor or
  // simplified)
  Q_INVOKABLE QString getGpuName(QObject *window = nullptr) {
    return "GPU Detection Simplified";
  }
  Q_INVOKABLE void refreshGraphicsInfo(QObject *window = nullptr) {}

signals:
  void graphicsApiChanged();
  void graphicsDriverChanged();
  void graphicsProfileChanged();
  void selectedApiChanged();
  void thumbnailSizeChanged();
  void gridSizeChanged();
  void cacheSizeMBChanged();
  void concurrentThreadsChanged();

private:
  QString m_graphicsApi;
  QString m_graphicsDriver;
  QString m_graphicsProfile;
  QSettings m_settings;
};

#endif // SETTINGSHELPER_H
