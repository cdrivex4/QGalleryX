#ifndef GROUPEDPROXYMODEL_H
#define GROUPEDPROXYMODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QVariant>

class GroupedProxyModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(QAbstractListModel *sourceModel READ sourceModel WRITE
                 setSourceModel NOTIFY sourceModelChanged)
  Q_PROPERTY(int columns READ columns WRITE setColumns NOTIFY columnsChanged)
  Q_PROPERTY(
      int groupRole READ groupRole WRITE setGroupRole NOTIFY groupRoleChanged)
  Q_PROPERTY(int precacheMode READ precacheMode WRITE setPrecacheMode NOTIFY precacheModeChanged)
  Q_PROPERTY(int totalCount READ totalCount NOTIFY totalCountChanged)

public:
  enum ProxyRoles {
    TypeRole = Qt::UserRole + 100, // 0 = Header, 1 = Row
    HeaderTitleRole,
    RowStartIndexRole, // Index in source model of the first image in this row
    RowCountRole       // Number of images in this row
  };

  enum ItemType { HeaderItem = 0, RowItem = 1 };

  explicit GroupedProxyModel(QObject *parent = nullptr);

  QAbstractListModel *sourceModel() const;
  void setSourceModel(QAbstractListModel *model);

  int columns() const;
  void setColumns(int columns);

  int groupRole() const;
  void setGroupRole(int role);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  Q_INVOKABLE int selectedCount() const;
  Q_INVOKABLE void deleteSelected();
  Q_INVOKABLE QStringList getSelectedPaths() const;
  Q_INVOKABLE qint64 getSelectedTotalSizeBytes() const;
  
  // Background control
  Q_INVOKABLE void pauseBackgroundTasks();
  Q_INVOKABLE void resumeBackgroundTasks();

  Q_INVOKABLE QModelIndex getProxyIndexForSourceIndex(int sourceIndex) const;
  Q_INVOKABLE int getProxyRowForSourceIndex(int sourceIndex) const;
  Q_INVOKABLE int getSourceIndexAbove(int sourceIndex) const;
  Q_INVOKABLE int getSourceIndexBelow(int sourceIndex) const;
  Q_INVOKABLE QVariantList getYearDistribution() const;
  Q_INVOKABLE QString getLabelForProxyIndex(int proxyIndex) const;
  Q_INVOKABLE int indexOfPath(const QString &path) const;
  
  int precacheMode() const;
  void setPrecacheMode(int mode);
  int totalCount() const;

signals:
  void sourceModelChanged();
  void columnsChanged();
  void groupRoleChanged();
  void precacheModeChanged();
  void totalCountChanged();

private slots:
  void onSourceModelReset();

private:
  void rebuildIndex();

  struct IndexItem {
    ItemType type;
    QString headerTitle;
    int sourceStartIndex;
    int count;
  };

  QPointer<QAbstractListModel> m_sourceModel;
  int m_columns = 4;
  int m_groupRole = Qt::UserRole + 1; // Default role
  QList<IndexItem> m_index;
};

#endif // GROUPEDPROXYMODEL_H
