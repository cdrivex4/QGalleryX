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

  Q_INVOKABLE void openInExplorer(const QString &path);
  Q_INVOKABLE int getFileType(const QString &path);

  // Generates a preview image to a temp file and returns { "path": tempFilePath, "size": bytes }
  Q_INVOKABLE QVariantMap generateResizePreview(const QString &sourcePath, int width, int height, int quality, int compression);
  
  // Batch process or copy
  Q_INVOKABLE void exportImages(const QStringList &paths, const QString &destinationDir, int width, int height, int quality, int compression);
  Q_INVOKABLE void copyFiles(const QStringList &paths, const QString &destinationDir);

  Q_INVOKABLE bool isNetworkPath(const QString &path);
  Q_INVOKABLE bool isDirectory(const QString &path);
  Q_INVOKABLE qint64 getFileSize(const QString &path);
  Q_INVOKABLE QString urlToLocalFile(const QString &url);
  Q_INVOKABLE QStringList getAdjacentFiles(const QString &filePath, int neighborWindow = 15);
  Q_INVOKABLE QVariantList getMountedDrives();
  Q_INVOKABLE bool isRunningAsAdmin() const;
  Q_INVOKABLE bool relaunchAsAdmin(const QString &folderToOpen = QString());
  Q_INVOKABLE bool relaunchAsStandardUser(const QString &folderToOpen = QString());
  Q_INVOKABLE void openNewWindow(const QString &folderPath = QString());

  static void setEngine(class QQmlEngine *engine);

  // Single Source of Truth for File Types & Supported Extensions
  static const QStringList& supportedExtensions();
  static const QStringList& supportedNameFilters();
  static bool isSupportedFile(const QString &filePath);
  static FileType staticGetFileType(const QString &path);
  static bool staticIsNetworkPath(const QString &path);
  static QString canonicalizePath(const QString &input, bool preserveQuery = false);

private:
  static class QQmlEngine *s_engine;
};

#endif // DESKTOPHELPER_H
