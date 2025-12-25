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
  Q_PROPERTY(bool viewportCullingEnabled READ viewportCullingEnabled WRITE
                 setViewportCullingEnabled NOTIFY viewportCullingEnabledChanged)
  Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectedCountChanged)
  Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)

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
    ExifRole,
    IsRawRole,
    // ScrollBench specific additions (moved to avoid collision with ProxyRoles
    // 356-360)
    ImageIndexRole = Qt::UserRole + 150,
    IsLoadedRole,
    ColorRole,
    IsSelectedRole,
    IsBurstRole
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
  int remainingItems() const { return m_remainingItems; }
  int totalItems() const { return m_totalItems; }

  bool viewportCullingEnabled() const { return m_viewportCullingEnabled; }
  void setViewportCullingEnabled(bool enabled);

  Q_INVOKABLE void generateTestData(int count = 10000);
  Q_INVOKABLE void clearData();
  Q_INVOKABLE void scanDirectory(const QString &path);
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
    bool isLoaded = false;
    bool isSelected = false;
    bool isBurst = false;
  };

  void updateVisibleRange();
  void requestThumbnail(int index);
  void cancelPendingRequests();

  QVector<ImageItem> m_items;
  QSet<int> m_activelyRequesting;
  int m_visibleStartIndex = 0;
  int m_visibleEndIndex = 0;
  int m_pendingDecodes = 0;
  int m_remainingItems = 0;
  int m_totalItems = 0;
  bool m_viewportCullingEnabled = true;
  bool m_isLoading = false;
  bool m_scanCancelled = false;
  static constexpr int BUFFER_SIZE = 10; // Load 10 items ahead/behind

  // Batching updates
  QSet<int> m_pendingLoadedIndices;
  QTimer *m_updateTimer = nullptr;
  QTimer *m_forceUpdateTimer = nullptr; // New timer for delayed QML update
  FrameBudgetScheduler *m_frameScheduler = nullptr;

public:
  Q_INVOKABLE bool cropImage(int index, const QRectF &cropRect);
  Q_INVOKABLE QVariantMap getMetadata(int index);
};

#endif // SCROLLBENCHIMAGEMODEL_H
