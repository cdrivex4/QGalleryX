#ifndef IMAGEMODEL_H
#define IMAGEMODEL_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QList>
#include <QObject>
#include <QRectF>
#include <QString>
#include <QVector>
#include <QTimer>
#include <deque>
#include <memory>
#include <QHash>
#include <QMutex>
#include <QReadWriteLock>
#include <QFileSystemWatcher>
#include <QPointer>
#include <queue>

struct ImageInfo {
  quint64 id = 0; // Absolute path hash
  QString filePath;
  QString fileName;
  QDateTime date;
  QDateTime dateTaken; // Added for sorting
  QDateTime dateModified;
  qint64 size = 0; // Added size member
  bool isSelected = false;
  bool isVideo = false;
};

class ImageModel : public QAbstractListModel {
  Q_OBJECT

  Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
  Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
  Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectedCountChanged)
  Q_PROPERTY(QString filterQuery READ filterQuery WRITE setFilterQuery NOTIFY filterQueryChanged)
  Q_PROPERTY(int totalCount READ totalCount NOTIFY totalCountChanged)
  Q_PROPERTY(int scanProgress READ scanProgress NOTIFY scanProgressChanged)
  Q_PROPERTY(int precacheMode READ precacheMode WRITE setPrecacheMode NOTIFY precacheModeChanged)
  Q_PROPERTY(int loadingResolution READ loadingResolution WRITE setLoadingResolution NOTIFY loadingResolutionChanged)
  Q_PROPERTY(int visibleStartIndex READ visibleStartIndex WRITE setVisibleStartIndex NOTIFY visibleIndicesChanged)
  Q_PROPERTY(int visibleEndIndex READ visibleEndIndex WRITE setVisibleEndIndex NOTIFY visibleIndicesChanged)
  Q_PROPERTY(double crawlerProgress READ crawlerProgress NOTIFY crawlerProgressChanged)
  Q_PROPERTY(int crawlerIndex READ crawlerIndex NOTIFY crawlerProgressChanged)
  Q_PROPERTY(int crawlerTotal READ crawlerTotal NOTIFY crawlerProgressChanged)
  Q_PROPERTY(int activeJobs READ activeJobs NOTIFY crawlerProgressChanged)
  Q_PROPERTY(QString scanMethod READ scanMethod NOTIFY scanMethodChanged)
  Q_PROPERTY(int scanDurationMs READ scanDurationMs NOTIFY scanMethodChanged)

public:
  enum ImageRoles {
    FilePathRole = Qt::UserRole + 1,
    FileNameRole,
    DateSectionRole,
    SectionDayRole,
    SectionMonthRole,
    SectionYearRole,
    SectionWeekRole,
    ExifRole,
    IsRawRole,
    IsSelectedRole,
    IsVideoRole
  };
  Q_ENUM(ImageRoles)

  explicit ImageModel(QObject *parent = nullptr);
  ~ImageModel() override;

  Q_INVOKABLE void scanDirectory(const QString &path);
  Q_INVOKABLE bool cropImage(int index, const QRectF &cropRect);
  Q_INVOKABLE QVariantMap getMetadata(int index);
  
  Q_INVOKABLE void clearSelection();
  Q_INVOKABLE void selectAll();
  Q_INVOKABLE void invertSelection();
  Q_INVOKABLE void selectItems(const QList<int> &indices);
  Q_INVOKABLE void selectRange(int fromIndex, int toIndex, bool isSelected = true);
  Q_INVOKABLE void toggleSelection(int index);
  Q_INVOKABLE void deleteSelected();
  Q_INVOKABLE QStringList getSelectedPaths() const;
  Q_INVOKABLE qint64 getSelectedTotalSizeBytes() const;

  // Background control
  Q_INVOKABLE void pauseBackgroundTasks();
  Q_INVOKABLE void resumeBackgroundTasks();
  Q_INVOKABLE void reCrawl();
  Q_INVOKABLE QVariantMap validateCacheCoverage() const;

  Q_INVOKABLE QStringList getActiveDirectories() const;

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::EditRole) override;
  QHash<int, QByteArray> roleNames() const override;

  qint64 getGroupKey(int index, int role) const;

  bool isLoading() const { return m_isLoading; }
  int selectedCount() const;
  QString filterQuery() const { return m_filterQuery; }
  void setFilterQuery(const QString &query);
  int totalCount() const { return m_totalCount; }
  int scanProgress() const { return m_scanProgress; }
  int precacheMode() const { return m_precacheMode; }
  void setPrecacheMode(int mode);
  int loadingResolution() const { return m_loadingResolution; }
  void setLoadingResolution(int res);
  
  int visibleStartIndex() const { return m_visibleStartIndex; }
  void setVisibleStartIndex(int idx);
  
  int visibleEndIndex() const { return m_visibleEndIndex; }
  void setVisibleEndIndex(int idx);
  const QList<ImageInfo>& allItems() const { return m_allItems; }

  double crawlerProgress() const {
    int total = m_crawlWorkQueue.size();
    return total > 0 ? std::min(1.0, (double)m_crawledCount.load() / total) : 1.0;
  }
  int crawlerIndex() const { return m_crawledCount.load(); }
  int crawlerTotal() const { return m_crawlWorkQueue.size(); }
  int activeJobs() const { return m_crawlInflight.load(); }
  QString scanMethod() const { return m_scanMethod; }
  int scanDurationMs() const { return m_scanDurationMs; }

  Q_INVOKABLE int indexOfPath(const QString& path) const;

signals:
  void isLoadingChanged();
  void countChanged();
  void selectedCountChanged();
  void filterQueryChanged();
  void totalCountChanged();
  void scanProgressChanged();
  void itemsPopulated(quint32 scanId);
  void passOneCompleted(quint32 scanId);
  void precacheModeChanged();
  void storageWarning(QString message);
  void loadingResolutionChanged();
  void visibleIndicesChanged();
  void crawlerProgressChanged();
  void scanMethodChanged();
  void crawlerStatusChanged(QString status, bool isWarning);
  void crawlerThrottled(bool throttled);

private slots:
  void processPrecacheTick();
  void processMetadataTick();
  void onDirectoryChanged(const QString &path);

private:
  void applyFilter();

  QList<ImageInfo> m_allItems;
  QList<ImageInfo> m_images;
  QString m_filterQuery;
  QString m_scanMethod = "Idle";
  int m_scanDurationMs = 0;
  std::atomic<uint64_t> m_scanGeneration{0}; // Cancellation token
  std::atomic<uint64_t> m_precacheGeneration{0}; // Precache Cancellation token
  std::atomic<uint64_t> m_filterGeneration{0}; // Filter query cancellation token
  std::atomic<quint32> m_scanId{0}; // Drag-Drop Race protection
  int m_totalCount = 0;
  int m_scanProgress = 0;
  std::atomic<bool> m_isLoading{false};
  QString m_currentPath;
  int m_activeThreadCount = 0;

  int m_precacheMode = 1; // 0: Battery Saver, 1: Lookahead Window, 2: Aggressive Full Crawl
  // Work queue: built once from scan results. Contains ONLY files NOT in the mmap.
  // GUARDED BY m_crawlMutex — accessed from GUI timer tick, scan worker, and cacheCleared signal.
  QList<QString> m_crawlWorkQueue;
  QMutex m_crawlMutex; // Protects m_crawlWorkQueue
  std::atomic<int> m_crawlQueueIndex{0};  // Current position in the work queue
  std::atomic<int> m_crawledCount{0};     // Items finished (submitted + skipped)
  std::atomic<int> m_crawlInflight{0};    // Tasks currently running
  bool m_crawlDbFull = false;
  bool m_crawlPassComplete = false;
  bool m_crawlerThrottledState = false;   // Track last throttle state to avoid repeat toasts
  QElapsedTimer m_crawlerThrottleTimer;   // Per-instance timer (not static!) so windows don't interfere
  QHash<quint64, int> m_idToRow;
  QHash<quint64, QString> m_idToPath; // For AsyncImageProvider
  int m_loadingResolution = 200;
  QTimer* m_precacheTimer;
  QTimer* m_metadataTimer;
  
  int m_visibleStartIndex = 0;
  int m_visibleEndIndex = 0;
  
  void updatePrecacheQueue();

  // Pass 2 Background Metadata Queue
  struct MetadataUpdate {
      quint64 id;
      qint64 size;
      QDateTime date;
  };
  std::queue<MetadataUpdate> m_metadataQueue;
  QMutex m_metadataMutex;
  mutable QReadWriteLock m_modelLock;
  QFileSystemWatcher m_folderWatcher;
  std::shared_ptr<std::atomic<bool>> m_aliveToken = std::make_shared<std::atomic<bool>>(true);
};

#endif // IMAGEMODEL_H
 
