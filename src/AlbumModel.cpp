#include "AlbumModel.h"
#include "DesktopHelper.h"
#include "FastVolumeScanner.h"
#include "TaskScheduler.h"
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QStorageInfo>
#include <QThread>
#include <QUrl>
#include <QtConcurrent>


AlbumModel::AlbumModel(QObject *parent) : QAbstractListModel(parent) {}

int AlbumModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return m_albums.count();
}

QVariant AlbumModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= m_albums.count())
    return QVariant();

  const AlbumInfo &info = m_albums[index.row()];
  switch (role) {
  case NameRole:
    return info.name;
  case PathRole:
    return info.path;
  case CoverPathRole:
    return info.coverPaths;
  case CountRole:
    return info.count;
  }
  return QVariant();
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
  // Clear existing
  beginResetModel();
  m_albums.clear();
  endResetModel();

  int myGen = ++m_scanGeneration;
  m_isLoading = true;
  emit isLoadingChanged();

  QStringList extensions = {
      "jpg", "jpeg", "png",  "mp4",  "mkv",  "avi", "mov",  "arw", "cr2",
      "dng", "nef",  "webp", "heic", "tiff", "bmp", "gif",  "ico", "tga",
      "sr2", "srf",  "orf",  "rw2",  "pef",  "raf", "webm", "flv", "vob",
      "ogg", "ogv",  "mts",  "m2ts", "ts",   "3gp"};

  QtConcurrent::run([this, path, myGen, extensions]() {
    QString cleanPath;
    QUrl url(path);
    if (url.isValid() && url.isLocalFile()) {
      cleanPath = url.toLocalFile();
    } else {
      cleanPath = path;
      if (cleanPath.startsWith("file:///"))
        cleanPath = cleanPath.mid(8);
      else if (cleanPath.startsWith("file://"))
        cleanPath = cleanPath.mid(7);
    }
    cleanPath = QDir::cleanPath(cleanPath);

    // Normalize path for comparisons
    cleanPath = QDir::fromNativeSeparators(cleanPath);

    bool isNetworkPath = DesktopHelper::staticIsNetworkPath(cleanPath);

    QMap<QString, AlbumInfo> albumMap;

    // --- Fast MFT Scan ---
    bool fastScanSuccess = false;
    if (!isNetworkPath && cleanPath.length() >= 2 && cleanPath[1] == ':') {
      FastVolumeScanner fastScanner;
      if (fastScanner.scanVolume(cleanPath.left(3))) {
        if (myGen != m_scanGeneration.load())
          return;

        QVector<QString> allFiles = fastScanner.getAllFiles();
        QString cleanPathPrefix = cleanPath;
        if (!cleanPathPrefix.endsWith("/"))
          cleanPathPrefix += "/";

        int batchCount = 0;
        for (const QString &f : allFiles) {
          if (myGen != m_scanGeneration.load())
            return;

          QString normF = QDir::fromNativeSeparators(f);
          if (normF.startsWith(cleanPathPrefix, Qt::CaseInsensitive)) {
            QString ext = QFileInfo(normF).suffix().toLower();
            if (extensions.contains(ext)) {
              QString fileUri = "file:///" + normF;
              QString currentDir =
                  QDir::cleanPath(QFileInfo(normF).absolutePath());
              currentDir = QDir::fromNativeSeparators(currentDir);

              while (currentDir.startsWith(cleanPath, Qt::CaseInsensitive) &&
                     currentDir.length() > cleanPath.length()) {
                QString key = currentDir.toLower();
                if (!albumMap.contains(key)) {
                  AlbumInfo info;
                  info.path = currentDir;
                  info.name = QFileInfo(currentDir).fileName();
                  if (info.name.isEmpty())
                    info.name = QDir(currentDir).dirName();
                  info.count = 0;
                  albumMap.insert(key, info);
                }
                albumMap[key].count++;
                if (albumMap[key].coverPaths.size() < 4)
                  albumMap[key].coverPaths.append(fileUri);

                int lastSlash = currentDir.lastIndexOf('/');
                if (lastSlash < 0 || lastSlash < cleanPath.length())
                  break;
                currentDir = currentDir.left(lastSlash);
                if (currentDir.endsWith(":"))
                  currentDir += "/";
              }
            }
          }

          if (!isNetworkPath && ++batchCount >= 500) {
            QVector<AlbumInfo> current = albumMap.values().toVector();
            QMetaObject::invokeMethod(this, [this, current, myGen]() {
              if (myGen != m_scanGeneration.load())
                return;
              beginResetModel();
              m_albums = current;
              std::sort(m_albums.begin(), m_albums.end(),
                        [](const AlbumInfo &a, const AlbumInfo &b) {
                          return a.name.compare(b.name, Qt::CaseInsensitive) <
                                 0;
                        });
              endResetModel();
            });
            batchCount = 0;
          }
        }
        fastScanSuccess = true;
      }
    }

    // --- Fallback Scan ---
    if (!fastScanSuccess) {
      QStringList nameFilters;
      for (const QString &ext : extensions) {
        nameFilters << "*." + ext;
        nameFilters << "*." + ext.toUpper();
      }

      QDirIterator fileIt(cleanPath, nameFilters,
                          QDir::Files | QDir::NoSymLinks,
                          QDirIterator::Subdirectories);
      int batchCount = 0;
      while (fileIt.hasNext()) {
        if (myGen != m_scanGeneration.load())
          return;
        fileIt.next();
        QFileInfo fileInfo = fileIt.fileInfo();
        QString filePath = QDir::fromNativeSeparators(
            QDir::cleanPath(fileInfo.absoluteFilePath()));
        QString fileUri = "file:///" + filePath;
        QString currentDir = QDir::fromNativeSeparators(
            QDir::cleanPath(fileInfo.dir().absolutePath()));

        while (currentDir.startsWith(cleanPath, Qt::CaseInsensitive) &&
               currentDir.length() > cleanPath.length()) {
          QString key = currentDir.toLower();
          if (!albumMap.contains(key)) {
            AlbumInfo info;
            info.path = currentDir;
            info.name = QFileInfo(currentDir).fileName();
            if (info.name.isEmpty())
              info.name = QDir(currentDir).dirName();
            info.count = 0;
            albumMap.insert(key, info);
          }
          albumMap[key].count++;
          if (albumMap[key].coverPaths.size() < 4)
            albumMap[key].coverPaths.append(fileUri);

          int lastSlash = currentDir.lastIndexOf('/');
          if (lastSlash < 0 || lastSlash < cleanPath.length())
            break;
          currentDir = currentDir.left(lastSlash);
          if (currentDir.endsWith(":"))
            currentDir += "/";
        }

        if (!isNetworkPath && ++batchCount >= 50) {
          QVector<AlbumInfo> current = albumMap.values().toVector();
          QMetaObject::invokeMethod(this, [this, current, myGen]() {
            if (myGen != m_scanGeneration.load())
              return;
            beginResetModel();
            m_albums = current;
            std::sort(m_albums.begin(), m_albums.end(),
                      [](const AlbumInfo &a, const AlbumInfo &b) {
                        return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
                      });
            endResetModel();
          });
          batchCount = 0;
          QThread::msleep(5);
        }
      }
    }

    // Final Update
    QMetaObject::invokeMethod(this, [this, albumMap, myGen]() {
      if (myGen != m_scanGeneration.load())
        return;
      QVector<AlbumInfo> filtered;
      for (const auto &info : albumMap) {
        if (info.count > 0)
          filtered.append(info);
      }
      beginResetModel();
      m_allAlbums = filtered;
      m_albums = m_allAlbums;
      std::sort(m_albums.begin(), m_albums.end(),
                [](const AlbumInfo &a, const AlbumInfo &b) {
                  return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
                });
      std::sort(m_allAlbums.begin(), m_allAlbums.end(),
                [](const AlbumInfo &a, const AlbumInfo &b) {
                  return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
                });
      endResetModel();
      m_isLoading = false;
      emit isLoadingChanged();
      emit scanFinished();
    });
  });
}

void AlbumModel::setFilterQuery(const QString &query) {
  if (m_filterQuery != query) {
    m_filterQuery = query;
    emit filterQueryChanged();
    // Normal filtering if not using active paths
    if (query.isEmpty()) {
      beginResetModel();
      m_albums = m_allAlbums;
      endResetModel();
    }
  }
}

void AlbumModel::applyFilterFromPaths(const QStringList &activePaths) {
  if (m_filterQuery.isEmpty()) {
    beginResetModel();
    m_albums = m_allAlbums;
    endResetModel();
    return;
  }

  QSet<QString> allowedPaths;
  for (const QString &p : activePaths) {
    allowedPaths.insert(p.toLower());
  }

  beginResetModel();
  m_albums.clear();
  for (const auto &album : m_allAlbums) {
    QString albumPath = QDir::fromNativeSeparators(album.path).toLower();
    bool match = album.name.toLower().contains(m_filterQuery.toLower());
    if (!match) {
      for (const QString &allowed : allowedPaths) {
        if (allowed.startsWith(albumPath)) {
          match = true;
          break;
        }
      }
    }
    if (match) {
      m_albums.append(album);
    }
  }
  endResetModel();
}
