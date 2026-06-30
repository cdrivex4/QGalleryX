#include "DesktopHelper.h"
#include "TaskScheduler.h" // Include TaskScheduler.h
#include <QCoreApplication>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QUrl>

// ... keep includes ...

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
  QProcess::startDetached(QCoreApplication::applicationFilePath(), QCoreApplication::arguments());
  QCoreApplication::quit();
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
  if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" ||
      ext == "bmp" || ext == "webp" || ext == "heic" || ext == "tiff" ||
      ext == "tif" || ext == "ico" || ext == "tga") {
    return Image;
  }

  return Unknown;
}
