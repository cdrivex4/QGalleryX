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
  Q_INVOKABLE void pauseBackgroundTasks();
  Q_INVOKABLE void resumeBackgroundTasks();

  // Static helper for C++ usage
  static FileType staticGetFileType(const QString &path);
};

#endif // DESKTOPHELPER_H
