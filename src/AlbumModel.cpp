#include "AlbumModel.h"
#include <QDebug>
#include <QDirIterator>
#include <QMap>
#include <QStandardPaths>
#include <QtConcurrent>

AlbumModel::AlbumModel(QObject *parent) : QAbstractListModel(parent) {}

int AlbumModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return m_albums.count();
}

QVariant AlbumModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= m_albums.count())
    return QVariant();

  const AlbumInfo &album = m_albums[index.row()];

  switch (role) {
  case NameRole:
    return album.name;
  case PathRole:
    return album.path;
  case CoverPathRole:
    return QVariant(album.coverPaths);
  case CountRole:
    return album.count;
  default:
    return QVariant();
  }
}

QHash<int, QByteArray> AlbumModel::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[NameRole] = "name";
  roles[PathRole] = "path";
  roles[CoverPathRole] = "coverPath";
  roles[CountRole] = "count";
  return roles;
}

void AlbumModel::scanAlbums(const QString &path) {
  if (path.isEmpty())
    return;

  QtConcurrent::run([this, path]() {
    QString cleanPath = path;
    if (cleanPath.startsWith("file:///")) {
      cleanPath = cleanPath.mid(8);
    }

    QMap<QString, AlbumInfo> albumMap;
    QStringList nameFilters;
    nameFilters << "*.jpg" << "*.jpeg" << "*.png" << "*.bmp" << "*.gif";

    QDirIterator it(cleanPath, nameFilters, QDir::Files,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
      it.next();
      QFileInfo fileInfo = it.fileInfo();
      QString dirName = fileInfo.dir().dirName();
      QString dirPath = fileInfo.dir().absolutePath();
      QString filePath = "file:///" + fileInfo.absoluteFilePath();

      if (!albumMap.contains(dirPath)) {
        AlbumInfo info;
        info.name = dirName;
        info.path = dirPath;
        info.coverPaths.append(filePath);
        info.count = 1;
        albumMap.insert(dirPath, info);
      } else {
        albumMap[dirPath].count++;
        if (albumMap[dirPath].coverPaths.count() < 4) {
          albumMap[dirPath].coverPaths.append(filePath);
        }
      }
    }

    // Update model on main thread
    QMetaObject::invokeMethod(this, [this, albumMap]() {
      beginResetModel();
      m_albums = albumMap.values().toVector();
      endResetModel();
      emit scanFinished();
    });
  });
}
