#ifndef HARDWAREACCELERATIONMANAGER_H
#define HARDWAREACCELERATIONMANAGER_H

#include "SettingsHelper.h"
#include <QMutex>
#include <QObject>

extern "C" {
#include <libavutil/buffer.h>
#include <libavutil/hwcontext.h>
#include <libavutil/pixfmt.h>
}

/**
 * @brief Manages hardware acceleration contexts (D3D11, Vulkan, OpenCL).
 * Provides a unified abstraction for video decoding and potentially image
 * processing.
 */
class HardwareAccelerationManager : public QObject {
  Q_OBJECT
public:
  static HardwareAccelerationManager &instance();

  /**
   * @brief Initializes or switches to the specified hardware acceleration mode.
   */
  void setMode(SettingsHelper::HWAccel mode);

  /**
   * @brief Gets the current hardware device context for FFmpeg.
   */
  AVBufferRef *deviceContext() const;

  /**
   * @brief Gets the pixel format associated with the current hardware mode.
   */
  AVPixelFormat pixelFormat() const;

  /**
   * @brief Gets the name of the current acceleration type.
   */
  Q_INVOKABLE QString currentModeName() const;

  /**
   * @brief Gets detailed information about the CPU instruction sets.
   */
  Q_INVOKABLE QString cpuInfo() const;

private:
  explicit HardwareAccelerationManager(QObject *parent = nullptr);
  ~HardwareAccelerationManager();

  void cleanup();
  bool tryInitialize(SettingsHelper::HWAccel mode, const QString &timeStr);

  SettingsHelper::HWAccel m_currentMode = SettingsHelper::None;
  AVBufferRef *m_deviceCtx = nullptr;
  AVPixelFormat m_pixFmt = AV_PIX_FMT_NONE;
  mutable QMutex m_mutex;
};

#endif // HARDWAREACCELERATIONMANAGER_H
