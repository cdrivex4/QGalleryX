#include "FastVolumeScanner.h"
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>

FastVolumeScanner::FastVolumeScanner(QObject *parent)
    : QObject(parent), m_hVol(INVALID_HANDLE_VALUE) {}

FastVolumeScanner::~FastVolumeScanner() {
  if (m_hVol != INVALID_HANDLE_VALUE) {
    CloseHandle(m_hVol);
  }
}

bool FastVolumeScanner::scanVolume(const QString &volumePath) {
  // Extract volume root (e.g., "C:\" -> "\\.\C:")
  QString vol = volumePath.left(3);
  QString devicePath = "\\\\.\\" + vol.left(2);

  if (!openVolume(devicePath)) {
    qWarning() << "FastScanner: Failed to open volume" << devicePath
               << "(Admin rights required?)";
    return false;
  }

  USN_JOURNAL_DATA ujData;
  if (!getUsnJournalState(ujData)) {
    qWarning() << "FastScanner: Failed to query USN Journal";
    return false;
  }

  QElapsedTimer timer;
  timer.start();

  if (!enumerateFiles(ujData)) {
    qWarning() << "FastScanner: Failed to enumerate MFT";
    return false;
  }

  qDebug() << "FastScanner: MFT Enumeration took" << timer.elapsed() << "ms";
  timer.restart();

  buildPaths();
  qDebug() << "FastScanner: Path Reconstruction took" << timer.elapsed()
           << "ms";

  return true;
}

bool FastVolumeScanner::openVolume(const QString &volName) {
  m_hVol = CreateFileW((LPCWSTR)volName.utf16(), GENERIC_READ,
                       FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                       0, NULL);
  return (m_hVol != INVALID_HANDLE_VALUE);
}

bool FastVolumeScanner::getUsnJournalState(USN_JOURNAL_DATA &ujData) {
  DWORD br;
  return DeviceIoControl(m_hVol, FSCTL_QUERY_USN_JOURNAL, NULL, 0, &ujData,
                         sizeof(ujData), &br, NULL);
}

bool FastVolumeScanner::enumerateFiles(const USN_JOURNAL_DATA &ujData) {
  MFT_ENUM_DATA enumData;
  enumData.StartFileReferenceNumber = 0;
  enumData.LowUsn = 0;
  enumData.HighUsn = ujData.NextUsn;

  char buffer[65536]; // 64KB buffer
  DWORD br;

  while (DeviceIoControl(m_hVol, FSCTL_ENUM_USN_DATA, &enumData,
                         sizeof(enumData), buffer, sizeof(buffer), &br, NULL)) {
    DWORD offset = sizeof(USN);
    while (offset < br) {
      PUSN_RECORD record = (PUSN_RECORD)(buffer + offset);

      // Basic Filter: Skip system files if needed, here we take all valid ones
      // We only care about normal files and directories

      QString name = QString::fromWCharArray(
          (wchar_t *)((char *)record + record->FileNameOffset),
          record->FileNameLength / 2);

      FileInfo info;
      info.name = name;
      info.parentFrn = record->ParentFileReferenceNumber;
      info.isDir = (record->FileAttributes & FILE_ATTRIBUTE_DIRECTORY);

      // Store by FileReferenceNumber (FRN)
      // FRN is 64-bit ID unique to file
      m_fileMap.insert(record->FileReferenceNumber, info);

      offset += record->RecordLength;
    }
    // Continue from where we left off
    enumData.StartFileReferenceNumber = *(DWORDLONG *)buffer;
  }

  // ERROR_HANDLE_EOF is normal termination
  return (GetLastError() == ERROR_HANDLE_EOF);
}

void FastVolumeScanner::buildPaths() {
  m_files.clear();
  m_dirs.clear();
  m_files.reserve(m_fileMap.size());

  // Reconstruct full paths by walking up the parent chain
  // This is the CPU intensive part, can be parallelized or cached

  // Optimization: Cache built paths?
  // Use a temporary cache for parent paths: QMap<FRN, QString> pathCache
  QMap<DWORDLONG, QString> pathCache;

  auto resolvePath = [&](DWORDLONG frn, auto &self) -> QString {
    if (pathCache.contains(frn))
      return pathCache[frn];

    if (!m_fileMap.contains(frn))
      return ""; // Root or orphan

    const FileInfo &info = m_fileMap[frn];
    if (info.parentFrn == 0 || info.parentFrn == frn) {
      // Root detection issues? usually root has specific ID
      return info.name;
    }

    QString parentPath = self(info.parentFrn, self);
    QString fullPath = parentPath + "/" + info.name;

    pathCache.insert(frn, fullPath);
    return fullPath;
  };

  // Naive iterative approach for now to avoid stack overflow recursion on deep
  // structures Actually, standard iteration and looking up parent is fast
  // enough for basic test

  QMapIterator<DWORDLONG, FileInfo> i(m_fileMap);
  while (i.hasNext()) {
    i.next();
    const FileInfo &info = i.value();

    // We only construct paths for things we care about?
    // Let's implement full resolution

    QString fullPath = resolvePath(i.key(), resolvePath);

    // Add Volume Prefix (e.g. C:) ?
    // m_hVol was opened on volume path. resolvePath builds relative to root of
    // volume. We should prepend the drive letter if needed, but for now
    // returned paths are relative to volume root? Actually resolvePath uses "/"
    // separator. If parent is root, parentPath might be empty.

    if (info.isDir)
      m_dirs.append(fullPath);
    else
      m_files.append(fullPath);
  }
}

QVector<QString> FastVolumeScanner::getAllFiles() const { return m_files; }
QVector<QString> FastVolumeScanner::getAllDirectories() const { return m_dirs; }
