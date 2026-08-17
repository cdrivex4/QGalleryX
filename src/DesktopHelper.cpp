#include "DesktopHelper.h"
#include "TaskScheduler.h" // Include TaskScheduler.h
#include <QCoreApplication>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStorageInfo>
#include <QUrl>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

DesktopHelper::DesktopHelper(QObject *parent) : QObject(parent) {}

void DesktopHelper::openInExplorer(const QString &path) {
  if (path.isEmpty() || path.startsWith("synthetic:"))
    return;

  QString cleanPath = path;
  if (cleanPath.startsWith("file:///")) {
    cleanPath = cleanPath.mid(8);
  } else if (cleanPath.startsWith("file://")) {
    cleanPath = cleanPath.mid(7);
  } else if (cleanPath.startsWith("file:/")) {
    cleanPath = cleanPath.mid(6);
  }

  QString nativePath = QDir::toNativeSeparators(cleanPath);

  QProcess process;
  process.setProgram("explorer.exe");
  process.setNativeArguments("/select,\"" + nativePath + "\"");
  process.startDetached();
}

QString DesktopHelper::urlToLocalFile(const QString &urlString) {
  QUrl url(urlString);
  if (url.isLocalFile()) {
    return url.toLocalFile();
  }
  return urlString;
}

QStringList DesktopHelper::getAdjacentFiles(const QString &filePath, int neighborWindow) {
  QStringList result;
  if (filePath.isEmpty()) return result;

  QString cleanPath = urlToLocalFile(filePath);
  QFileInfo fileInfo(cleanPath);
  QDir dir = fileInfo.dir();

  const QStringList &nameFilters = supportedNameFilters();

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

void DesktopHelper::pauseBackgroundTasks() {
  TaskScheduler::instance().pause();
  qDebug() << "DesktopHelper: Paused background tasks.";
}

void DesktopHelper::resumeBackgroundTasks() {
  TaskScheduler::instance().resume();
  qDebug() << "DesktopHelper: Resumed background tasks.";
}

void DesktopHelper::requestRestart() {
  qDebug() << "DesktopHelper: Restarting application...";
  QProcess::startDetached(QCoreApplication::applicationFilePath(),
                          QCoreApplication::arguments());
  QCoreApplication::quit();
}

int DesktopHelper::getFileType(const QString &path) {
  return staticGetFileType(path);
}

const QStringList& DesktopHelper::supportedExtensions() {
  static const QStringList s_extensions = {
      // Images
      "jpg", "jpeg", "png", "webp", "heic", "heif", "tiff", "tif", "bmp", "gif", "ico", "tga", "avif", "jfif",
      // Videos & Audio
      "mp4", "mkv", "avi", "mov", "webm", "flv", "vob", "ogg", "ogv", "mp3", "wav", "flac", "m4a", "aac", "wma", "opus", "mts", "m2ts", "ts", "3gp", "wmv", "m4v", "mpg", "mpeg",
      // RAW Formats
      "arw", "cr2", "cr3", "dng", "nef", "nrw", "orf", "rw2", "pef", "raf", "sr2", "srf", "kdc", "dcr", "raw"
  };
  return s_extensions;
}

const QStringList& DesktopHelper::supportedNameFilters() {
  static const QStringList s_filters = []() {
    QStringList filters;
    const auto& exts = supportedExtensions();
    filters.reserve(exts.size());
    for (const QString& ext : exts) {
      filters.append("*." + ext);
    }
    return filters;
  }();
  return s_filters;
}

bool DesktopHelper::isSupportedFile(const QString &filePath) {
  return staticGetFileType(filePath) != Unknown;
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
      ext.compare(u"3gp", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"wmv", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"m4v", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"mpg", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"mpeg", Qt::CaseInsensitive) == 0) {
    return Video;
  }

  // Raw
  if (ext.compare(u"arw", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"cr2", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"cr3", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"dng", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"nef", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"nrw", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"orf", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"rw2", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"pef", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"raf", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"sr2", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"srf", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"kdc", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"dcr", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"raw", Qt::CaseInsensitive) == 0) {
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
      ext.compare(u"tga", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"avif", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"jfif", Qt::CaseInsensitive) == 0) {
    return Image;
  }

  return Unknown;
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
      if (driveType == DRIVE_REMOTE || driveType == DRIVE_REMOVABLE ||
          driveType == DRIVE_CDROM) {
        return true;
      }
    }
#else
    QStorageInfo storage(root);
    if (storage.isValid()) {
      QString fsType = storage.fileSystemType().toLower();
      if (fsType == "network" || fsType == "cifs" || fsType == "smb" ||
          fsType == "nfs") {
        return true;
      }
    }
#endif
  }

  return false;
}

QVariantList DesktopHelper::getMountedDrives() {
  QVariantList drives;
  const auto volumes = QStorageInfo::mountedVolumes();
  for (const QStorageInfo &storage : volumes) {
    if (!storage.isValid() || !storage.isReady())
      continue;

    QString rootPath = QDir::toNativeSeparators(storage.rootPath());
    if (rootPath.endsWith('\\') && rootPath.length() > 3) {
      rootPath.chop(1);
    }

    QVariantMap drive;
    drive["rootPath"] = rootPath;
    drive["name"] = storage.name().isEmpty() ? rootPath : storage.name();
    drive["displayName"] = QString("%1 (%2)").arg(storage.name().isEmpty() ? "Local Disk" : storage.name(), rootPath);
    drive["fileSystemType"] = QString::fromLatin1(storage.fileSystemType());
    drive["bytesTotal"] = storage.bytesTotal();
    drive["bytesAvailable"] = storage.bytesAvailable();
    drive["bytesFree"] = storage.bytesFree();
    drive["isReadOnly"] = storage.isReadOnly();

    QString type = "FIXED";
#ifdef Q_OS_WIN
    if (rootPath.length() >= 2 && rootPath[1] == ':') {
      QString rootWin = rootPath.left(2) + "\\";
      UINT dType = GetDriveTypeW((const wchar_t *)rootWin.utf16());
      switch (dType) {
        case DRIVE_REMOVABLE: type = "REMOVABLE"; break; // USB / SD Card
        case DRIVE_FIXED:     type = "FIXED"; break;     // Internal NVMe / SATA
        case DRIVE_REMOTE:    type = "REMOTE"; break;    // Network share / SMB
        case DRIVE_CDROM:     type = "CDROM"; break;
        case DRIVE_RAMDISK:   type = "RAMDISK"; break;
        default:              type = "UNKNOWN"; break;
      }
    } else if (rootPath.startsWith("\\\\")) {
      type = "REMOTE";
    }
#endif
    drive["driveType"] = type;
    drives.append(drive);
  }
  return drives;
}

