#include "DesktopHelper.h"
#include "TaskScheduler.h" // Include TaskScheduler.h
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QUrl>

// ... keep includes ...

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

  QString param = QLatin1String("/select,") + nativePath;
  QProcess::startDetached("explorer.exe", QStringList() << param);
}

void DesktopHelper::pauseBackgroundTasks() {
  TaskScheduler::instance().pause();
  qDebug() << "DesktopHelper: Paused background tasks.";
}

void DesktopHelper::resumeBackgroundTasks() {
  TaskScheduler::instance().resume();
  qDebug() << "DesktopHelper: Resumed background tasks.";
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
