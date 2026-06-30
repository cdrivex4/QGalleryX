#ifndef SCROLLBENCHIMAGEMODEL_H
#define SCROLLBENCHIMAGEMODEL_H

#include <QAbstractListModel>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QHash>
#include <QSet>

class FrameBudgetScheduler;
class QTimer;
class ImageProcessor; // Forward declaration

class ScrollBenchImageModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int visibleStartIndex READ visibleStartIndex WRITE
                 setVisibleStartIndex NOTIFY visibleRangeChanged)
  Q_PROPERTY(int visibleEndIndex READ visibleEndIndex WRITE setVisibleEndIndex
                 NOTIFY visibleRangeChanged)
  Q_PROPERTY(int pendingDecodeCount READ pendingDecodeCount NOTIFY
                 pendingDecodeCountChanged)
  Q_PROPERTY(
      int remainingItems READ remainingItems NOTIFY remainingItemsChanged)
  Q_PROPERTY(int totalItems READ totalItems CONSTANT)
  Q_PROPERTY(int stagedRequestCount READ stagedRequestCount NOTIFY
                 stagedRequestCountChanged)
  Q_PROPERTY(bool viewportCullingEnabled READ viewportCullingEnabled WRITE
                 setViewportCullingEnabled NOTIFY viewportCullingEnabledChanged)
  Q_PROPERTY(int loadedCount READ loadedCount NOTIFY loadedCountChanged)
  Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
  Q_PROPERTY(int scannedCount READ scannedCount NOTIFY scannedCountChanged)
  Q_PROPERTY(QString filterQuery READ filterQuery WRITE setFilterQuery NOTIFY filterQueryChanged)
  Q_PROPERTY(int roleSectionDay READ roleSectionDay CONSTANT)
  Q_PROPERTY(int roleSectionWeek READ roleSectionWeek CONSTANT)
  Q_PROPERTY(int roleSectionMonth READ roleSectionMonth CONSTANT)
  Q_PROPERTY(int roleSectionYear READ roleSectionYear CONSTANT)
  Q_PROPERTY(int roleSectionType READ roleSectionType CONSTANT)

public:
  explicit ScrollBenchImageModel(QObject *parent = nullptr);

  enum Roles {
    FilePathRole = Qt::UserRole + 1,
    FileNameRole,
    DateSectionRole,
    SectionDayRole,
    SectionMonthRole,
    SectionYearRole,
    SectionWeekRole,
    SectionTypeRole,
    ExifRole,
    IsRawRole,
    IsVideoRole, // Added for video detection
    // ScrollBench specific additions (moved to avoid collision with ProxyRoles
    ImageIndexRole = Qt::UserRole + 150,
    IsLoadedRole,
    ColorRole,
    IsSelectedRole,
    IsBurstRole,
    VersionRole // For cache busting
  };

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  // Viewport culling properties
  int visibleStartIndex() const { return m_visibleStartIndex; }
  void setVisibleStartIndex(int index);

  int visibleEndIndex() const { return m_visibleEndIndex; }
  void setVisibleEndIndex(int index);

  int pendingDecodeCount() const { return m_pendingDecodes; }
  int loadedCount() const { return m_loadedCount; }
  int remainingItems() const { return m_remainingItems; }
  int totalItems() const { return m_totalItems; }
  int scannedCount() const { return m_scannedCount; }

  bool viewportCullingEnabled() const { return m_viewportCullingEnabled; }
  void setViewportCullingEnabled(bool enabled);

  QString filterQuery() const { return m_filterQuery; }
  void setFilterQuery(const QString &query);

  int roleSectionDay() const { return SectionDayRole; }
  int roleSectionWeek() const { return SectionWeekRole; }
  int roleSectionMonth() const { return SectionMonthRole; }
  int roleSectionYear() const { return SectionYearRole; }
  int roleSectionType() const { return SectionTypeRole; }

  Q_INVOKABLE QStringList getActiveDirectories() const;
  Q_INVOKABLE void generateTestData(int count = 10000);


  Q_INVOKABLE void setSortMode(int mode); // 1-4=Date, 5=Type
  Q_INVOKABLE void clearData();
  Q_INVOKABLE void scanDirectory(const QString &path);
  Q_INVOKABLE int stagedRequestCount() const;
  Q_INVOKABLE void cancelScan();

  // Selection methods
  Q_INVOKABLE void toggleSelection(int index);
  Q_INVOKABLE void selectRange(int start, int end);
  Q_INVOKABLE void selectVisualRect(int colMin, int colMax, int rowMin,
                                    int rowMax, int columns);
  Q_INVOKABLE void selectAll();
  Q_INVOKABLE void clearSelection();
  Q_INVOKABLE void invertSelection();
  Q_INVOKABLE void deleteSelected();
  Q_INVOKABLE QStringList getSelectedPaths() const;
  Q_INVOKABLE qint64 getSelectedTotalSizeBytes() const;
  Q_INVOKABLE void rotateSelected(int degrees);
  int selectedCount() const;

  bool isLoading() const { return m_isLoading; }

  void setFrameScheduler(FrameBudgetScheduler *scheduler);

signals:
  void visibleRangeChanged();
  void pendingDecodeCountChanged();
  void remainingItemsChanged();
  void viewportCullingEnabledChanged();
  void scanProgress(int current, int total);
  void scanComplete(int totalFound);
  void selectedCountChanged();
  void isLoadingChanged();
  void scannedCountChanged();
  void loadedCountChanged();
  void stagedRequestCountChanged();
  void filterQueryChanged();
  void forceUpdateGridView(); // New signal to trigger QML GridView update

public slots: // New public slot
  void forceDelayedUpdate();

private slots:
  void processPendingUpdates();

private:
  struct ImageItem {
    QString path;
    QString fileName;
    QString color; // For synthetic test images
    QDateTime date;
    QString sectionDay;
    bool isLoaded = false;
    bool isSelected = false;
    bool isBurst = false;
    bool isRaw = false;
    bool isVideo = false;
    int version = 0;
  };

  void updateVisibleRange();
  void requestThumbnail(int index);
  void cancelPendingRequests();
  void applyFilter();
  int m_sortMode = 1;
  void resortItems();

  QVector<ImageItem> m_allItems; // Backup of all scanned items
  QVector<ImageItem> m_items;    // Currently visible (filtered) items
  QString m_filterQuery;
  QString m_scanRoot;
  
  QSet<int> m_activelyRequesting;
  int m_visibleStartIndex = 0;
  int m_visibleEndIndex = 0;
  QTimer *m_loadAllTimer = nullptr;
  int m_loadAllIndex = 0;
  int m_loadedCount = 0;
  int m_pendingDecodes = 0;
  int m_remainingItems = 0;
  int m_totalItems = 0;
  bool m_viewportCullingEnabled = true;
  bool m_isLoading = false;
  int m_scannedCount = 0;
  bool m_scanCancelled = false;
  std::atomic<int> m_scanGeneration{0};

  // Batching updates
  QSet<int> m_pendingLoadedIndices;
  QTimer *m_updateTimer = nullptr;
  QTimer *m_forceUpdateTimer = nullptr; // New timer for delayed QML update
  FrameBudgetScheduler *m_frameScheduler = nullptr;

public:
  Q_INVOKABLE bool cropImage(int index, const QRectF &cropRect);
  Q_INVOKABLE bool rotateImage(int index, int degrees);
  Q_INVOKABLE QVariantMap getMetadata(int index);

private:
  ImageProcessor *m_imageProcessor = nullptr;
};

#endif // SCROLLBENCHIMAGEMODEL_H
