#ifndef ALBUMMODEL_H
#define ALBUMMODEL_H

#include <QAbstractListModel>
#include <QString>
#include <QVector>

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

  explicit AlbumModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  Q_INVOKABLE void scanAlbums(const QString &path);

signals:
  void scanFinished();

private:
  QVector<AlbumInfo> m_albums;
};

#endif // ALBUMMODEL_H
