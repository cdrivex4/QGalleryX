#include "DesktopHelper.h"
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStorageInfo>
#include <QUrl>

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
  QString ext = QFileInfo(path).suffix().toLower();

  // Video
  if (ext == "mp4" || ext == "mkv" || ext == "avi" || ext == "mov" ||
      ext == "webm" || ext == "flv" || ext == "vob" || ext == "ogg" ||
      ext == "ogv" || ext == "mts" || ext == "m2ts" || ext == "ts" ||
      ext == "3gp") {
    return Video;
  }

  // Raw
  if (ext == "arw" || ext == "cr2" || ext == "dng" || ext == "nef" ||
      ext == "sr2" || ext == "srf" || ext == "orf" || ext == "rw2" ||
      ext == "pef" || ext == "raf") {
    return Raw;
  }

  // Image
  if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "webp" ||
      ext == "heic" || ext == "tiff" || ext == "bmp" || ext == "gif") {
    return Image;
  }

  return Unknown;
}

#include <QTemporaryFile>
#include <QImageReader>
#include <QImageWriter>

QVariantMap DesktopHelper::generateResizePreview(const QString &sourcePath, int width, int height, int quality, int compression) {
    QVariantMap result;
    result["path"] = "";
    result["size"] = 0;

    QImageReader reader(sourcePath);
    if (!reader.canRead()) return result;

    QSize originalSize = reader.size();
    QSize targetSize = originalSize;
    if (width > 0 && height > 0) {
        targetSize.scale(width, height, Qt::KeepAspectRatio);
        reader.setScaledSize(targetSize);
    }

    QImage img = reader.read();
    if (img.isNull()) return result;

    QString tempPath = QDir::tempPath() + "/preview_antigravity.jpg";
    QImageWriter writer(tempPath, "jpg");
    writer.setQuality(quality);
    if (compression >= 0) {
        writer.setCompression(compression);
    }
    
    if (writer.write(img)) {
        result["path"] = "file:///" + tempPath;
        result["size"] = QFileInfo(tempPath).size();
    }
    return result;
}

void DesktopHelper::exportImages(const QStringList &paths, const QString &destinationDir, int width, int height, int quality, int compression) {
    QDir dir(destinationDir);
    if (!dir.exists()) dir.mkpath(".");

    for (const QString &path : paths) {
        QFileInfo fi(path);
        QString destPath = dir.absoluteFilePath(fi.fileName());

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
                QImageWriter writer(destPath);
                writer.setQuality(quality);
                if (compression >= 0) writer.setCompression(compression);
                writer.write(img);
            }
        }
    }
}

void DesktopHelper::copyFiles(const QStringList &paths, const QString &destinationDir) {
    QDir dir(destinationDir);
    if (!dir.exists()) dir.mkpath(".");

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

    QStorageInfo storage(root);
    if (storage.isValid()) {
      QString fsType = storage.fileSystemType().toLower();
      if (fsType == "network" || fsType == "cifs" || fsType == "smb" || fsType == "nfs") {
        return true;
      }
    }
  }

  return false;
}
