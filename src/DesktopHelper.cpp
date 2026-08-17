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

DesktopHelper::FileType DesktopHelper::staticGetFileType(const QString &path) {
  int dotIdx = path.lastIndexOf('.');
  if (dotIdx == -1 || dotIdx == path.length() - 1)
    return Unknown;

  QStringView ext = QStringView(path).mid(dotIdx + 1);

  // Video
  if (ext.compare(u"mp4", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"mkv", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"avi", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"mov", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"webm", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"flv", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"vob", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"ogg", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"ogv", Qt::CaseInsensitive) == 0 ||
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
