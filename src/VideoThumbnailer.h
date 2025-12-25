#ifndef VIDEOTHUMBNAILER_H
#define VIDEOTHUMBNAILER_H

#include "SettingsHelper.h"
#include <QImage>
#include <QString>


/**
 * @brief Handles video thumbnail extraction using FFmpeg libraries.
 *
 * Direct dependency on libavcodec, libavformat, libswscale.
 * Uses a dedicated class to encapsulate C-style memory management.
 */
class VideoThumbnailer {
public:
  VideoThumbnailer();
  ~VideoThumbnailer();

  /**
   * @brief Extracts a frame from the video at the specified timestamp.
   * @param path File path to the video.
   * @param timeMs Timestamp in milliseconds (default 5000ms = 5s).
   * @return QImage of the frame, or null QImage on failure.
   */
  QImage extractFrame(const QString &path, int timeMs = 0,
                      const QSize &targetSize = QSize(),
                      SettingsHelper::HWAccel accel = SettingsHelper::None,
                      std::atomic<bool> *cancelled = nullptr);

  /**
   * @brief Extracts metadata from the video file.
   */
  QVariantMap getMetadata(const QString &path);

private:
  void initFFmpeg();
};

#endif // VIDEOTHUMBNAILER_H
