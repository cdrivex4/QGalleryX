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

void AlbumModel::setFilterQuery(const QString &query) {
  if (m_filterQuery != query) {
    m_filterQuery = query;
    emit filterQueryChanged();
    applyFilter();
  }
}

void AlbumModel::applyFilter() {
  beginResetModel();
  if (m_filterQuery.isEmpty()) {
    m_albums = m_allAlbums;
  } else {
    m_albums.clear();
    for (const auto &album : m_allAlbums) {
      if (album.name.contains(m_filterQuery, Qt::CaseInsensitive) || 
          album.path.contains(m_filterQuery, Qt::CaseInsensitive)) {
        m_albums.append(album);
      }
    }
  }
  endResetModel();
}

void AlbumModel::applyFilterFromPaths(const QStringList &validPaths) {
  beginResetModel();
  
  if (validPaths.isEmpty() && m_filterQuery.isEmpty()) {
     // If no paths match but query is also empty, show all (query cleared)
     m_albums = m_allAlbums;
  } else {
     m_albums.clear();
     QSet<QString> validDirs;
     for (const QString &p : validPaths) {
         validDirs.insert(QDir::fromNativeSeparators(p).toLower());
     }
     
     for (const auto &album : m_allAlbums) {
        QString normalizedAlbumPath = QDir::fromNativeSeparators(album.path).toLower();
        if (validDirs.contains(normalizedAlbumPath)) {
            m_albums.append(album);
        }
     }
  }
  endResetModel();
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

  m_isLoading = true;
  emit isLoadingChanged();

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

    // Batch updates to avoid blocking UI
    int batchSize = 0;

    while (it.hasNext()) {
      it.next();
      QFileInfo fileInfo = it.fileInfo();
      QString dirName = fileInfo.dir().dirName();
      QString dirPath = fileInfo.dir().absolutePath();
      QString filePath = "file:///" + fileInfo.absoluteFilePath();

      bool newAlbum = !albumMap.contains(dirPath);
      if (newAlbum) {
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

      batchSize++;

      // Update UI every 50 items or new album found to make it progressive
      if (batchSize >= 50 || (newAlbum && albumMap.count() <= 5)) {
        QVector<AlbumInfo> currentAlbums = albumMap.values().toVector();
        QMetaObject::invokeMethod(this, [this, currentAlbums]() {
          m_allAlbums = currentAlbums;
          applyFilter();
          // If we have at least one album, we can consider "established" enough
          // to hide overlay
          if (m_albums.count() > 0 && m_isLoading) {
            // Keep m_isLoading true, but the QML check (count === 0) will hide
            // overlay
          }
        });
        batchSize = 0;
        QThread::msleep(10); // Yield slightly to let UI update
      }
    }

    // Final Update
    QMetaObject::invokeMethod(this, [this, albumMap]() {
        QVector<AlbumInfo> finalAlbums = albumMap.values().toVector();
        m_allAlbums = finalAlbums;
        applyFilter();
      m_isLoading = false;
      emit isLoadingChanged();
      emit scanFinished();
    });
  });
}
