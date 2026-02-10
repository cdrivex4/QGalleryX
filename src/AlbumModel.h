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

  explicit AlbumModel(QObject *parent = nullptr);

  bool isLoading() const { return m_isLoading; }

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  Q_INVOKABLE void scanAlbums(const QString &path);

signals:
  void isLoadingChanged();
  void scanFinished();

private:
  bool m_isLoading = false;
  QVector<AlbumInfo> m_albums;
  std::atomic<int> m_scanGeneration{0};
};

#endif // ALBUMMODEL_H
