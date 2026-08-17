#ifndef FASTVOLUMESCANNER_H
#define FASTVOLUMESCANNER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <vector>
#include <unordered_map>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winioctl.h>

struct VolumeIndexSoA {
  std::vector<DWORDLONG> frns;
  std::vector<DWORDLONG> parentFrns;
  std::vector<uint64_t> fileSizes;
  std::vector<int64_t> creationTimes;
  std::vector<uint32_t> nameOffsets;
  std::vector<uint32_t> nameLengths;
  std::vector<bool> isDir;
  std::vector<char> stringPool;
};

struct ScannedFile {
    QString path;
    uint64_t size;
    int64_t creationTime;
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
  QVector<ScannedFile> getScannedFiles() const { return m_scannedFiles; }
  
  const VolumeIndexSoA& getIndex() const { return m_soa; }

private:
  HANDLE m_hVol;
  QString m_volumeDrive;
  VolumeIndexSoA m_soa;
  QVector<ScannedFile> m_scannedFiles;
  std::unordered_map<DWORDLONG, size_t> m_frnToIndex;

  bool openVolume(const QString &volName);
  bool enumerateFiles(const QString &volRoot);
  void buildPaths();
};

#endif // FASTVOLUMESCANNER_H
