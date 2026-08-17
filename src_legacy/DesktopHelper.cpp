#include "DesktopHelper.h"
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QSettings>
#include <QStorageInfo>
#include <QTransform>
#include <QUrl>


#ifdef Q_OS_WIN
#include <windows.h>
#endif

DesktopHelper::DesktopHelper(QObject *parent) : QObject(parent) {}

void DesktopHelper::openInExplorer(const QString &path) {
  if (path.isEmpty())
    return;
  // ... (keep existing implementation or replace, I'll overwrite to be safe)

  QString nativePath = QDir::toNativeSeparators(path);
  if (nativePath.startsWith("file:")) {
    nativePath = QUrl(path).toLocalFile();
  }

  // Clean up any remaining file:/// prefix or QML artifacts
  if (nativePath.startsWith("file:///"))
    nativePath = nativePath.mid(8);
  else if (nativePath.startsWith("file:/"))
    nativePath = nativePath.mid(6);

  QString param;
  if (!QFileInfo(nativePath).isDir())
    param = QLatin1String("/select,");

  QProcess::startDetached("explorer.exe", QStringList() << param << nativePath);
}

int DesktopHelper::getFileType(const QString &path) {
  return staticGetFileType(path);
}

DesktopHelper::FileType DesktopHelper::staticGetFileType(const QString &path) {
  int dotIdx = path.lastIndexOf('.');
  if (dotIdx == -1 || dotIdx == path.length() - 1)
    return Unknown;

  QStringView ext = QStringView(path).mid(dotIdx + 1);

  // Video & Audio Media
  if (ext.compare(u"mp4", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"mkv", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"avi", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"mov", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"webm", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"flv", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"vob", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"ogg", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"ogv", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"mp3", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"wav", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"flac", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"m4a", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"aac", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"wma", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"opus", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"mts", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"m2ts", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"ts", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"3gp", Qt::CaseInsensitive) == 0) {
    return Video;
  }

  // Raw
  if (ext.compare(u"arw", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"cr2", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"dng", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"nef", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"sr2", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"srf", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"orf", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"rw2", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"pef", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"raf", Qt::CaseInsensitive) == 0) {
    return Raw;
  }

  // Image
  if (ext.compare(u"jpg", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"jpeg", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"png", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"webp", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"heic", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"heif", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"tiff", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"tif", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"bmp", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"gif", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"ico", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"tga", Qt::CaseInsensitive) == 0) {
    return Image;
  }

  return Unknown;
}

#include <QImageReader>
#include <QImageWriter>
#include <QTemporaryFile>


QVariantMap DesktopHelper::generateResizePreview(const QString &sourcePath,
                                                 int width, int height,
                                                 int quality, int compression) {
  QVariantMap result;
  result["path"] = "";
  result["size"] = 0;

  QImageReader reader(sourcePath);
  if (!reader.canRead())
    return result;

  QSize originalSize = reader.size();
  QSize targetSize = originalSize;
  if (width > 0 && height > 0) {
    targetSize.scale(width, height, Qt::KeepAspectRatio);
    reader.setScaledSize(targetSize);
  }

  QImage img = reader.read();
  if (img.isNull())
    return result;

  QSettings settings("SamsungClone", "VirtualRotations");
  int virtualRot = settings.value(sourcePath, 0).toInt();
  if (virtualRot != 0) {
    QTransform t;
    t.rotate(virtualRot);
    img = img.transformed(t, Qt::SmoothTransformation);
  }

  QString tempPath = QDir::tempPath() + "/preview_cache_" +
                     QString::number(qHash(sourcePath)) +
                     (quality == 101 ? ".png" : ".jpg");
  QImageWriter writer(tempPath);
  if (quality == 101) {
    writer.setFormat("png");
    writer.setQuality(100);
  } else {
    writer.setFormat("jpg");
    writer.setQuality(quality);
  }
  if (compression >= 0) {
    writer.setCompression(compression);
  }

  if (writer.write(img)) {
    result["path"] = "file:///" + tempPath;
    result["size"] = QFileInfo(tempPath).size();
  }
  return result;
}

void DesktopHelper::exportImages(const QStringList &paths,
                                 const QString &destinationDir, int width,
                                 int height, int quality, int compression) {
  QDir dir(destinationDir);
  if (!dir.exists())
    dir.mkpath(".");

  for (const QString &path : paths) {
    QFileInfo fi(path);
    QString destPath = dir.absoluteFilePath(fi.fileName());
    if (quality == 101) {
      destPath = dir.absoluteFilePath(fi.baseName() + ".png");
    }

    QImageReader reader(path);
    if (reader.canRead()) {
      QSize originalSize = reader.size();
      QSize targetSize = originalSize;
      if (width > 0 && height > 0) {
        targetSize.scale(width, height, Qt::KeepAspectRatio);
        reader.setScaledSize(targetSize);
      }

      QImage img = reader.read();
      if (!img.isNull()) {
        QSettings settings("SamsungClone", "VirtualRotations");
        int virtualRot = settings.value(path, 0).toInt();
        if (virtualRot != 0) {
          QTransform t;
          t.rotate(virtualRot);
          img = img.transformed(t, Qt::SmoothTransformation);
        }

        QImageWriter writer(destPath);
        if (quality == 101) {
          writer.setFormat("png");
          writer.setQuality(100);
        } else {
          writer.setQuality(quality);
        }
        if (compression >= 0)
          writer.setCompression(compression);
        writer.write(img);
      }
    }
  }
}

void DesktopHelper::copyFiles(const QStringList &paths,
                              const QString &destinationDir) {
  QDir dir(destinationDir);
  if (!dir.exists())
    dir.mkpath(".");

  for (const QString &path : paths) {
    QFileInfo fi(path);
    QString destPath = dir.absoluteFilePath(fi.fileName());
    if (QFile::exists(destPath)) {
      QFile::remove(destPath);
    }
    QFile::copy(path, destPath);
  }
}

bool DesktopHelper::isNetworkPath(const QString &path) {
  return staticIsNetworkPath(path);
}

bool DesktopHelper::isDirectory(const QString &path) {
  if (path.isEmpty())
    return false;
  return QFileInfo(QDir::fromNativeSeparators(path)).isDir();
}

qint64 DesktopHelper::getFileSize(const QString &path) {
  if (path.isEmpty())
    return 0;
  QString clean = path;
  if (clean.startsWith("file:///")) clean = clean.mid(8);
  else if (clean.startsWith("file://")) clean = clean.mid(7);
  else if (clean.startsWith("file:")) clean = clean.mid(5);
  clean = QDir::fromNativeSeparators(clean);
  return QFileInfo(clean).size();
}

bool DesktopHelper::staticIsNetworkPath(const QString &path) {
  if (path.isEmpty())
    return false;

  QString cleanPath = QDir::fromNativeSeparators(path);

  // UNC network paths: //server/share or \\server\share
  if (cleanPath.startsWith("//") || cleanPath.startsWith("\\\\")) {
    return true;
  }

  // Drive letter checks (e.g. "I:/..." or "I:\...")
  if (cleanPath.length() >= 2 && cleanPath[1] == ':') {
    QString root = cleanPath.left(3);
    if (!root.endsWith('/')) {
      root += '/';
    }

#ifdef Q_OS_WIN
    QChar driveLetter = cleanPath[0].toUpper();
    if (driveLetter >= 'A' && driveLetter <= 'Z') {
      static UINT s_driveTypeCache[26] = {0};
      int idx = driveLetter.unicode() - 'A';
      UINT driveType = s_driveTypeCache[idx];
      if (driveType == 0) {
        driveType = GetDriveTypeW((const wchar_t *)root.utf16());
        s_driveTypeCache[idx] = driveType;
      }
      return (driveType == DRIVE_REMOTE);
    }
#else
    QStorageInfo storage(root);
    if (storage.isValid()) {
      QByteArray fsType = storage.fileSystemType().toLower();
      if (fsType == "nfs" || fsType == "cifs" || fsType == "smb3" ||
          fsType == "smbfs" || fsType == "afpfs" || fsType == "webdav") {
        return true;
      }
    }
#endif
  }

#ifndef Q_OS_WIN
  QStorageInfo storage(cleanPath);
  if (storage.isValid()) {
    QByteArray fsType = storage.fileSystemType().toLower();
    if (fsType == "nfs" || fsType == "cifs" || fsType == "smb3" ||
        fsType == "smbfs" || fsType == "afpfs" || fsType == "webdav") {
      return true;
    }
  }
#endif

  return false;
}

QString DesktopHelper::urlToLocalFile(const QString &urlString) {
  QUrl url(urlString);
  if (url.isLocalFile()) {
    return url.toLocalFile();
  }
  return urlString;
}

QStringList DesktopHelper::getAdjacentFiles(const QString &filePath,
                                            int neighborWindow) {
  QStringList result;
  if (filePath.isEmpty())
    return result;

  QString cleanPath = filePath;
  if (cleanPath.startsWith("file:///"))
    cleanPath = cleanPath.mid(8);
  else if (cleanPath.startsWith("file://"))
    cleanPath = cleanPath.mid(7);
  else if (cleanPath.startsWith("file:"))
    cleanPath = cleanPath.mid(5);
  cleanPath = QDir::fromNativeSeparators(cleanPath);

  QFileInfo fileInfo(cleanPath);
  QDir dir = fileInfo.dir();
  if (!dir.exists()) {
    result.append(cleanPath);
    return result;
  }

  QStringList nameFilters;
  nameFilters << "*.jpg" << "*.jpeg" << "*.png" << "*.webp" << "*.heic"
              << "*.bmp" << "*.gif" << "*.tiff" << "*.mp4" << "*.mkv"
              << "*.avi" << "*.mov" << "*.webm" << "*.flv" << "*.ts"
              << "*.m2ts" << "*.mts" << "*.3gp" << "*.wmv" << "*.vob"
              << "*.arw" << "*.cr2" << "*.dng" << "*.nef" << "*.raf";

  QStringList files = dir.entryList(nameFilters, QDir::Files | QDir::NoSymLinks,
                                    QDir::Name | QDir::IgnoreCase);
  if (files.isEmpty()) {
    result.append(cleanPath);
    return result;
  }

  QString targetName = fileInfo.fileName();
  int targetIdx = -1;
  for (int i = 0; i < files.size(); ++i) {
    if (files[i].compare(targetName, Qt::CaseInsensitive) == 0) {
      targetIdx = i;
      break;
    }
  }

  if (targetIdx == -1) {
    result.append(cleanPath);
    return result;
  }

  int startIdx = qMax(0, targetIdx - neighborWindow);
  int endIdx = qMin(files.size() - 1, targetIdx + neighborWindow);

  for (int i = startIdx; i <= endIdx; ++i) {
    result.append(dir.absoluteFilePath(files[i]));
  }
  return result;
}
