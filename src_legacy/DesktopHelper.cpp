#include "DesktopHelper.h"
#include <QDesktopServices>
#include <QFileInfo>
#include <QGuiApplication>
#include <QProcess>
#include <QStandardPaths>
#include <QSettings>
#include <QStorageInfo>
#include <QTransform>
#include <QUrl>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QWindow>


#ifdef Q_OS_WIN
#include <windows.h>
#endif

DesktopHelper::DesktopHelper(QObject *parent) : QObject(parent) {}

QString DesktopHelper::toMediaUrl(const QString &filePath) {
  if (filePath.isEmpty()) return QString();

  // If it's already a properly-formed URL, return as-is
  if (filePath.startsWith("file:///") || filePath.startsWith("http://") ||
      filePath.startsWith("https://") || filePath.startsWith("qrc:")) {
    return filePath;
  }

  // Normalize path separators and strip leading slash before drive letter
  // e.g. /C:/Users → C:/Users (QUrl artefact from some QML drop events)
  QString normalized = filePath;
  normalized.replace('\\', '/');
  if (normalized.startsWith("/") && normalized.length() > 2 && normalized[2] == ':') {
    normalized = normalized.mid(1);
  }

  // QUrl::fromLocalFile() is the authoritative Qt encoder — correctly handles
  // spaces → %20, # → %23, & → %26, Unicode, UNC paths (\\server\share),
  // and all other characters that would break a raw file:/// string.
  return QUrl::fromLocalFile(normalized).toString();
}

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

  QString param;
  if (!QFileInfo(nativePath).isDir())
    param = QLatin1String("/select,");

  QProcess::startDetached("explorer.exe", QStringList() << param << nativePath);
}

int DesktopHelper::getFileType(const QString &path) {
  return staticGetFileType(path);
}

const QStringList& DesktopHelper::supportedExtensions() {
  static const QStringList s_extensions = {
      // Images
      "jpg", "jpeg", "png", "webp", "heic", "heif", "tiff", "tif", "bmp", "gif", "ico", "tga", "avif", "jfif",
      // Videos
      "mp4", "mkv", "avi", "mov", "webm", "flv", "vob", "ogv", "mts", "m2ts", "3gp", "wmv", "m4v", "mpg", "mpeg",
      // RAW Formats
      "arw", "cr2", "cr3", "dng", "nef", "nrw", "orf", "rw2", "pef", "raf", "sr2", "srf", "kdc", "dcr", "raw"
  };
  return s_extensions;
}

const QStringList& DesktopHelper::supportedNameFilters() {
  static const QStringList s_filters = []() {
    QStringList filters;
    const auto& exts = supportedExtensions();
    filters.reserve(exts.size());
    for (const QString& ext : exts) {
      filters.append("*." + ext);
    }
    return filters;
  }();
  return s_filters;
}

bool DesktopHelper::isSupportedFile(const QString &filePath) {
  return staticGetFileType(filePath) != Unknown;
}

DesktopHelper::FileType DesktopHelper::staticGetFileType(const QString &path) {
  int dotIdx = path.lastIndexOf('.');
  if (dotIdx == -1 || dotIdx == path.length() - 1)
    return Unknown;

  QStringView ext = QStringView(path).mid(dotIdx + 1);

  // Video & Audio Media
  if (ext.compare(u"mp4", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"mkv", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"avi", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"mov", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"webm", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"flv", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"vob", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"ogg", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"ogv", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"mp3", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"wav", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"flac", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"m4a", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"aac", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"wma", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"opus", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"mts", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"m2ts", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"3gp", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"wmv", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"m4v", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"mpg", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"mpeg", Qt::CaseInsensitive) == 0) {
    return Video;
  }

  // Disambiguate MPEG-TS Video (.ts) vs TypeScript Code (.ts)
  if (ext.compare(u"ts", Qt::CaseInsensitive) == 0) {
    QFile f(path);
    if (f.open(QIODevice::ReadOnly)) {
      char header[1];
      if (f.read(header, 1) == 1 && static_cast<unsigned char>(header[0]) == 0x47) {
        return Video; // Valid MPEG Transport Stream sync byte (0x47)
      }
    }
    return Unknown; // TypeScript source code or text file
  }

  // Raw
  if (ext.compare(u"arw", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"cr2", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"cr3", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"dng", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"nef", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"nrw", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"orf", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"rw2", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"pef", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"raf", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"sr2", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"srf", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"kdc", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"dcr", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"raw", Qt::CaseInsensitive) == 0) {
    return Raw;
  }

  // Image
  if (ext.compare(u"jpg", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"jpeg", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"png", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"webp", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"heic", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"heif", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"tiff", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"tif", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"bmp", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"gif", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"ico", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"tga", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"avif", Qt::CaseInsensitive) == 0 ||
      ext.compare(u"jfif", Qt::CaseInsensitive) == 0) {
    return Image;
  }

  return Unknown;
}

#include <QImageReader>
#include <QImageWriter>
#include <QTemporaryFile>


QVariantMap DesktopHelper::getImageDimensions(const QString &path) {
  QVariantMap result;
  result["width"] = 0;
  result["height"] = 0;
  result["dpi"] = 72;
  QString clean = path;
  if (clean.startsWith("file:///")) clean = clean.mid(8);
  else if (clean.startsWith("file://")) clean = clean.mid(7);
  else if (clean.startsWith("file:")) clean = clean.mid(5);
  clean = QDir::fromNativeSeparators(clean);

  QImageReader reader(clean);
  QSize sz = reader.size();
  if (sz.isValid()) {
    result["width"] = sz.width();
    result["height"] = sz.height();
  }
  QImage img = reader.read();
  if (!img.isNull()) {
    int dpmX = img.dotsPerMeterX();
    if (dpmX > 0) {
      result["dpi"] = qRound(dpmX / 39.3701);
    }
  }
  return result;
}

qint64 DesktopHelper::getTotalSize(const QStringList &paths) {
  qint64 total = 0;
  for (const QString &p : paths) {
    total += getFileSize(p);
  }
  return total;
}

QVariantMap DesktopHelper::generateResizePreview(const QString &sourcePath,
                                                 int width, int height,
                                                 int quality, int compression,
                                                 int dpi) {
  QVariantMap result;
  result["path"] = "";
  result["size"] = 0;
  result["origW"] = 0;
  result["origH"] = 0;
  result["newW"] = 0;
  result["newH"] = 0;
  result["origDpi"] = 72;

  QString cleanPath = sourcePath;
  if (cleanPath.startsWith("file:///")) cleanPath = cleanPath.mid(8);
  else if (cleanPath.startsWith("file://")) cleanPath = cleanPath.mid(7);
  else if (cleanPath.startsWith("file:")) cleanPath = cleanPath.mid(5);
  cleanPath = QDir::fromNativeSeparators(cleanPath);

  QImageReader reader(cleanPath);
  reader.setAutoTransform(true);
  if (!reader.canRead())
    return result;

  QSize originalSize = reader.size();
  result["origW"] = originalSize.width();
  result["origH"] = originalSize.height();

  QSize targetSize = originalSize;
  if (width > 0 && height > 0) {
    targetSize.scale(width, height, Qt::KeepAspectRatio);
    reader.setScaledSize(targetSize);
  }
  result["newW"] = targetSize.width();
  result["newH"] = targetSize.height();

  QImage img = reader.read();
  if (img.isNull())
    return result;

  int dpmX = img.dotsPerMeterX();
  if (dpmX > 0) {
    result["origDpi"] = qRound(dpmX / 39.3701);
  }

  QSettings settings("SamsungClone", "VirtualRotations");
  int virtualRot = settings.value(cleanPath, 0).toInt();
  if (virtualRot != 0) {
    QTransform t;
    t.rotate(virtualRot);
    img = img.transformed(t, Qt::SmoothTransformation);
  }

  if (dpi > 0) {
    int dpm = qRound(dpi * 39.3701);
    img.setDotsPerMeterX(dpm);
    img.setDotsPerMeterY(dpm);
  }

  QString tempPath = QDir::tempPath() + "/preview_resize_" +
                     QString::number(qHash(cleanPath)) +
                     (quality == 101 ? ".png" : ".jpg");
  QImageWriter writer(tempPath);
  if (quality == 101) {
    writer.setFormat("png");
    writer.setQuality(100);
  } else {
    writer.setFormat("jpg");
    writer.setQuality(std::clamp(quality, 1, 100));
  }
  if (compression >= 0) {
    writer.setCompression(compression);
  }

  if (writer.write(img)) {
    result["path"] = "file:///" + QDir::fromNativeSeparators(tempPath);
    result["size"] = QFileInfo(tempPath).size();
  }
  return result;
}

qint64 DesktopHelper::estimateBatchSize(const QStringList &paths, int width, int height,
                                         int quality, int compression,
                                         qint64 previewSingleBytes,
                                         int previewOrigW, int previewOrigH) {
  if (paths.isEmpty()) return 0;
  
  double bpp = 0.09; // Default ~0.72 bits/pixel for standard JPEG Q=75-80
  if (previewSingleBytes > 0 && previewOrigW > 0 && previewOrigH > 0) {
    QSize scaledPrev(previewOrigW, previewOrigH);
    if (width > 0 && height > 0) scaledPrev.scale(width, height, Qt::KeepAspectRatio);
    double prevPixels = std::max(1, scaledPrev.width() * scaledPrev.height());
    bpp = (double)previewSingleBytes / prevPixels;
  } else {
    if (quality == 101) bpp = 0.65; // Lossless PNG
    else bpp = std::max(0.03, (quality / 100.0) * 0.14);
  }

  qint64 totalEst = 0;
  for (const QString &rawPath : paths) {
    QString p = rawPath;
    if (p.startsWith("file:///")) p = p.mid(8);
    else if (p.startsWith("file://")) p = p.mid(7);
    else if (p.startsWith("file:")) p = p.mid(5);
    p = QDir::fromNativeSeparators(p);

    QImageReader reader(p);
    QSize origSz = reader.size();
    if (origSz.isValid() && origSz.width() > 0 && origSz.height() > 0) {
      QSize targetSz = origSz;
      if (width > 0 && height > 0) targetSz.scale(width, height, Qt::KeepAspectRatio);
      qint64 imgBytes = std::max((qint64)10240, (qint64)std::round(targetSz.width() * targetSz.height() * bpp));
      totalEst += imgBytes;
    } else {
      totalEst += std::max((qint64)15000, (qint64)(getFileSize(p) * 0.3));
    }
  }
  return totalEst;
}

int DesktopHelper::exportImages(const QStringList &paths,
                                const QString &destinationDir, int width,
                                int height, int quality, int compression,
                                const QString &suffix, int dpi) {
  QDir dir(destinationDir);
  if (!dir.exists())
    dir.mkpath(".");

  int count = 0;
  for (const QString &rawPath : paths) {
    QString path = rawPath;
    if (path.startsWith("file:///")) path = path.mid(8);
    else if (path.startsWith("file://")) path = path.mid(7);
    else if (path.startsWith("file:")) path = path.mid(5);
    path = QDir::fromNativeSeparators(path);

    QFileInfo fi(path);
    QString baseName = fi.baseName();
    // When compressing/resizing, output real JPEG (.jpg) unless lossless PNG is explicitly chosen
    QString ext = (quality == 101) ? "png" : "jpg";

    QString fileName = baseName + (suffix.isEmpty() ? "" : suffix) + "." + ext;
    QString destPath = dir.absoluteFilePath(fileName);

    QImageReader reader(path);
    reader.setAutoTransform(true);
    if (reader.canRead()) {
      QSize originalSize = reader.size();
      QSize targetSize = originalSize;
      if (width > 0 && height > 0) {
        targetSize.scale(width, height, Qt::KeepAspectRatio);
        reader.setScaledSize(targetSize);
      }

      QImage img = reader.read();
      if (!img.isNull()) {
        QSettings settings("SamsungClone", "VirtualRotations");
        int virtualRot = settings.value(path, 0).toInt();
        if (virtualRot != 0) {
          QTransform t;
          t.rotate(virtualRot);
          img = img.transformed(t, Qt::SmoothTransformation);
        }

        if (dpi > 0) {
          int dpm = qRound(dpi * 39.3701);
          img.setDotsPerMeterX(dpm);
          img.setDotsPerMeterY(dpm);
        }

        QImageWriter writer(destPath);
        if (quality == 101) {
          writer.setFormat("png");
          writer.setQuality(100);
        } else {
          writer.setFormat("jpg");
          writer.setQuality(std::clamp(quality, 1, 100));
        }
        if (compression >= 0)
          writer.setCompression(compression);
        if (writer.write(img)) {
          count++;
        }
      }
    }
  }
  return count;
}

void DesktopHelper::copyFiles(const QStringList &paths,
                              const QString &destinationDir) {
  QDir dir(destinationDir);
  if (!dir.exists())
    dir.mkpath(".");

  for (const QString &path : paths) {
    QFileInfo fi(path);
    QString destPath = dir.absoluteFilePath(fi.fileName());
    if (QFile::exists(destPath)) {
      QFile::remove(destPath);
    }
    QFile::copy(path, destPath);
  }
}

bool DesktopHelper::isNetworkPath(const QString &path) {
  return staticIsNetworkPath(path);
}

bool DesktopHelper::isDirectory(const QString &path) {
  if (path.isEmpty())
    return false;
  return QFileInfo(QDir::fromNativeSeparators(path)).isDir();
}

qint64 DesktopHelper::getFileSize(const QString &path) {
  if (path.isEmpty())
    return 0;
  QString clean = path;
  if (clean.startsWith("file:///")) clean = clean.mid(8);
  else if (clean.startsWith("file://")) clean = clean.mid(7);
  else if (clean.startsWith("file:")) clean = clean.mid(5);
  clean = QDir::fromNativeSeparators(clean);
  return QFileInfo(clean).size();
}

bool DesktopHelper::staticIsNetworkPath(const QString &path) {
  if (path.isEmpty())
    return false;

  QString cleanPath = QDir::fromNativeSeparators(path);

  // UNC network paths: //server/share or \\server\share
  if (cleanPath.startsWith("//") || cleanPath.startsWith("\\\\")) {
    return true;
  }

  // Drive letter checks (e.g. "I:/..." or "I:\...")
  if (cleanPath.length() >= 2 && cleanPath[1] == ':') {
    QString root = cleanPath.left(3);
    if (!root.endsWith('/')) {
      root += '/';
    }

#ifdef Q_OS_WIN
    QChar driveLetter = cleanPath[0].toUpper();
    if (driveLetter >= 'A' && driveLetter <= 'Z') {
      static UINT s_driveTypeCache[26] = {0};
      int idx = driveLetter.unicode() - 'A';
      UINT driveType = s_driveTypeCache[idx];
      if (driveType == 0) {
        driveType = GetDriveTypeW((const wchar_t *)root.utf16());
        s_driveTypeCache[idx] = driveType;
      }
      return (driveType == DRIVE_REMOTE);
    }
#else
    QStorageInfo storage(root);
    if (storage.isValid()) {
      QByteArray fsType = storage.fileSystemType().toLower();
      if (fsType == "nfs" || fsType == "cifs" || fsType == "smb3" ||
          fsType == "smbfs" || fsType == "afpfs" || fsType == "webdav") {
        return true;
      }
    }
#endif
  }

#ifndef Q_OS_WIN
  QStorageInfo storage(cleanPath);
  if (storage.isValid()) {
    QByteArray fsType = storage.fileSystemType().toLower();
    if (fsType == "nfs" || fsType == "cifs" || fsType == "smb3" ||
        fsType == "smbfs" || fsType == "afpfs" || fsType == "webdav") {
      return true;
    }
  }
#endif

  return false;
}

QString DesktopHelper::urlToLocalFile(const QString &urlString) {
  QUrl url(urlString);
  if (url.isLocalFile()) {
    return url.toLocalFile();
  }
  return urlString;
}

QStringList DesktopHelper::getAdjacentFiles(const QString &filePath,
                                            int neighborWindow) {
  QStringList result;
  if (filePath.isEmpty())
    return result;

  QString cleanPath = filePath;
  if (cleanPath.startsWith("file:///"))
    cleanPath = cleanPath.mid(8);
  else if (cleanPath.startsWith("file://"))
    cleanPath = cleanPath.mid(7);
  else if (cleanPath.startsWith("file:"))
    cleanPath = cleanPath.mid(5);
  cleanPath = QDir::fromNativeSeparators(cleanPath);

  QFileInfo fileInfo(cleanPath);
  QDir dir = fileInfo.dir();
  if (!dir.exists()) {
    result.append(cleanPath);
    return result;
  }

  const QStringList &nameFilters = supportedNameFilters();

  QStringList files = dir.entryList(nameFilters, QDir::Files | QDir::NoSymLinks,
                                    QDir::Name | QDir::IgnoreCase);
  if (files.isEmpty()) {
    result.append(cleanPath);
    return result;
  }

  QString targetName = fileInfo.fileName();
  int targetIdx = -1;
  for (int i = 0; i < files.size(); ++i) {
    if (files[i].compare(targetName, Qt::CaseInsensitive) == 0) {
      targetIdx = i;
      break;
    }
  }

  if (targetIdx == -1) {
    result.append(cleanPath);
    return result;
  }

  int startIdx = qMax(0, targetIdx - neighborWindow);
  int endIdx = qMin(files.size() - 1, targetIdx + neighborWindow);

  for (int i = startIdx; i <= endIdx; ++i) {
    result.append(dir.absoluteFilePath(files[i]));
  }
  return result;
}

QVariantList DesktopHelper::getMountedDrives() {
  QVariantList drives;
  const auto volumes = QStorageInfo::mountedVolumes();
  for (const QStorageInfo &storage : volumes) {
    if (!storage.isValid() || !storage.isReady())
      continue;

    QString rootPath = QDir::toNativeSeparators(storage.rootPath());
    if (rootPath.endsWith('\\') && rootPath.length() > 3) {
      rootPath.chop(1);
    }

    QVariantMap drive;
    drive["rootPath"] = rootPath;
    drive["name"] = storage.name().isEmpty() ? rootPath : storage.name();
    drive["displayName"] = QString("%1 (%2)").arg(storage.name().isEmpty() ? "Local Disk" : storage.name(), rootPath);
    drive["fileSystemType"] = QString::fromLatin1(storage.fileSystemType());
    drive["bytesTotal"] = storage.bytesTotal();
    drive["bytesAvailable"] = storage.bytesAvailable();
    drive["bytesFree"] = storage.bytesFree();
    drive["isReadOnly"] = storage.isReadOnly();

    QString type = "FIXED";
#ifdef Q_OS_WIN
    if (rootPath.length() >= 2 && rootPath[1] == ':') {
      QString rootWin = rootPath.left(2) + "\\";
      UINT dType = GetDriveTypeW((const wchar_t *)rootWin.utf16());
      switch (dType) {
        case DRIVE_REMOVABLE: type = "REMOVABLE"; break; // USB / SD Card
        case DRIVE_FIXED:     type = "FIXED"; break;     // Internal NVMe / SATA
        case DRIVE_REMOTE:    type = "REMOTE"; break;    // Network share / SMB
        case DRIVE_CDROM:     type = "CDROM"; break;
        case DRIVE_RAMDISK:   type = "RAMDISK"; break;
        default:              type = "UNKNOWN"; break;
      }
    } else if (rootPath.startsWith("\\\\")) {
      type = "REMOTE";
    }
#endif
    drive["driveType"] = type;
    drives.append(drive);
  }
  return drives;
}

bool DesktopHelper::isRunningAsAdmin() const {
#ifdef Q_OS_WIN
  BOOL isAdmin = FALSE;
  PSID adminGroup = NULL;
  SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
  if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                               DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                               &adminGroup)) {
    CheckTokenMembership(NULL, adminGroup, &isAdmin);
    FreeSid(adminGroup);
  }
  return isAdmin != FALSE;
#else
  return false;
#endif
}

bool DesktopHelper::relaunchAsAdmin(const QString &folderToOpen) {
#ifdef Q_OS_WIN
  QString appExe = QCoreApplication::applicationFilePath();
  QString nativeAppExe = QDir::toNativeSeparators(appExe);
  QString argString;
  
  // Resolve standard user paths BEFORE elevation handoff
  QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
  QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
  
  argString = QString("--cache-dir \"%1\" --temp-dir \"%2\"")
                  .arg(QDir::toNativeSeparators(appDataPath))
                  .arg(QDir::toNativeSeparators(tempPath));

  if (!folderToOpen.isEmpty()) {
    argString += " \"" + QDir::toNativeSeparators(folderToOpen) + "\"";
  }

  HINSTANCE result = ShellExecuteW(
      NULL,
      L"runas",
      (LPCWSTR)nativeAppExe.utf16(),
      (LPCWSTR)argString.utf16(),
      NULL,
      SW_SHOWNORMAL);

  if ((INT_PTR)result > 32) {
    QCoreApplication::quit();
    TerminateProcess(GetCurrentProcess(), 0);
    return true;
  }
  return false;
#else
  Q_UNUSED(folderToOpen);
  return false;
#endif
}

bool DesktopHelper::relaunchAsStandardUser(const QString &folderToOpen) {
#ifdef Q_OS_WIN
  QString appExe = QCoreApplication::applicationFilePath();
  QString nativeAppExe = QDir::toNativeSeparators(appExe);
  QString argString = "\"" + nativeAppExe + "\"";
  if (!folderToOpen.isEmpty()) {
    argString += " \"" + QDir::toNativeSeparators(folderToOpen) + "\"";
  }

  // Spawning via explorer.exe forces Windows to drop High Integrity (Admin) back to Medium Integrity (Standard User)
  HINSTANCE result = ShellExecuteW(
      NULL,
      L"open",
      L"explorer.exe",
      (LPCWSTR)argString.utf16(),
      NULL,
      SW_SHOWNORMAL);

  if ((INT_PTR)result > 32) {
    QCoreApplication::quit();
    TerminateProcess(GetCurrentProcess(), 0);
    return true;
  }
  return false;
#else
  Q_UNUSED(folderToOpen);
  return false;
#endif
}

QQmlEngine *DesktopHelper::s_engine = nullptr;

void DesktopHelper::setEngine(QQmlEngine *engine) {
  s_engine = engine;
}

void DesktopHelper::openNewWindow(const QString &folderPath) {
  if (!s_engine) {
    qWarning() << "[DesktopHelper] Cannot open new window: QQmlEngine not set!";
    return;
  }

  QUrl qmlUrl;
  if (QFile::exists(":/ScrollBench/qml/Main.qml")) {
    qmlUrl = QUrl("qrc:/ScrollBench/qml/Main.qml");
  } else {
    qmlUrl = QUrl("qrc:/QGalleryX/resources/qml_legacy/Main.qml");
  }

  QQmlComponent component(s_engine, qmlUrl);
  if (component.isReady()) {
    QObject *windowObj = component.create(s_engine->rootContext());
    if (windowObj) {
      QQmlEngine::setObjectOwnership(windowObj, QQmlEngine::CppOwnership);
      QWindow *win = qobject_cast<QWindow*>(windowObj);
      if (win) {
        // Stagger window position slightly so it doesn't overlap completely
        static int windowOffset = 0;
        windowOffset = (windowOffset + 35) % 200;
        win->setX(win->x() + windowOffset);
        win->setY(win->y() + windowOffset);

        // Safely clean up all resources and child models when closed
        connect(win, &QWindow::visibleChanged, win, [win](bool visible) {
          if (!visible) {
            win->deleteLater();
          }
        });
      }
      if (!folderPath.isEmpty()) {
        windowObj->setProperty("currentPath", folderPath);
      }
      qDebug() << "[DesktopHelper] Spawned new top-level gallery window for path:" << folderPath;
    }
  } else {
    qWarning() << "[DesktopHelper] Failed to create new window component:" << component.errorString();
  }
}

void DesktopHelper::setFormatEngineOverride(const QString &extension, int engine) {
  staticSetFormatEngineOverride(extension, engine);
}

int DesktopHelper::getFormatEngineOverride(const QString &extension) const {
  return staticGetFormatEngineOverride(extension);
}

QVariantMap DesktopHelper::getAllFormatOverrides() const {
  QSettings settings("SamsungClone", "FormatOverrides");
  QVariantMap map;
  for (const QString &key : settings.allKeys()) {
    map[key] = settings.value(key).toInt();
  }
  return map;
}

int DesktopHelper::staticGetFormatEngineOverride(const QString &pathOrExt) {
  QString ext = pathOrExt;
  int dot = ext.lastIndexOf('.');
  if (dot >= 0) ext = ext.mid(dot + 1);
  ext = ext.trimmed().toLower();
  if (ext.isEmpty()) return 0;
  QSettings settings("SamsungClone", "FormatOverrides");
  return settings.value(ext, 0).toInt();
}

void DesktopHelper::staticSetFormatEngineOverride(const QString &pathOrExt, int engine) {
  QString ext = pathOrExt;
  int dot = ext.lastIndexOf('.');
  if (dot >= 0) ext = ext.mid(dot + 1);
  ext = ext.trimmed().toLower();
  if (ext.isEmpty()) return;
  QSettings settings("SamsungClone", "FormatOverrides");
  if (engine <= 0) {
    settings.remove(ext);
    qDebug() << "[FormatOverrides] Cleared override for extension:" << ext;
  } else {
    settings.setValue(ext, engine);
    qDebug() << "[FormatOverrides] Set override for extension:" << ext << "-> Engine:" << engine;
  }
}



