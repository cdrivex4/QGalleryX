#ifndef FILETYPEROUTER_H
#define FILETYPEROUTER_H

#include <QSet>
#include <QString>

/**
 * @brief Central file type routing logic - Single Source of Truth
 *
 * This class determines which processing backend handles each file type:
 * - Qt Native (QImageReader): Fast, built-in support for common formats
 * - LibRaw: RAW camera file processing with full metadata extraction
 * - FFmpeg: Video and motion picture codecs
 */
class FileTypeRouter {
public:
  enum ProcessorType {
    QtNative,   // QImageReader - Built-in Qt support
    LibRaw,     // RAW camera files requiring LibRaw
    FFmpeg,     // Video files requiring FFmpeg
    Unsupported // Unknown/unsupported format
  };

  /**
   * @brief Determine which processor should handle this file
   * @param extension File extension (without dot, lowercase)
   * @return ProcessorType indicating the appropriate handler
   */
  static ProcessorType getProcessorForExtension(const QString &extension);

  /**
   * @brief Check if this is a video file
   */
  static bool isVideo(const QString &extension);

  /**
   * @brief Check if this is a RAW camera file
   */
  static bool isRaw(const QString &extension);

  /**
   * @brief Check if this is a standard image file (Qt native)
   */
  static bool isStandardImage(const QString &extension);

  /**
   * @brief Verify file type by inspecting content (magic bytes)
   * @param path Absolute file path
   * @param outIsImage Set to true if detected as image
   * @param outIsVideo Set to true if detected as video
   * @param outIsRaw Set to true if detected as RAW
   */
  static void verifyFileType(const QString &path, bool &outIsImage,
                             bool &outIsVideo, bool &outIsRaw);

private:
  // ===== Qt Native Image Formats =====
  // Supported by QImageReader (Qt 6.x with standard plugins)
  static const QSet<QString> s_qtNativeFormats;

  // ===== LibRaw RAW Camera Formats =====
  // Updated list from LibRaw 202502 snapshot + DNG 1.7 support
  static const QSet<QString> s_rawFormats;

  // ===== FFmpeg Video/Motion Picture Formats =====
  // Common video container formats supported by FFmpeg
  static const QSet<QString> s_videoFormats;
};

#endif // FILETYPEROUTER_H
