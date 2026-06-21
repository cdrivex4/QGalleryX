#ifndef ALBUMMODEL_H
#define ALBUMMODEL_H

#include <QAbstractListModel>
#include <QString>
#include <QVector>
#include <atomic>

struct AlbumInfo {
  QString name;
  QString path;
  QStringList coverPaths;
  int count;
};

class AlbumModel : public QAbstractListModel {
  Q_OBJECT

public:
  enum AlbumRoles {
    NameRole = Qt::UserRole + 1,
    PathRole,
    CoverPathRole,
    CountRole
  };

  Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
  Q_PROPERTY(QString filterQuery READ filterQuery WRITE setFilterQuery NOTIFY filterQueryChanged)

  explicit AlbumModel(QObject *parent = nullptr);

  bool isLoading() const { return m_isLoading; }
  QString filterQuery() const { return m_filterQuery; }
  void setFilterQuery(const QString &query);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  Q_INVOKABLE void scanAlbums(const QString &path);

signals:
  void isLoadingChanged();
  void scanFinished();
  void filterQueryChanged();

public slots:
  void applyFilterFromPaths(const QStringList &activePaths);

private:
  bool m_isLoading = false;
  QString m_filterQuery;
  QVector<AlbumInfo> m_albums;
  QVector<AlbumInfo> m_allAlbums;
  std::atomic<int> m_scanGeneration{0};
};

#endif // ALBUMMODEL_H
