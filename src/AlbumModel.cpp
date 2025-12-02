#include "AlbumModel.h"
#include <QDebug>
#include <QDirIterator>
#include <QMap>
#include <QStandardPaths>

AlbumModel::AlbumModel(QObject *parent) : QAbstractListModel(parent) {
  scanAlbums();
}

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
  case CoverPathRole:
    return album.coverPath;
  case CountRole:
    return album.count;
  default:
    return QVariant();
  }
}

QHash<int, QByteArray> AlbumModel::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[NameRole] = "name";
  roles[CoverPathRole] = "coverPath";
  roles[CountRole] = "count";
  return roles;
}

void AlbumModel::scanAlbums() {
  beginResetModel();
  m_albums.clear();

  QString picturesPath =
      QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
  if (!QDir(picturesPath).exists()) {
    picturesPath =
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
  }
  QStringList nameFilters;
  nameFilters << "*.jpg" << "*.jpeg" << "*.png" << "*.bmp" << "*.gif";

  QMap<QString, AlbumInfo> albumMap;

  QDirIterator it(picturesPath, nameFilters, QDir::Files,
                  QDirIterator::Subdirectories);
  while (it.hasNext()) {
    it.next();
    QFileInfo fileInfo = it.fileInfo();
    QString dirName = fileInfo.dir().dirName();
    QString filePath = "file:///" + fileInfo.absoluteFilePath();

    if (!albumMap.contains(dirName)) {
      AlbumInfo info;
      info.name = dirName;
      info.coverPath = filePath; // Use first found image as cover
      info.count = 1;
      albumMap.insert(dirName, info);
    } else {
      albumMap[dirName].count++;
    }
  }

  // Fallback for testing if empty
  if (albumMap.isEmpty()) {
    QDirIterator it2(QDir::currentPath(), nameFilters, QDir::Files,
                     QDirIterator::Subdirectories);
    while (it2.hasNext()) {
      it2.next();
      QFileInfo fileInfo = it2.fileInfo();
      QString dirName = fileInfo.dir().dirName();
      QString filePath = "file:///" + fileInfo.absoluteFilePath();

      if (!albumMap.contains(dirName)) {
        AlbumInfo info;
        info.name = dirName;
        info.coverPath = filePath;
        info.count = 1;
        albumMap.insert(dirName, info);
      } else {
        albumMap[dirName].count++;
      }
    }
  }

  m_albums = albumMap.values().toVector();
  endResetModel();
}
