#ifndef DIAGNOSTICSMONITOR_H
#define DIAGNOSTICSMONITOR_H

#include <QDateTime>
#include <QObject>
#include <QTimer>


class ScrollBenchImageModel;
class AsyncImageProvider;
class SettingsHelper;

// Real-time diagnostics and validation system
class DiagnosticsMonitor : public QObject {
  Q_OBJECT

  // Overall system health
  Q_PROPERTY(QString healthStatus READ healthStatus NOTIFY healthChanged)
  Q_PROPERTY(QString healthColor READ healthColor NOTIFY healthChanged)

  // Viewport Culling Diagnostics
  Q_PROPERTY(bool viewportCullingEnabled READ viewportCullingEnabled NOTIFY
                 viewportCullingChanged)
  Q_PROPERTY(
      int viewportRangeSize READ viewportRangeSize NOTIFY viewportRangeChanged)
  Q_PROPERTY(
      int bufferedRangeSize READ bufferedRangeSize NOTIFY viewportRangeChanged)
  Q_PROPERTY(
      QString viewportStatus READ viewportStatus NOTIFY viewportStatusChanged)
  Q_PROPERTY(QString viewportStatusColor READ viewportStatusColor NOTIFY
                 viewportStatusChanged)

  // Load Progress Diagnostics
  Q_PROPERTY(int totalItems READ totalItems NOTIFY totalItemsChanged)
  Q_PROPERTY(int loadedItems READ loadedItems NOTIFY loadedItemsChanged)
  Q_PROPERTY(
      int pendingRequests READ pendingRequests NOTIFY pendingRequestsChanged)
  Q_PROPERTY(
      int stagedRequests READ stagedRequests NOTIFY stagedRequestsChanged)
  Q_PROPERTY(QString loadProgressStatus READ loadProgressStatus NOTIFY
                 loadProgressChanged)

  // Settings Synchronization
  Q_PROPERTY(
      bool diskCacheEnabled READ diskCacheEnabled NOTIFY diskCacheChanged)
  Q_PROPERTY(
      QString settingsStatus READ settingsStatus NOTIFY settingsStatusChanged)
  Q_PROPERTY(QString settingsStatusColor READ settingsStatusColor NOTIFY
                 settingsStatusChanged)

  // Adaptive I/O Diagnostics
  Q_PROPERTY(int activeIOTasks READ activeIOTasks NOTIFY ioTasksChanged)
  Q_PROPERTY(QString ioStatus READ ioStatus NOTIFY ioStatusChanged)

  // Anomaly Detection
  Q_PROPERTY(
      QStringList activeWarnings READ activeWarnings NOTIFY warningsChanged)
  Q_PROPERTY(
      QStringList activeCriticals READ activeCriticals NOTIFY criticalsChanged)

public:
  explicit DiagnosticsMonitor(QObject *parent = nullptr);

  // Attach to components for monitoring
  void attachModel(ScrollBenchImageModel *model);
  void attachSettings(SettingsHelper *settings);

  // Getters
  QString healthStatus() const { return m_healthStatus; }
  QString healthColor() const { return m_healthColor; }

  bool viewportCullingEnabled() const { return m_viewportCullingEnabled; }
  int viewportRangeSize() const { return m_viewportRangeSize; }
  int bufferedRangeSize() const { return m_bufferedRangeSize; }
  QString viewportStatus() const { return m_viewportStatus; }
  QString viewportStatusColor() const { return m_viewportStatusColor; }

  int totalItems() const { return m_totalItems; }
  int loadedItems() const { return m_loadedItems; }
  int pendingRequests() const { return m_pendingRequests; }
  int stagedRequests() const { return m_stagedRequests; }
  QString loadProgressStatus() const { return m_loadProgressStatus; }

  bool diskCacheEnabled() const { return m_diskCacheEnabled; }
  QString settingsStatus() const { return m_settingsStatus; }
  QString settingsStatusColor() const { return m_settingsStatusColor; }

  int activeIOTasks() const { return m_activeIOTasks; }
  QString ioStatus() const { return m_ioStatus; }

  QStringList activeWarnings() const { return m_activeWarnings; }
  QStringList activeCriticals() const { return m_activeCriticals; }

signals:
  void healthChanged();
  void viewportCullingChanged();
  void viewportRangeChanged();
  void viewportStatusChanged();
  void totalItemsChanged();
  void loadedItemsChanged();
  void pendingRequestsChanged();
  void stagedRequestsChanged();
  void loadProgressChanged();
  void diskCacheChanged();
  void settingsStatusChanged();
  void ioTasksChanged();
  void ioStatusChanged();
  void warningsChanged();
  void criticalsChanged();

  // Critical alerts
  void criticalIssueDetected(const QString &issue);

private slots:
  void runDiagnostics();
  void onModelStateChanged();
  void onSettingsChanged();

private:
  void checkViewportCulling();
  void checkLoadProgress();
  void checkSettings();
  void checkAdaptiveIO();
  void updateOverallHealth();

  void addWarning(const QString &warning);
  void addCritical(const QString &critical);
  void clearWarnings();

  QTimer *m_diagnosticsTimer;
  ScrollBenchImageModel *m_model;
  SettingsHelper *m_settings;

  // State tracking
  QString m_healthStatus;
  QString m_healthColor; // "green", "yellow", "red"

  bool m_viewportCullingEnabled;
  int m_viewportRangeSize;
  int m_bufferedRangeSize;
  int m_viewportStartIndex;
  int m_viewportEndIndex;
  QString m_viewportStatus;
  QString m_viewportStatusColor;

  int m_totalItems;
  int m_loadedItems;
  int m_pendingRequests;
  int m_stagedRequests;
  QString m_loadProgressStatus;

  bool m_diskCacheEnabled;
  QString m_settingsStatus;
  QString m_settingsStatusColor;

  int m_activeIOTasks;
  QString m_ioStatus;

  QStringList m_activeWarnings;
  QStringList m_activeCriticals;

  // Anomaly thresholds
  static constexpr int EXPECTED_MIN_RANGE_SIZE = 50;
  static constexpr int CRITICAL_STALL_THRESHOLD_MS = 5000;
  QDateTime m_lastLoadTimestamp;
};

#endif // DIAGNOSTICSMONITOR_H
