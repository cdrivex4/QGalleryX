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

signals:
  void isLoadingChanged();
  void selectedCountChanged();
  void filterQueryChanged();

private:
  void applyFilter();

  QList<ImageInfo> m_allItems;
  QList<ImageInfo> m_images;
  QString m_filterQuery;
  bool m_isLoading = false;
  QString m_currentPath;
  int m_activeThreadCount = 0;
};

#endif // IMAGEMODEL_H
