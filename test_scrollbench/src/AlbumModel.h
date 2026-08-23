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

#include "ImageModel.h"

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
  Q_PROPERTY(ImageModel* sourceModel READ sourceModel WRITE setSourceModel NOTIFY sourceModelChanged)

  explicit AlbumModel(QObject *parent = nullptr);

  bool isLoading() const { return m_isLoading; }
  
  QString filterQuery() const { return m_filterQuery; }
  void setFilterQuery(const QString &query);

  ImageModel* sourceModel() const { return m_sourceModel; }
  void setSourceModel(ImageModel *model);

  Q_INVOKABLE void rebuildFromSourceModel();

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  Q_INVOKABLE void scanAlbums(const QString &path);
  Q_INVOKABLE void applyFilterFromPaths(const QStringList &validPaths);

signals:
  void isLoadingChanged();
  void scanFinished();
  void filterQueryChanged();
  void sourceModelChanged();

private:
  void applyFilter();

  ImageModel *m_sourceModel = nullptr;
  bool m_isLoading = false;
  QString m_filterQuery;
  QStringList m_validPaths;
  QVector<AlbumInfo> m_allAlbums; // Full source of truth
  QVector<AlbumInfo> m_albums;    // Filtered visible items
};

#endif // ALBUMMODEL_H
