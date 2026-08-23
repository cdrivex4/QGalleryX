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
#ifdef _WIN32
  HANDLE hCurThread = GetCurrentThread();
  int oldPriority = GetThreadPriority(hCurThread);
  SetThreadPriority(hCurThread, THREAD_PRIORITY_BELOW_NORMAL);
#endif

  // Extract volume root (e.g., "C:\" -> "\\.\C:")
  QString vol = volumePath.left(3);
  m_volumeDrive = vol.left(2);
  QString devicePath = "\\\\.\\" + m_volumeDrive;

  if (!openVolume(devicePath)) {
    qWarning() << "FastScanner: Failed to open volume" << devicePath
               << "(Admin rights required?)";
#ifdef _WIN32
    SetThreadPriority(hCurThread, oldPriority);
#endif
    return false;
  }

  QElapsedTimer timer;
  timer.start();

  if (!enumerateFiles(vol)) {
    qWarning() << "FastScanner: Failed to enumerate MFT";
#ifdef _WIN32
    SetThreadPriority(hCurThread, oldPriority);
#endif
    return false;
  }

  qDebug() << "FastScanner: MFT Enumeration took" << timer.elapsed() << "ms";
  timer.restart();

  buildPaths();
  qDebug() << "FastScanner: Path Reconstruction took" << timer.elapsed()
           << "ms";

#ifdef _WIN32
  SetThreadPriority(hCurThread, oldPriority);
#endif
  return true;
}

bool FastVolumeScanner::openVolume(const QString &volName) {
  m_hVol =
      CreateFileW((LPCWSTR)volName.utf16(), GENERIC_READ,
                  FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                  FILE_FLAG_NO_BUFFERING | FILE_FLAG_BACKUP_SEMANTICS, NULL);
  return (m_hVol != INVALID_HANDLE_VALUE);
}

#include "NtfsMft.h"

bool FastVolumeScanner::enumerateFiles(const QString &volRoot) {
  NTFS_VOLUME_DATA_BUFFER volData;
  DWORD br;
  if (!DeviceIoControl(m_hVol, FSCTL_GET_NTFS_VOLUME_DATA, NULL, 0, &volData,
                       sizeof(volData), &br, NULL)) {
    qWarning() << "Failed to get NTFS volume data.";
    return false;
  }

  uint64_t bytesPerRecord = volData.BytesPerFileRecordSegment;
  size_t estimatedFiles = volData.MftValidDataLength.QuadPart / bytesPerRecord;

  m_soa.frns.reserve(estimatedFiles);
  m_soa.parentFrns.reserve(estimatedFiles);
  m_soa.fileSizes.reserve(estimatedFiles);
  m_soa.creationTimes.reserve(estimatedFiles);
  m_soa.nameOffsets.reserve(estimatedFiles);
  m_soa.nameLengths.reserve(estimatedFiles);
  m_soa.isDir.reserve(estimatedFiles);
  m_soa.stringPool.reserve(estimatedFiles * 40);
  m_frnToIndex.reserve(estimatedFiles);

  DWORD chunkSize = 256 * 1024; // 256 KB
  void *buffer =
      VirtualAlloc(NULL, chunkSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  if (!buffer)
    return false;

  // 1. Query physical MFT cluster extents via FSCTL_GET_RETRIEVAL_POINTERS
  struct ExtentRun {
    LARGE_INTEGER startOffset;
    uint64_t lengthBytes;
  };
  std::vector<ExtentRun> runs;

  QString mftPath = volRoot + "$MFT";
  HANDLE hMft = CreateFileW((LPCWSTR)mftPath.utf16(), FILE_READ_ATTRIBUTES,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            NULL, OPEN_EXISTING, 0, NULL);

  if (hMft != INVALID_HANDLE_VALUE) {
    STARTING_VCN_INPUT_BUFFER inputVcn = {};
    DWORD rpBufSize = 64 * 1024; // 64 KB for extents list
    std::vector<BYTE> rpBuffer(rpBufSize);
    RETRIEVAL_POINTERS_BUFFER *rp = (RETRIEVAL_POINTERS_BUFFER *)rpBuffer.data();

    if (DeviceIoControl(hMft, FSCTL_GET_RETRIEVAL_POINTERS, &inputVcn, sizeof(inputVcn),
                        rp, rpBufSize, &br, NULL)) {
      int64_t currentVcn = rp->StartingVcn.QuadPart;
      for (DWORD e = 0; e < rp->ExtentCount; ++e) {
        if (rp->Extents[e].Lcn.QuadPart != (LONGLONG)-1) {
          int64_t clusterCount = rp->Extents[e].NextVcn.QuadPart - currentVcn;
          ExtentRun run;
          run.startOffset.QuadPart = rp->Extents[e].Lcn.QuadPart * volData.BytesPerCluster;
          run.lengthBytes = clusterCount * volData.BytesPerCluster;
          runs.push_back(run);
        }
        currentVcn = rp->Extents[e].NextVcn.QuadPart;
      }
    }
    CloseHandle(hMft);
  }

  // 2. Fallback to start LCN if retrieval pointers unavailable
  if (runs.empty()) {
    ExtentRun singleRun;
    singleRun.startOffset.QuadPart = volData.MftStartLcn.QuadPart * volData.BytesPerCluster;
    singleRun.lengthBytes = volData.MftValidDataLength.QuadPart;
    runs.push_back(singleRun);
  }

  uint64_t recordsProcessed = 0;
  for (const auto &run : runs) {
    if (!SetFilePointerEx(m_hVol, run.startOffset, NULL, FILE_BEGIN)) {
      continue;
    }

    uint64_t bytesReadInRun = 0;
    while (bytesReadInRun < run.lengthBytes && recordsProcessed < estimatedFiles) {
      DWORD toRead = (DWORD)std::min((uint64_t)chunkSize, run.lengthBytes - bytesReadInRun);
      DWORD bytesRead = 0;
      if (!ReadFile(m_hVol, buffer, toRead, &bytesRead, NULL) || bytesRead == 0) {
        break;
      }
      bytesReadInRun += bytesRead;

    for (DWORD i = 0; i < bytesRead; i += bytesPerRecord) {
      MFT_RECORD_HEADER *record = (MFT_RECORD_HEADER *)((char *)buffer + i);

      if (record->magic != 0x454C4946) { // "FILE"
        recordsProcessed++;
        continue;
      }
      if (!(record->flags & 1)) { // In use flag
        recordsProcessed++;
        continue;
      }

      uint64_t frn = record->mftRecordNumber;
      uint64_t parentFrn = 0;
      uint64_t fileSize = 0;
      int64_t creationTime = 0;
      QString name;
      bool isDir = (record->flags & 2);

      // Parse attributes
      uint16_t attrOffset = record->firstAttributeOffset;
      while (attrOffset < bytesPerRecord) {
        ATTRIBUTE_HEADER *attr =
            (ATTRIBUTE_HEADER *)((char *)record + attrOffset);
        if (attr->type == 0xFFFFFFFF)
          break; // End of attributes

        if (attr->type == 0x10) { // $STANDARD_INFORMATION
          if (!attr->nonResident) {
            RESIDENT_ATTRIBUTE_HEADER *res = (RESIDENT_ATTRIBUTE_HEADER *)attr;
            STANDARD_INFORMATION *si =
                (STANDARD_INFORMATION *)((char *)attr + res->valueOffset);
            creationTime = si->creationTime;
          }
        } else if (attr->type == 0x30) { // $FILE_NAME
          if (!attr->nonResident) {
            RESIDENT_ATTRIBUTE_HEADER *res = (RESIDENT_ATTRIBUTE_HEADER *)attr;
            FILE_NAME_ATTRIBUTE *fn =
                (FILE_NAME_ATTRIBUTE *)((char *)attr + res->valueOffset);

            // Prefer Win32 or DOS+Win32 name spaces (avoid short 8.3 names if
            // possible)
            if (name.isEmpty() || fn->nameType == 1 || fn->nameType == 3) {
              parentFrn = fn->parentDirectory & 0x0000FFFFFFFFFFFFULL;
              name = QString::fromWCharArray(fn->name, fn->nameLength);
              // File name attribute also contains realSize!
              if (fileSize == 0)
                fileSize = fn->realSize;
            }
          }
        } else if (attr->type == 0x80) { // $DATA
          if (attr->nonResident) {
            NON_RESIDENT_ATTRIBUTE_HEADER *nonRes =
                (NON_RESIDENT_ATTRIBUTE_HEADER *)attr;
            fileSize = nonRes->realSize;
          } else {
            RESIDENT_ATTRIBUTE_HEADER *res = (RESIDENT_ATTRIBUTE_HEADER *)attr;
            fileSize = res->valueLength;
          }
        }

        if (attr->length == 0)
          break; // Prevent infinite loop on corruption
        attrOffset += attr->length;
      }

      if (!name.isEmpty()) {
        QByteArray utf8Name = name.toUtf8();
        uint32_t nameOffset = m_soa.stringPool.size();
        uint32_t nameLength = utf8Name.size();
        m_soa.stringPool.insert(m_soa.stringPool.end(), utf8Name.constData(),
                                utf8Name.constData() + nameLength);
        m_soa.stringPool.push_back('\0');

        size_t index = m_soa.frns.size();
        m_soa.frns.push_back(frn);
        m_soa.parentFrns.push_back(parentFrn);
        m_soa.fileSizes.push_back(fileSize);
        m_soa.creationTimes.push_back(creationTime);
        m_soa.nameOffsets.push_back(nameOffset);
        m_soa.nameLengths.push_back(nameLength);
        m_soa.isDir.push_back(isDir);

        m_frnToIndex[frn] = index;
      }
      recordsProcessed++;
    }
  }
  }

  VirtualFree(buffer, 0, MEM_RELEASE);
  return true;
}

void FastVolumeScanner::buildPaths() {
  m_scannedFiles.clear();
  m_scannedFiles.reserve(m_soa.frns.size());

  std::unordered_map<DWORDLONG, QString> pathCache;
  QString drivePrefix = m_volumeDrive;
  if (drivePrefix.isEmpty()) drivePrefix = "C:";

  // Pre-seed root FRN 5 (NTFS volume root)
  pathCache[5] = drivePrefix;

  auto resolvePath = [&](DWORDLONG frn, int depth, auto &self) -> QString {
    if (depth > 64) return drivePrefix; // Guard against circular MFT loops / deep recursion stack overflow
    if (pathCache.count(frn))
      return pathCache[frn];

    if (frn == 5 || frn == 0) {
      pathCache[frn] = drivePrefix;
      return drivePrefix;
    }

    auto it = m_frnToIndex.find(frn);
    if (it == m_frnToIndex.end())
      return drivePrefix; // Root or orphan

    size_t index = it->second;
    if (index >= m_soa.parentFrns.size())
      return drivePrefix;

    DWORDLONG parentFrn = m_soa.parentFrns[index];
    uint32_t nameOffset = m_soa.nameOffsets[index];
    uint32_t nameLength = m_soa.nameLengths[index];
    if (nameOffset + nameLength > m_soa.stringPool.size())
      return drivePrefix;

    QString name =
        QString::fromUtf8(m_soa.stringPool.data() + nameOffset,
                          nameLength);

    if (name.isEmpty() || name == "." || name == "$Root" || parentFrn == frn) {
      pathCache[frn] = drivePrefix;
      return drivePrefix;
    }

    QString parentPath = self(parentFrn, depth + 1, self);
    QString fullPath;
    if (parentPath.isEmpty() || parentPath == drivePrefix) {
      fullPath = drivePrefix + "/" + name;
    } else {
      fullPath = parentPath + "/" + name;
    }

    pathCache[frn] = fullPath;
    return fullPath;
  };

  for (size_t i = 0; i < m_soa.frns.size(); ++i) {
    if (m_soa.isDir[i])
      continue;

    QString fullPath = resolvePath(m_soa.frns[i], 0, resolvePath);

    ScannedFile sf;
    sf.path = fullPath;
    sf.size = m_soa.fileSizes[i];
    sf.creationTime = m_soa.creationTimes[i];

    m_scannedFiles.append(sf);
  }
}
