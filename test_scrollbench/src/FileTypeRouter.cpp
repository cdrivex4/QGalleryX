#include "FileTypeRouter.h"
#include <QByteArray>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>


// ===== Qt Native Image Formats =====
// QImageReader supported formats (Qt 6.x with standard plugins)
// Core formats: PNG, JPEG, BMP, GIF
// Extended formats (via Qt Image Formats module): HEIC, ICNS, JP2, TGA, TIFF,
// WBMP, WEBP
const QSet<QString> FileTypeRouter::s_qtNativeFormats = {
    // Core formats (always available)
    "png", "jpg", "jpeg", "jpe", "jfif", "bmp", "dib", "gif",

    // Extended formats (via Qt Image Formats add-on)
    "heic", "heif", // High Efficiency Image Format (iOS/macOS native)
    "icns",         // Apple Icon Image
    "jp2", "j2k", "jpf", "jpx", // JPEG 2000
    "tga", "targa",             // Truevision Targa
    "tif", "tiff",              // Tagged Image File Format
    "wbmp",                     // Wireless Bitmap
    "webp",                     // WebP (Google)
    "pbm", "pgm", "ppm", "pnm", // Portable Anymap formats
    "xbm", "xpm",               // X11 Bitmap/Pixmap
    "svg", "svgz"               // Scalable Vector Graphics (via Qt SVG)
};

// ===== LibRaw RAW Camera Formats =====
// Based on LibRaw 202502 snapshot + DNG 1.7 support
// Comprehensive list of RAW camera formats from major manufacturers
const QSet<QString> FileTypeRouter::s_rawFormats = {
    // Adobe
    "dng", // Digital Negative (universal RAW format)

    // Canon
    "crw", "cr2", "cr3", // Canon RAW formats
    "crn",               // Canon CRN (embedded RAW extraction)

    // Nikon
    "nef", "nrw", // Nikon Electronic Format

    // Sony
    "arw", "srf", "sr2", // Sony Alpha RAW

    // Olympus / OM System
    "orf", // Olympus RAW Format (12-bit and 14-bit high-res)

    // Fujifilm
    "raf", // Fuji RAW Format

    // Panasonic / Lumix
    "rw2", "raw", // Panasonic RAW (including encoding 8)

    // Pentax
    "pef", "ptx", // Pentax Electronic Format

    // Samsung
    "srw", // Samsung RAW Format

    // Hasselblad
    "3fr", "fff", // Hasselblad RAW

    // Leica
    "rwl", "dng", // Leica RAW (often uses DNG)

    // Phase One
    "iiq", "eip", // Phase One Intelligent Image Quality

    // Mamiya
    "mef", // Mamiya Electronic Format

    // Sigma
    "x3f", // Sigma X3F (via X3F Tools)

    // Kodak
    "dcr", "kdc", "k25", "dcs", // Kodak Digital Camera RAW

    // Minolta
    "mrw", // Minolta RAW

    // Epson
    "erf", // Epson RAW Format

    // Red Digital Cinema
    "r3d", // RedCode RAW (varies by camera)

    // Blackmagic
    "braw", // Blackmagic RAW

    // DJI (Drones via GoPro SDK)
    "dng", // DJI drones typically use DNG

    // GoPro
    "gpr", // GoPro RAW (via GoPro SDK)

    // Apple (iPhone/iPad ProRAW)
    "dng", // Apple ProRAW uses DNG container

    // Mobile/Smartphone DNGs
    "dng" // Various smartphones (Google Pixel, Huawei, etc.)
};

// ===== FFmpeg Video/Motion Picture Formats =====
// Common video container formats supported by FFmpeg
// Based on FFmpeg codec support (H.264, H.265, VP8, VP9, AV1, etc.)
const QSet<QString> FileTypeRouter::s_videoFormats = {
    // MPEG-4 family (most common)
    "mp4", "m4v", "m4a", // MPEG-4 Part 14
    "mov", "qt",         // QuickTime
    "3gp", "3g2",        // 3GPP mobile video
    "f4v", "f4a", "f4b", // Flash (Adobe)

    // Matroska family
    "mkv", "mk3d", // Matroska video
    "webm",        // WebM (VP8/VP9/AV1 in Matroska)

    // MPEG legacy
    "mpg", "mpeg", "mpe", "m2v", // MPEG-1/2 Program Stream
    "ts", "mts", "m2ts",         // MPEG Transport Stream
    "vob",                       // DVD Video Object

    // AVI family
    "avi",  // Audio Video Interleave
    "divx", // DivX

    // Windows Media
    "wmv", "asf", // Windows Media Video

    // RealMedia
    "rm", "rmvb", // RealMedia Variable Bitrate

    // Flash Video
    "flv", // Flash Video

    // Ogg family
    "ogv", "ogg", "ogm", // Ogg Video (Theora)

    // Other containers
    "mxf", // Material Exchange Format (broadcast)
    "gxf", // General eXchange Format
    "dv",  // DV (Digital Video)
    "yuv", // Raw YUV
    "y4m", // YUV4MPEG2

    // Streaming protocols
    "m3u8", // HLS (HTTP Live Streaming)
    "mpd",  // DASH (Dynamic Adaptive Streaming)

    // Professional/Cinema
    "r3d",  // RedCode RAW (also in RAW, but FFmpeg can handle)
    "braw", // Blackmagic RAW (partial support)
    "mxf",  // MXF (broadcast standard)
    "gxf"   // GXF (broadcast)
};

FileTypeRouter::ProcessorType
FileTypeRouter::getProcessorForExtension(const QString &extension) {
  QString ext = extension.toLower().trimmed();

  // Priority order:
  // 1. Check RAW first (most specialized)
  if (s_rawFormats.contains(ext)) {
    return LibRaw;
  }

  // 2. Check video (second most specialized)
  if (s_videoFormats.contains(ext)) {
    return FFmpeg;
  }

  // 3. Check Qt native (most common)
  if (s_qtNativeFormats.contains(ext)) {
    return QtNative;
  }

  // 4. Unknown format
  return Unsupported;
}

bool FileTypeRouter::isVideo(const QString &extension) {
  return s_videoFormats.contains(extension.toLower().trimmed());
}

bool FileTypeRouter::isRaw(const QString &extension) {
  return s_rawFormats.contains(extension.toLower().trimmed());
}

bool FileTypeRouter::isStandardImage(const QString &extension) {
  return s_qtNativeFormats.contains(extension.toLower().trimmed());
}

void FileTypeRouter::verifyFileType(const QString &path, bool &outIsImage,
                                    bool &outIsVideo, bool &outIsRaw) {
  outIsImage = false;
  outIsVideo = false;
  outIsRaw = false;

  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    return;
  }

  // Read header for magic bytes
  QByteArray header = file.read(32); // Read 32 bytes to be safe
  if (header.size() < 4) {
    return;
  }

  // --- Video Detection (Magic Bytes) ---
  // MP4/MOV/3GP: ftyp atom at offset 4
  if (header.size() >= 12 && header.mid(4, 4) == "ftyp") {
    outIsVideo = true;
    return;
  }
  // MKV/WebM: EBML ID 1A 45 DF A3
  if (header.startsWith("\x1A\x45\xDF\xA3")) {
    outIsVideo = true;
    return;
  }
  // AVI: RIFF .... AVI
  if (header.startsWith("RIFF") && header.size() >= 12 &&
      header.mid(8, 4) == "AVI ") {
    outIsVideo = true;
    return;
  }
  // MPEG-TS: Sync byte 0x47 (every 188 bytes, but checks first one)
  if (header[0] == 0x47) {
    // Weak check, assume video if extension also matches?
    // Let's rely on extension fallback for TS if this simple check isn't
    // enough. Or check if path ends with .ts
    QString ext = QFileInfo(path).suffix().toLower();
    if (isVideo(ext)) {
      outIsVideo = true;
      return;
    }
  }

  // --- Image/RAW Detection ---
  // Use QImageReader to check if Qt recognizes it (JPG, PNG, TIFF, etc.)
  file.seek(0);
  QImageReader reader(&file);
  reader.setDecideFormatFromContent(true);
  QByteArray fmt = reader.format();

  QString ext = QFileInfo(path).suffix().toLower(); // Fallback extension

  if (!fmt.isEmpty()) {
    QString fmtStr = QString::fromLatin1(fmt).toLower();

    // Check if detected format implies RAW (TIFF-based)
    if (fmtStr == "tiff" || fmtStr == "tif") {
      // TIFF structure is common for RAW (CR2, NEF, ARW, etc.)
      // Distinguish by extension
      if (isRaw(ext)) {
        outIsRaw = true;
      } else {
        outIsImage = true; // Standard TIFF
      }
    } else {
      // Known format (jpg, png, webp, etc.)
      outIsImage = true;

      // Some RAW formats might be detected as something else by plugins?
      if (s_rawFormats.contains(fmtStr) || isRaw(ext)) {
        outIsRaw = true; // Trust extension if QImageReader confirms it's a
                         // valid image file
      }
    }
  } else {
    // QImageReader failed. Could be unsupported RAW format (e.g. some CR3).
    // Start crude signature check for common RAWs that libraw supports but Qt
    // might not.

    // TIFF-like headers (II/MM) are used by CR2, NEF, ARW, ORF, PEF...
    bool isTiffHeader = (header.startsWith("\x49\x49\x2A\x00") ||
                         header.startsWith("\x4D\x4D\x00\x2A"));

    if (isTiffHeader) {
      if (isRaw(ext)) {
        outIsRaw = true;
      } else {
        // Maybe a TIFF that Qt failed to read? Treat as image.
        outIsImage = true;
      }
    } else {
      // Panasonic RW2 (II + 55 00) or similar?
      // RAF (Fuji) start with "FUJIFILM"
      if (header.startsWith("FUJIFILM")) {
        outIsRaw = true;
      }

      // If we still don't know, but extension claims RAW, we TRUST extension
      // ONLY IF file is large? No, just trust it if it has bytes.
      // But user wants "corruption check".
      // If header is garbage, maybe it's corrupted.
      // We will leave outIsImage/Raw as false.
    }
  }
}
