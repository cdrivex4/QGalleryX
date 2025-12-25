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

public:
  enum ProxyRoles {
    TypeRole = Qt::UserRole + 100, // 0 = Header, 1 = Row
    HeaderTitleRole,
    RowStartIndexRole, // Index in source model of the first image in this row
    RowCountRole,      // Number of images in this row
    SourceFilePathRole // The file path of the item from the source model
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

  Q_INVOKABLE QModelIndex getProxyIndexForSourceIndex(int sourceIndex) const;
  Q_INVOKABLE QVariantList getYearDistribution() const;
  Q_INVOKABLE QString getLabelForProxyIndex(int proxyIndex) const;
signals:
  void sourceModelChanged();
  void columnsChanged();
  void groupRoleChanged();

private slots:
  void onSourceModelReset();
  void onSourceDataChanged(const QModelIndex &topLeft,
                           const QModelIndex &bottomRight,
                           const QVector<int> &roles);

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
