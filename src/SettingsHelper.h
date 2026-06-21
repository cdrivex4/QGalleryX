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
  Q_PROPERTY(
      int logLevel READ logLevel WRITE setLogLevel NOTIFY logLevelChanged)
  Q_PROPERTY(bool rawAcceleration READ rawAcceleration WRITE setRawAcceleration
                 NOTIFY rawAccelerationChanged)
  Q_PROPERTY(bool useDiskCache READ useDiskCache WRITE setUseDiskCache NOTIFY
                 useDiskCacheChanged)
  Q_PROPERTY(int videoAcceleration READ videoAcceleration WRITE
                 setVideoAcceleration NOTIFY videoAccelerationChanged)
  Q_PROPERTY(bool showWatermark READ showWatermark WRITE setShowWatermark NOTIFY
                 showWatermarkChanged)
  Q_PROPERTY(bool showDiagnostics READ showDiagnostics WRITE setShowDiagnostics NOTIFY
                 showDiagnosticsChanged)
  Q_PROPERTY(bool useFastImage READ useFastImage WRITE setUseFastImage NOTIFY
                 useFastImageChanged)

public:
  enum HWAccel { None, Auto, CUDA, QSV, D3D11VA, DXVA2, Vulkan, OpenCL };
  Q_ENUM(HWAccel)

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

  int logLevel() const;
  void setLogLevel(int level);

  bool rawAcceleration() const;
  void setRawAcceleration(bool enable);

  bool useDiskCache() const;
  void setUseDiskCache(bool enable);

  int videoAcceleration() const;
  void setVideoAcceleration(int mode);

  bool showWatermark() const;
  void setShowWatermark(bool show);

  bool showDiagnostics() const;
  void setShowDiagnostics(bool show);

  bool useFastImage() const;
  void setUseFastImage(bool use);

  Q_INVOKABLE void restartApp();
  Q_INVOKABLE bool isApiSupported(int apiValue);
  Q_INVOKABLE QVariantMap getCacheStats();
  Q_INVOKABLE void clearDiskCache();

  Q_INVOKABLE QString getGpuName(QObject *window = nullptr);
  Q_INVOKABLE void refreshGraphicsInfo(QObject *window = nullptr);

signals:
  void graphicsApiChanged();
  void graphicsDriverChanged();
  void graphicsProfileChanged();
  void selectedApiChanged();
  void thumbnailSizeChanged();
  void gridSizeChanged();
  void cacheSizeMBChanged();
  void concurrentThreadsChanged();
  void logLevelChanged();
  void rawAccelerationChanged();
  void useDiskCacheChanged();
  void videoAccelerationChanged();
  void showWatermarkChanged();
  void showDiagnosticsChanged();
  void useFastImageChanged();

private:
  QString m_graphicsApi;
  QString m_graphicsDriver;
  QString m_graphicsProfile;
  QSettings m_settings;
};

#endif // SETTINGSHELPER_H
