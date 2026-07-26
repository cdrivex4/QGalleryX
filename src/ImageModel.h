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
  bool isBurst = false;
  bool isSelected = false;
};

class ImageModel : public QAbstractListModel {
  Q_OBJECT

  Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
  Q_PROPERTY(int visibleStartIndex READ visibleStartIndex WRITE setVisibleStartIndex NOTIFY visibleRangeChanged)
  Q_PROPERTY(int visibleEndIndex READ visibleEndIndex WRITE setVisibleEndIndex NOTIFY visibleRangeChanged)

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
    DateTimeRole,
    IsBurstRole,
    IsSelectedRole
  };
  Q_ENUM(ImageRoles)

  Q_PROPERTY(QString filterQuery READ filterQuery WRITE setFilterQuery NOTIFY filterQueryChanged)
  Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectedCountChanged)

  explicit ImageModel(QObject *parent = nullptr);

  Q_INVOKABLE void scanDirectory(const QString &path);
  Q_INVOKABLE bool cropImage(int index, const QRectF &cropRect);
  Q_INVOKABLE QVariantMap getMetadata(int index);
  Q_INVOKABLE void clearSelection();
  Q_INVOKABLE QStringList getSelectedPaths() const;
  Q_INVOKABLE int getProxyIndexForSourceIndex(int sourceIndex) const;
  Q_INVOKABLE QStringList getActiveDirectories() const;

  int selectedCount() const;

  QString filterQuery() const { return m_filterQuery; }
  void setFilterQuery(const QString &query);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
  QHash<int, QByteArray> roleNames() const override;

  bool isLoading() const { return m_isLoading; }
  
  int visibleStartIndex() const { return m_visibleStartIndex; }
  void setVisibleStartIndex(int index);
  
  int visibleEndIndex() const { return m_visibleEndIndex; }
  void setVisibleEndIndex(int index);

  static bool isPathVisible(const QString &path);

signals:
  void isLoadingChanged();
  void visibleRangeChanged();
  void filterQueryChanged();
  void selectedCountChanged();

private:
  void updateVisiblePaths();
  void processPendingUpdates();
  void applyFilter();

  QList<ImageInfo> m_images;
  QList<ImageInfo> m_allImages;
  QString m_filterQuery;
  
  QList<ImageInfo> m_pendingInsertions;
  class QTimer* m_updateTimer = nullptr;
  bool m_isLoading = false;
  int m_visibleStartIndex = -1;
  int m_visibleEndIndex = -1;
};

#endif // IMAGEMODEL_H
