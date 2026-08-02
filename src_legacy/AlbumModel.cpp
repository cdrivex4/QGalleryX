#include "AlbumModel.h"
#include "ImageModel.h"
#include <QDebug>
#include <QDirIterator>
#include <QMap>
#include <QStandardPaths>
#include <QtConcurrent>

AlbumModel::AlbumModel(QObject *parent) : QAbstractListModel(parent) {}

void AlbumModel::setSourceModel(ImageModel *model) {
  if (m_sourceModel != model) {
    if (m_sourceModel) {
      disconnect(m_sourceModel, &ImageModel::itemsPopulated, this, &AlbumModel::rebuildFromSourceModel);
    }
    m_sourceModel = model;
    if (m_sourceModel) {
      connect(m_sourceModel, &ImageModel::itemsPopulated, this, &AlbumModel::rebuildFromSourceModel);
      rebuildFromSourceModel();
    }
    emit sourceModelChanged();
  }
}

void AlbumModel::rebuildFromSourceModel() {
  if (!m_sourceModel) return;

  const auto &items = m_sourceModel->allItems();
  QMap<QString, AlbumInfo> albumMap;

  for (const auto &item : items) {
    QFileInfo fi(item.filePath);
    QString dirPath = QDir::fromNativeSeparators(fi.dir().absolutePath());
    QString dirName = fi.dir().dirName();

    if (!albumMap.contains(dirPath)) {
      AlbumInfo info;
      info.name = dirName;
      info.path = dirPath;
      info.count = 0;
      albumMap[dirPath] = info;
    }

    albumMap[dirPath].count++;
    if (albumMap[dirPath].coverPaths.count() < 4) {
      albumMap[dirPath].coverPaths.append(item.filePath);
    }
  }

  QVector<AlbumInfo> newAlbums;
  for (auto it = albumMap.begin(); it != albumMap.end(); ++it) {
    newAlbums.append(it.value());
  }

  m_allAlbums = newAlbums;
  applyFilter();

  m_isLoading = false;
  emit isLoadingChanged();
  emit scanFinished();
}

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
  
  if (m_validPaths.isEmpty() && m_filterQuery.isEmpty()) {
     m_albums = m_allAlbums;
  } else {
     m_albums.clear();
     QSet<QString> validDirs;
     for (const QString &p : m_validPaths) {
         validDirs.insert(QDir::fromNativeSeparators(p).toLower());
     }
     
     for (const auto &album : m_allAlbums) {
         bool pathMatches = validDirs.contains(album.path.toLower());
         bool nameMatches = m_filterQuery.isEmpty() ? false : 
             (album.name.contains(m_filterQuery, Qt::CaseInsensitive) || 
              album.path.contains(m_filterQuery, Qt::CaseInsensitive));
         
         if (pathMatches || nameMatches) {
             m_albums.append(album);
         }
     }
  }
  endResetModel();
}

void AlbumModel::applyFilterFromPaths(const QStringList &validPaths) {
  m_validPaths = validPaths;
  applyFilter();
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

  if (m_sourceModel) {
    m_sourceModel->scanDirectory(path);
    rebuildFromSourceModel();
    return;
  }

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

    QDirIterator it(cleanPath, nameFilters, QDir::Files | QDir::NoSymLinks,
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
