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

  // Generates a preview image to a temp file and returns { "path": tempFilePath, "size": bytes, "origW": w, "origH": h, "newW": nw, "newH": nh, "origDpi": dpi }
  Q_INVOKABLE QVariantMap generateResizePreview(const QString &sourcePath, int width, int height, int quality, int compression, int dpi = 0);
  Q_INVOKABLE qint64 getTotalSize(const QStringList &paths);
  Q_INVOKABLE QVariantMap getImageDimensions(const QString &path);
  Q_INVOKABLE qint64 estimateBatchSize(const QStringList &paths, int width, int height, int quality, int compression, qint64 previewSingleBytes = 0, int previewOrigW = 0, int previewOrigH = 0);
  
  // Batch process or copy
  Q_INVOKABLE int exportImages(const QStringList &paths, const QString &destinationDir, int width, int height, int quality, int compression, const QString &suffix = QString(), int dpi = 0);
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
  Q_INVOKABLE void setFormatEngineOverride(const QString &extension, int engine);
  Q_INVOKABLE int getFormatEngineOverride(const QString &extension) const;
  Q_INVOKABLE QVariantMap getAllFormatOverrides() const;

  static void setEngine(class QQmlEngine *engine);

  // Single Source of Truth for File Types & Supported Extensions
  static const QStringList& supportedExtensions();
  static const QStringList& supportedNameFilters();
  static bool isSupportedFile(const QString &filePath);
  static FileType staticGetFileType(const QString &path);
  static bool staticIsNetworkPath(const QString &path);
  static int staticGetFormatEngineOverride(const QString &pathOrExt);
  static void staticSetFormatEngineOverride(const QString &ext, int engine);

private:
  static class QQmlEngine *s_engine;
};

#endif // DESKTOPHELPER_H
