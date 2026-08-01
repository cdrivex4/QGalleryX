#ifndef IMAGEMODEL_H
#define IMAGEMODEL_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QList>
#include <QObject>
#include <QRectF>
#include <QString>
#include <QVector>

struct ImageInfo {
  QString filePath;
  QString fileName;
  QDateTime date;
  QDateTime dateTaken; // Added for sorting
  QDateTime dateModified;
  qint64 size = 0; // Added size member
  bool isSelected = false;
};

class ImageModel : public QAbstractListModel {
  Q_OBJECT

  Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
  Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectedCountChanged)
  Q_PROPERTY(QString filterQuery READ filterQuery WRITE setFilterQuery NOTIFY filterQueryChanged)
  Q_PROPERTY(int totalCount READ totalCount NOTIFY totalCountChanged)
  Q_PROPERTY(int scanProgress READ scanProgress NOTIFY scanProgressChanged)

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
    IsSelectedRole
  };
  Q_ENUM(ImageRoles)

  explicit ImageModel(QObject *parent = nullptr);

  Q_INVOKABLE void scanDirectory(const QString &path);
  Q_INVOKABLE bool cropImage(int index, const QRectF &cropRect);
  Q_INVOKABLE QVariantMap getMetadata(int index);
  
  Q_INVOKABLE void clearSelection();
  Q_INVOKABLE void selectAll();
  Q_INVOKABLE void invertSelection();
  Q_INVOKABLE void selectItems(const QList<int> &indices);
  Q_INVOKABLE void deleteSelected();
  Q_INVOKABLE QStringList getSelectedPaths() const;
  Q_INVOKABLE qint64 getSelectedTotalSizeBytes() const;

  // Background control
  Q_INVOKABLE void pauseBackgroundTasks();
  Q_INVOKABLE void resumeBackgroundTasks();

  Q_INVOKABLE QStringList getActiveDirectories() const;

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::EditRole) override;
  QHash<int, QByteArray> roleNames() const override;

  bool isLoading() const { return m_isLoading; }
  int selectedCount() const;
  QString filterQuery() const { return m_filterQuery; }
  void setFilterQuery(const QString &query);
  int totalCount() const { return m_totalCount; }
  int scanProgress() const { return m_scanProgress; }
  const QList<ImageInfo>& allItems() const { return m_allItems; }

signals:
  void isLoadingChanged();
  void selectedCountChanged();
  void filterQueryChanged();
  void totalCountChanged();
  void scanProgressChanged();
  void itemsPopulated();

private:
  void applyFilter();

  QList<ImageInfo> m_allItems;
  QList<ImageInfo> m_images;
  QString m_filterQuery;
  std::atomic<uint64_t> m_scanGeneration{0}; // Cancellation token
  int m_totalCount = 0;
  int m_scanProgress = 0;
  std::atomic<bool> m_isLoading{false};
  QString m_currentPath;
  int m_activeThreadCount = 0;
};

#endif // IMAGEMODEL_H
