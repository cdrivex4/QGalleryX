#include "AlbumModel.h"
#include "FastVolumeScanner.h"
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

  m_isLoading = true;
  emit isLoadingChanged();

  QFuture<void> future = QtConcurrent::run([this, path]() {
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
    cleanPath = QDir::toNativeSeparators(cleanPath);
    qDebug() << "[AlbumModel] Starting scan for path:"
             << cleanPath; // Debug log

    QMap<QString, AlbumInfo> albumMap;

    // --- Fast MFT Scan Attempt ---
    bool fastScanSuccess = false;
    if (cleanPath.length() >= 3 && cleanPath[1] == ':' && cleanPath[2] == '/') {
      FastVolumeScanner fastScanner;
      if (fastScanner.scanVolume(cleanPath)) {
        qDebug() << "[AlbumModel] FastScanner: Success! Populating albums...";
        QVector<QString> allFiles = fastScanner.getAllFiles();
        QString searchPrefix = cleanPath;
        if (!searchPrefix.endsWith("/"))
          searchPrefix += "/";

        QStringList extensions = {"jpg", "jpeg", "png", "bmp",  "gif", "mp4",
                                  "mkv", "avi",  "mov", "webm", "cr2", "dng"};

        for (const QString &f : allFiles) {
          if (f.startsWith(searchPrefix, Qt::CaseInsensitive)) {
            QString ext = QFileInfo(f).suffix().toLower();
            if (extensions.contains(ext)) {
              // Extract Directory
              // Ideally FastVolumeScanner should give us Parent FRN to
              // reconstruct hierarchy without string manipulation But for now,
              // string manipulation is what we have. QFileInfo(f).path() might
              // be slow? Let's do simple string search for last slash
              int lastSlash = f.lastIndexOf('/');
              if (lastSlash > 0) {
                QString dirPath = f.left(lastSlash);

                // Add Album if not exists
                if (!albumMap.contains(dirPath)) {
                  AlbumInfo info;
                  info.name = QFileInfo(dirPath).fileName();
                  if (info.name.isEmpty())
                    info.name = QDir(dirPath).dirName();
                  info.path = dirPath;
                  info.count = 0;
                  albumMap.insert(dirPath, info);
                }

                // Add File info to Album
                albumMap[dirPath].count++;
                if (albumMap[dirPath].coverPaths.count() < 4) {
                  albumMap[dirPath].coverPaths.append("file:///" + f);
                }
              }
            }
          }
        }
        fastScanSuccess = true;
      }
    }

    if (fastScanSuccess) {
      qDebug() << "[AlbumModel] Fast scan complete. Albums found:"
               << albumMap.size();
    } else {
      // Fallback to standard recursive scan
      qDebug() << "[AlbumModel] Fast scan failed or N/A. Falling back to "
                  "standard scan.";

      // First pass: Enumerate all subdirectories as albums
      QDirIterator dirIt(cleanPath,
                         QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks,
                         QDirIterator::Subdirectories);
      while (dirIt.hasNext()) {
        dirIt.next();
        QFileInfo dirInfo = dirIt.fileInfo();
        QString dirName = dirInfo.fileName();
        QString dirPath = dirInfo.absoluteFilePath();

        AlbumInfo info;
        info.name = dirName;
        info.path = dirPath;
        info.count = 0; // Will be populated in second pass
        albumMap.insert(dirPath, info);
        qDebug() << "[AlbumModel] Added directory as album:" << dirPath;
      }

      // Always add the root path itself as an album if not already present
      if (!albumMap.contains(cleanPath)) {
        AlbumInfo info;
        info.name = QFileInfo(cleanPath).fileName();
        if (info.name.isEmpty())
          info.name = QDir(cleanPath).dirName(); // Fallback for root
        info.path = cleanPath;
        info.count = 0;
        albumMap.insert(cleanPath, info);
        qDebug() << "[AlbumModel] Added root path as album:" << cleanPath;
      }

      // Second pass: Populate album contents with files
      QStringList nameFilters;
      nameFilters << "*.jpg" << "*.jpeg" << "*.png" << "*.bmp" << "*.gif"
                  << "*.mp4" << "*.mkv" << "*.avi" << "*.mov" << "*.webm"
                  << "*.cr2" << "*.dng"; // Added video and raw extensions

      QDirIterator fileIt(cleanPath, nameFilters,
                          QDir::Files | QDir::NoSymLinks,
                          QDirIterator::Subdirectories);

      int batchSize = 0; // Batch updates to avoid blocking UI

      while (fileIt.hasNext()) {
        fileIt.next();
        QFileInfo fileInfo = fileIt.fileInfo();
        QString dirPath = fileInfo.dir().absolutePath();
        QString filePath = "file:///" + fileInfo.absoluteFilePath();

        // Ensure the album exists (it should from the first pass, or be the
        // root)
        if (albumMap.contains(dirPath)) {
          albumMap[dirPath].count++;
          if (albumMap[dirPath].coverPaths.count() < 4) { // Limit to 4 covers
            albumMap[dirPath].coverPaths.append(filePath);
          }
        } else {
          // This should ideally not happen if first pass is correct, but as a
          // fallback
          qWarning() << "[AlbumModel] File found in un-enumerated directory:"
                     << fileInfo.absoluteFilePath();
        }

        batchSize++;

        // Update UI every 50 items to make it progressive
        if (batchSize >= 50) {
          QVector<AlbumInfo> currentAlbums = albumMap.values().toVector();
          // Sort alphabetically for consistency in progressive updates
          std::sort(currentAlbums.begin(), currentAlbums.end(),
                    [](const AlbumInfo &a, const AlbumInfo &b) {
                      return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
                    });
          QMetaObject::invokeMethod(this, [this, currentAlbums]() {
            beginResetModel();
            m_albums = currentAlbums;
            endResetModel();
          });
          qDebug() << "[AlbumModel] Batch update. Current albumMap size:"
                   << albumMap.count(); // Debug log
          batchSize = 0;
          QThread::msleep(10); // Yield slightly to let UI update
        }
      }
    }

    // Final Update
    QMetaObject::invokeMethod(this, [this, albumMap]() {
      beginResetModel();
      m_albums = albumMap.values().toVector();
      // Sort final list
      std::sort(m_albums.begin(), m_albums.end(),
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
