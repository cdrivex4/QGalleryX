#ifndef FASTVOLUMESCANNER_H
#define FASTVOLUMESCANNER_H

#include <QMap>
#include <QObject>
#include <QString>
#include <QVector>
#include <windows.h>
#include <winioctl.h>

struct FileInfo {
  QString name;
  DWORDLONG parentFrn;
  bool isDir;
};

class FastVolumeScanner : public QObject {
  Q_OBJECT
public:
  explicit FastVolumeScanner(QObject *parent = nullptr);
  ~FastVolumeScanner();

  // Scans the volume of the given path (e.g., "C:/")
  // Returns true if successful (requires Admin)
  bool scanVolume(const QString &volumePath);

  // Retrieve results
  QVector<QString> getAllFiles() const;
  QVector<QString> getAllDirectories() const;

private:
  HANDLE m_hVol;
  QMap<DWORDLONG, FileInfo> m_fileMap;
  QVector<QString> m_files;
  QVector<QString> m_dirs;

  bool openVolume(const QString &volName);
  bool getUsnJournalState(USN_JOURNAL_DATA &ujData);
  bool enumerateFiles(const USN_JOURNAL_DATA &ujData);
  void buildPaths();
};

#endif // FASTVOLUMESCANNER_H
