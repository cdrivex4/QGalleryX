#ifndef DESKTOPHELPER_H
#define DESKTOPHELPER_H

#include <QObject>
#include <QString>

class DesktopHelper : public QObject {
  Q_OBJECT
public:
  explicit DesktopHelper(QObject *parent = nullptr);

  enum FileType { Unknown, Image, Video, Raw };
  Q_ENUM(FileType)

  Q_INVOKABLE QString urlToLocalFile(const QString &url);
  Q_INVOKABLE QStringList getAdjacentFiles(const QString &filePath, int neighborWindow = 15);
  Q_INVOKABLE QVariantList getMountedDrives();
  Q_INVOKABLE void openInExplorer(const QString &path);
  Q_INVOKABLE int getFileType(const QString &path);
  Q_INVOKABLE void pauseBackgroundTasks();
  Q_INVOKABLE void resumeBackgroundTasks();
  Q_INVOKABLE void requestRestart();
  Q_INVOKABLE bool isNetworkPath(const QString &path);

  // Single Source of Truth for File Types & Supported Extensions
  static const QStringList& supportedExtensions();
  static const QStringList& supportedNameFilters();
  static bool isSupportedFile(const QString &filePath);
  static FileType staticGetFileType(const QString &path);
  static bool staticIsNetworkPath(const QString &path);
};

#endif // DESKTOPHELPER_H
