#include "DesktopHelper.h"
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QUrl>

DesktopHelper::DesktopHelper(QObject *parent) : QObject(parent) {}

void DesktopHelper::openInExplorer(const QString &path) {
  if (path.isEmpty())
    return;

  // Clean up path (remove file:/// prefix if present)
  QString cleanPath = path;
  if (cleanPath.startsWith("file:///")) {
    cleanPath = cleanPath.mid(8);
  } else if (cleanPath.startsWith("file://")) {
    cleanPath = cleanPath.mid(7);
  }

  QFileInfo fileInfo(cleanPath);
  if (!fileInfo.exists()) {
    qWarning() << "File does not exist:" << cleanPath;
    return;
  }

  QString nativePath = QDir::toNativeSeparators(cleanPath);

#ifdef Q_OS_WIN
  // Use explorer.exe /select, <path> to highlight the file
  // We use setNativeArguments to avoid QProcess auto-quoting the comma argument
  // incorrectly
  QProcess process;
  process.setProgram("explorer.exe");
  QString args = "/select,\"" + nativePath + "\"";
  process.setNativeArguments(args);
  process.startDetached();
#else
  // Fallback for non-Windows (just open folder)
  QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.absolutePath()));
#endif
}
