#include "VideoThumbnailer.h"
#include <QDebug>
#include <QFileInfo>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

// Static callback for FFmpeg logging
static void ffmpegLogCallback(void *ptr, int level, const char *fmt,
                              va_list vl) {
  if (level > av_log_get_level())
    return;

  char buffer[1024];
  vsnprintf(buffer, sizeof(buffer), fmt, vl);
  QString message = QString::fromLocal8Bit(buffer).trimmed();

  if (message.isEmpty())
    return;

  switch (level) {
  case AV_LOG_PANIC:
  case AV_LOG_FATAL:
    qCritical() << "[FFmpeg Fatal]" << message;
    break;
  case AV_LOG_ERROR:
    qCritical() << "[FFmpeg Error]" << message;
    break;
  case AV_LOG_WARNING:
    qWarning() << "[FFmpeg Warn]" << message;
    break;
  case AV_LOG_INFO:
    qInfo() << "[FFmpeg Info]" << message;
    break;
  case AV_LOG_VERBOSE:
  case AV_LOG_DEBUG:
    qDebug() << "[FFmpeg Debug]" << message;
    break;
  default:
    qDebug() << "[FFmpeg]" << message;
    break;
  }
}

// Hardware Device Context (Static to reuse across threads/instances)
static AVBufferRef *s_hwDeviceCtx = nullptr;
static enum AVPixelFormat s_hwPixFmt = AV_PIX_FMT_NONE;

static enum AVPixelFormat get_hw_format(AVCodecContext *ctx,
                                        const enum AVPixelFormat *pix_fmts) {
  const enum AVPixelFormat *p;
  for (p = pix_fmts; *p != -1; p++) {
    if (*p == s_hwPixFmt) {
      return *p;
    }
  }
  qWarning() << "[FFmpeg] Failed to get HW surface format.";
  return AV_PIX_FMT_NONE;
}

// Circuit Breaker: If we fail to initialize/use HW multiple times, stop trying.
static std::atomic<int> s_consecutiveHwFailures(0);
static const int MAX_HW_FAILURES = 3;

#include <mutex>

static std::mutex s_hwMutex;

static void initHardware() {
  // Circuit Breaker Check
  if (s_consecutiveHwFailures > MAX_HW_FAILURES) {
    return;
  }

  std::lock_guard<std::mutex> lock(s_hwMutex);
  if (s_hwDeviceCtx)
    return;

  // Initialize D3D11VA
  // We try to find D3D11
  int type = av_hwdevice_find_type_by_name("d3d11va");
  if (type == AV_HWDEVICE_TYPE_NONE) {
    qWarning() << "[FFmpeg] D3D11VA not found.";
    return;
  }

  // Loop to find a working device (or just default)
  // av_hwdevice_ctx_create creates the context.
  int err = av_hwdevice_ctx_create(&s_hwDeviceCtx, (enum AVHWDeviceType)type,
                                   nullptr, nullptr, 0);
  if (err < 0) {
    qWarning() << "[FFmpeg] Failed to create D3D11VA device context.";
    return;
  }
  s_hwPixFmt = AV_PIX_FMT_D3D11;
  qInfo() << "[FFmpeg] Initialized D3D11VA Hardware Acceleration.";
}

VideoThumbnailer::VideoThumbnailer() {
  // FFmpeg initialization is mostly automatic in newer versions (>= 4.0)
  // Connect logs to Qt system
  av_log_set_callback(ffmpegLogCallback);
  av_log_set_level(AV_LOG_WARNING); // Restore granularity (Warning level)

  // Try to Init HW
  initHardware();
}

VideoThumbnailer::~VideoThumbnailer() {}

QImage VideoThumbnailer::extractFrame(const QString &path, int timeMs,
                                      const QSize &targetSize, bool allowHW) {
  QImage resultImage;
  struct FFmpegCleanup {
    uint8_t *buffer = nullptr;
    AVFormatContext *fmtCtx = nullptr;
    AVCodecContext *codecCtx = nullptr;
    AVFrame *frame = nullptr;
    AVFrame *swFrame = nullptr; // For HW download
    AVFrame *rgbFrame = nullptr;
    SwsContext *swsCtx = nullptr;
    AVPacket *packet = nullptr;

    ~FFmpegCleanup() {
      if (swsCtx)
        sws_freeContext(swsCtx);
      if (swFrame)
        av_frame_free(&swFrame);
      if (rgbFrame)
        av_frame_free(&rgbFrame);
      if (frame)
        av_frame_free(&frame);
      if (packet)
        av_packet_free(&packet);
      if (codecCtx)
        avcodec_free_context(&codecCtx);
      if (fmtCtx)
        avformat_close_input(&fmtCtx);
      if (buffer)
        av_free(buffer);
    }
  } cleanup;

  // FFmpeg requires paths in UTF-8
  std::string pathStr = path.toStdString();

  // 1. Open Input File
  if (avformat_open_input(&cleanup.fmtCtx, pathStr.c_str(), nullptr, nullptr) !=
      0) {
    qWarning() << "[FFmpeg] Failed to open file:" << path;
    return QImage();
  }

  // 2. Find Stream Info
  if (avformat_find_stream_info(cleanup.fmtCtx, nullptr) < 0)
    return QImage();

  // 3. Find Best Video Stream
  int streamIdx = av_find_best_stream(cleanup.fmtCtx, AVMEDIA_TYPE_VIDEO, -1,
                                      -1, nullptr, 0);
  if (streamIdx < 0)
    return QImage();

  AVStream *stream = cleanup.fmtCtx->streams[streamIdx];
  const AVCodec *decoder = avcodec_find_decoder(stream->codecpar->codec_id);
  if (!decoder)
    return QImage();

  // 4. Decode Context
  cleanup.codecCtx = avcodec_alloc_context3(decoder);
  avcodec_parameters_to_context(cleanup.codecCtx, stream->codecpar);

  // Enable HW Acceleration if available AND allowed AND healthy
  if (allowHW && s_hwDeviceCtx && s_consecutiveHwFailures <= MAX_HW_FAILURES) {
    cleanup.codecCtx->hw_device_ctx = av_buffer_ref(s_hwDeviceCtx);
    cleanup.codecCtx->get_format = get_hw_format;
  }

  if (avcodec_open2(cleanup.codecCtx, decoder, nullptr) < 0)
    return QImage();

  // --- HARDENING: Dimension Limit (Prevent OOM) ---
  if (cleanup.codecCtx->width > 8192 || cleanup.codecCtx->height > 8192) {
    qWarning() << "[FFmpeg] Video dimensions too large (Risk of OOM):" << path;
    return QImage();
  }

  // Determine Output Size
  int dstW = cleanup.codecCtx->width;
  int dstH = cleanup.codecCtx->height;

  if (targetSize.isValid() && targetSize.width() > 0 &&
      targetSize.height() > 0) {
    // Keep Aspect Ratio
    double aspect =
        (double)cleanup.codecCtx->width / (double)cleanup.codecCtx->height;
    double targetAspect =
        (double)targetSize.width() / (double)targetSize.height();

    if (aspect > targetAspect) {
      // Limited by width
      dstW = targetSize.width();
      dstH = (int)(dstW / aspect);
    } else {
      // Limited by height
      dstH = targetSize.height();
      dstW = (int)(dstH * aspect);
    }
  }

  // Use a loop to retry if we get a black frame
  int retryCount = 0;

  while (retryCount < 3) {
    // 5. Seek to timestamp
    if (timeMs > 0) {
      int64_t timestamp = (int64_t)timeMs * 1000;
      timestamp = av_rescale_q(timestamp, {1, 1000000}, stream->time_base);
      // Seek backward to keyframe
      if (av_seek_frame(cleanup.fmtCtx, streamIdx, timestamp,
                        AVSEEK_FLAG_BACKWARD) < 0) {
        av_seek_frame(cleanup.fmtCtx, streamIdx, 0, AVSEEK_FLAG_BACKWARD);
      }
    }

    // 6. Decode Frame
    // Re-allocate frames if they were freed or null
    if (!cleanup.frame)
      cleanup.frame = av_frame_alloc();
    if (!cleanup.packet)
      cleanup.packet = av_packet_alloc();
    else
      av_packet_unref(cleanup.packet);

    avcodec_flush_buffers(cleanup.codecCtx); // Flush state for new seek

    bool frameFound = false;
    int maxPackets = 50;

    while (av_read_frame(cleanup.fmtCtx, cleanup.packet) >= 0 &&
           maxPackets > 0) {
      if (cleanup.packet->stream_index == streamIdx) {
        int ret = avcodec_send_packet(cleanup.codecCtx, cleanup.packet);
        if (ret >= 0) {
          ret = avcodec_receive_frame(cleanup.codecCtx, cleanup.frame);
          if (ret == 0) {
            frameFound = true;
            av_packet_unref(cleanup.packet);
            break;
          }
        }
      }
      av_packet_unref(cleanup.packet);
      maxPackets--;
    }

    if (frameFound) {
      AVFrame *finalFrame = cleanup.frame;

      // Check if we need to download from GPU
      if (cleanup.frame->format == s_hwPixFmt) {
        if (!cleanup.swFrame)
          cleanup.swFrame = av_frame_alloc();
        else
          av_frame_unref(cleanup.swFrame);

        if (av_hwframe_transfer_data(cleanup.swFrame, cleanup.frame, 0) < 0) {
          qWarning() << "[FFmpeg] Error transferring frame data from GPU";
          // Critical HW Failure: Reset Context and Increment HwFailures
          s_consecutiveHwFailures++;
          {
            std::lock_guard<std::mutex> lock(s_hwMutex);
            if (s_hwDeviceCtx) {
              av_buffer_unref(&s_hwDeviceCtx);
              s_hwDeviceCtx = nullptr;
            }
          }
          return QImage();
        }
        // Success: Reset failure count
        s_consecutiveHwFailures = 0;
        finalFrame = cleanup.swFrame;
      }

      // ... (Rest of conversion logic)

      // 7. Convert to RGB
      if (cleanup.swsCtx) {
        sws_freeContext(cleanup.swsCtx); // Free previous
        cleanup.swsCtx = nullptr;
      }

      // Check for valid dimensions before sws_getContext
      if (finalFrame->width <= 0 || finalFrame->height <= 0) {
        qWarning() << "[FFmpeg] Invalid frame dimensions:" << finalFrame->width
                   << "x" << finalFrame->height;
        return QImage();
      }

      cleanup.swsCtx = sws_getContext(finalFrame->width, finalFrame->height,
                                      (enum AVPixelFormat)finalFrame->format,
                                      dstW, dstH, AV_PIX_FMT_RGB24,
                                      SWS_BILINEAR, nullptr, nullptr, nullptr);

      if (!cleanup.swsCtx) {
        qWarning() << "[FFmpeg] Failed to create SwsContext";
        return QImage();
      }

      int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, dstW, dstH, 1);

      // We reuse rgbFrame, but need new buffer if size changed?
      // For simplicity, re-alloc buffer every time (it's small)
      if (cleanup.buffer)
        av_free(cleanup.buffer);
      cleanup.buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

      if (!cleanup.rgbFrame)
        cleanup.rgbFrame = av_frame_alloc();
      else
        av_frame_unref(cleanup.rgbFrame);

      av_image_fill_arrays(cleanup.rgbFrame->data, cleanup.rgbFrame->linesize,
                           cleanup.buffer, AV_PIX_FMT_RGB24, dstW, dstH, 1);

      sws_scale(cleanup.swsCtx, finalFrame->data, finalFrame->linesize, 0,
                finalFrame->height, cleanup.rgbFrame->data,
                cleanup.rgbFrame->linesize);

      // 8. Create QImage
      QImage tmp((const uchar *)cleanup.rgbFrame->data[0], dstW, dstH,
                 cleanup.rgbFrame->linesize[0], QImage::Format_RGB888);

      // DEEP COPY to detach from FFmpeg buffer
      resultImage = tmp.copy();

      // BLACK CHECK
      auto isMostlyBlack = [](const QImage &img) -> bool {
        if (img.isNull())
          return true;
        int w = img.width();
        int h = img.height();
        // Sample center area to be faster/more accurate? No, keep stride.
        uint64_t totalLuma = 0;
        int samples = 0;
        const int step = 10;
        // Protect against tiny images
        if (w < step || h < step)
          return false;

        for (int y = 0; y < h; y += step) {
          const uchar *scanline = img.constScanLine(y);
          for (int x = 0; x < w; x += step) {
            int r = scanline[x * 3];
            int g = scanline[x * 3 + 1];
            int b = scanline[x * 3 + 2];
            int luma = (r * 299 + g * 587 + b * 114) / 1000;
            totalLuma += luma;
            samples++;
          }
        }
        if (samples == 0)
          return false;
        return (totalLuma / samples) < 25;
      };

      if (!isMostlyBlack(resultImage)) {
        return resultImage;
      }

      qDebug() << "[VideoThumbnailer] Found black frame at" << timeMs
               << "ms. Retrying...";
    } else {
      qDebug() << "[VideoThumbnailer] Failed to decode frame at" << timeMs;
    }

    // Prepare for next attempt
    retryCount++;

    int64_t duration = cleanup.fmtCtx->duration;
    if (duration != AV_NOPTS_VALUE && duration > 0) {
      // Attempt 1: 10% mark
      // Attempt 2: 25% mark
      double pct = (retryCount == 1) ? 0.10 : 0.25;
      timeMs = (duration * pct) / 1000;
    } else {
      timeMs += 2000;
    }
  } // End Loop

  return resultImage;
}
