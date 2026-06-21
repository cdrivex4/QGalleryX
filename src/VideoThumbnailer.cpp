#include "VideoThumbnailer.h"
#include "TaskScheduler.h"
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/buffer.h>
#include <libavutil/hwcontext.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}
#include "HardwareAccelerationManager.h"

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

  QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");

  switch (level) {
  case AV_LOG_PANIC:
  case AV_LOG_FATAL:
    qCritical() << "[" << timeStr << "][FFmpeg Fatal]" << message;
    break;
  case AV_LOG_ERROR:
    qCritical() << "[" << timeStr << "][FFmpeg Error]" << message;
    break;
  case AV_LOG_WARNING:
    qWarning() << "[" << timeStr << "][FFmpeg Warn]" << message;
    break;
  case AV_LOG_INFO:
    qInfo() << "[" << timeStr << "][FFmpeg Info]" << message;
    break;
  case AV_LOG_VERBOSE:
  case AV_LOG_DEBUG:
    qDebug() << "[" << timeStr << "][FFmpeg Debug]" << message;
    break;
  default:
    qDebug() << "[" << timeStr << "][FFmpeg]" << message;
    break;
  }
}

static enum AVPixelFormat get_hw_format(AVCodecContext *ctx,
                                        const enum AVPixelFormat *pix_fmts) {
  enum AVPixelFormat target =
      HardwareAccelerationManager::instance().pixelFormat();
  const enum AVPixelFormat *p;
  for (p = pix_fmts; *p != -1; p++) {
    if (*p == target) {
      return *p;
    }
  }
  return AV_PIX_FMT_NONE;
}

struct TimeoutData {
  QElapsedTimer timer;
  std::atomic<bool> *cancelled;
  int timeoutMs;
};

static int interrupt_cb(void *ctx) {
  if (!ctx)
    return 0;
  auto *data = (TimeoutData *)ctx;
  if (data->timer.elapsed() > data->timeoutMs)
    return 1;
  if (data->cancelled && data->cancelled->load())
    return 1;
  return 0;
}

static std::atomic<int> s_consecutiveHwFailures(0);
static const int MAX_HW_FAILURES = 3;

static void initHardware(SettingsHelper::HWAccel mode) {
  // Managed globally by HardwareAccelerationManager
}

VideoThumbnailer::VideoThumbnailer() {
  av_log_set_callback(ffmpegLogCallback);
  av_log_set_level(AV_LOG_WARNING);
}

VideoThumbnailer::~VideoThumbnailer() {}

QImage VideoThumbnailer::extractFrame(const QString &path, int timeMs,
                                      const QSize &targetSize,
                                      SettingsHelper::HWAccel accel,
                                      std::atomic<bool> *cancelled) {
  QString timeLogStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
  qDebug() << "[" << timeLogStr
           << "][VideoThumbnailer] Entering extractFrame for" << path;
  if (cancelled && cancelled->load())
    return QImage();

  QImage resultImage;
  struct FFmpegCleanup {
    uint8_t *buffer = nullptr;
    AVFormatContext *fmtCtx = nullptr;
    AVCodecContext *codecCtx = nullptr;
    AVFrame *frame = nullptr;
    AVFrame *swFrame = nullptr;
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

  std::string pathStr = path.toStdString();

  TimeoutData timeoutData = {QElapsedTimer(), cancelled, 5000}; // 5s timeout
  timeoutData.timer.start();

  cleanup.fmtCtx = avformat_alloc_context();
  cleanup.fmtCtx->interrupt_callback.callback = interrupt_cb;
  cleanup.fmtCtx->interrupt_callback.opaque = &timeoutData;

  if (avformat_open_input(&cleanup.fmtCtx, pathStr.c_str(), nullptr, nullptr) !=
      0) {
    qWarning() << "[" << timeLogStr
               << "][FFmpeg] Failed to open file (or timeout):" << path;
    return QImage();
  }

  if (cancelled && cancelled->load())
    return QImage();

  if (avformat_find_stream_info(cleanup.fmtCtx, nullptr) < 0)
    return QImage();

  int streamIdx = av_find_best_stream(cleanup.fmtCtx, AVMEDIA_TYPE_VIDEO, -1,
                                      -1, nullptr, 0);
  if (streamIdx < 0)
    return QImage();

  AVStream *stream = cleanup.fmtCtx->streams[streamIdx];
  const AVCodec *decoder = avcodec_find_decoder(stream->codecpar->codec_id);
  if (!decoder)
    return QImage();

  cleanup.codecCtx = avcodec_alloc_context3(decoder);
  avcodec_parameters_to_context(cleanup.codecCtx, stream->codecpar);
  
  // RESTRICT THREADS TO PREVENT CPU DEATH ON MULTIPLE CONCURRENT VIDEOS
  cleanup.codecCtx->thread_count = 1;

  if (accel != SettingsHelper::None &&
      s_consecutiveHwFailures <= MAX_HW_FAILURES) {
    AVBufferRef *hwCtx =
        HardwareAccelerationManager::instance().deviceContext();
    if (hwCtx) {
      cleanup.codecCtx->hw_device_ctx = hwCtx;
      cleanup.codecCtx->get_format = get_hw_format;

      QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
      qInfo() << "[" << timeStr << "][VideoThumbnailer] GPU decode enabled for"
              << path;
    }
  }

  if (avcodec_open2(cleanup.codecCtx, decoder, nullptr) < 0)
    return QImage();

  int dstW = cleanup.codecCtx->width;
  int dstH = cleanup.codecCtx->height;
  if (targetSize.isValid() && targetSize.width() > 0 && dstH > 0) {
    double aspect = (double)dstW / dstH;
    dstW = targetSize.width();
    dstH = (int)(dstW / aspect);
  }

  int retryCount = 0;
  while (retryCount < 1) { // Removed 3x retry on black frames to stop CPU overloading
    if (cancelled && cancelled->load())
      return QImage();

    // Smart seek to skip fade-ins and guarantee a representative frame instantly
    if (timeMs <= 0 && cleanup.fmtCtx->duration != AV_NOPTS_VALUE) {
      timeMs = (cleanup.fmtCtx->duration / 1000) * 0.15;
    }

    if (timeMs > 0) {
      int64_t timestamp = (int64_t)timeMs * 1000;
      timestamp = av_rescale_q(timestamp, {1, 1000000}, stream->time_base);
      av_seek_frame(cleanup.fmtCtx, streamIdx, timestamp, AVSEEK_FLAG_BACKWARD);
    }

    if (!cleanup.frame)
      cleanup.frame = av_frame_alloc();
    if (!cleanup.packet)
      cleanup.packet = av_packet_alloc();
    else
      av_packet_unref(cleanup.packet);

    avcodec_flush_buffers(cleanup.codecCtx);

    bool frameFound = false;
    int maxPackets = 150; // Massively reduced from 2000 to prevent 6s network stalls on bad files

    while (maxPackets > 0 &&
           av_read_frame(cleanup.fmtCtx, cleanup.packet) >= 0) {
      // Periodic cancellation/pause check
      if (maxPackets % 50 == 0) {
        if (cancelled && cancelled->load())
          return QImage();
        if (TaskScheduler::instance().isPaused())
          return QImage();
      }

      if (cleanup.packet->stream_index == streamIdx) {
        maxPackets--;
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
    }

    if (!frameFound) {
      qWarning() << "[" << timeLogStr
                 << "][VideoThumbnailer] Failed to find frame after 2000 video "
                    "packets at"
                 << timeMs << "ms for" << path;
    }

    if (frameFound) {
      AVFrame *finalFrame = cleanup.frame;
      enum AVPixelFormat hwFmt =
          HardwareAccelerationManager::instance().pixelFormat();
      if (hwFmt != AV_PIX_FMT_NONE && cleanup.frame->format == hwFmt) {
        // GPU frame detected - transfer from GPU to CPU memory
        static bool logged = false;
        if (!logged) {
          QString timeStr =
              QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
          qInfo() << "[" << timeStr
                  << "][VideoThumbnailer] GPU-decoded frame detected, "
                     "transferring to CPU memory";
          logged = true;
        }

        if (!cleanup.swFrame)
          cleanup.swFrame = av_frame_alloc();
        else
          av_frame_unref(cleanup.swFrame);
        if (av_hwframe_transfer_data(cleanup.swFrame, cleanup.frame, 0) < 0) {
          s_consecutiveHwFailures++;
          return QImage();
        }
        finalFrame = cleanup.swFrame;
      }

      if (cleanup.swsCtx) {
        sws_freeContext(cleanup.swsCtx);
        cleanup.swsCtx = nullptr;
      }

      cleanup.swsCtx = sws_getContext(finalFrame->width, finalFrame->height,
                                      (enum AVPixelFormat)finalFrame->format,
                                      dstW, dstH, AV_PIX_FMT_RGB24,
                                      SWS_BILINEAR, nullptr, nullptr, nullptr);
      if (!cleanup.swsCtx)
        return QImage();

      int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, dstW, dstH, 1);
      if (cleanup.buffer)
        av_free(cleanup.buffer);
      cleanup.buffer = (uint8_t *)av_malloc(numBytes);

      if (!cleanup.rgbFrame)
        cleanup.rgbFrame = av_frame_alloc();
      else
        av_frame_unref(cleanup.rgbFrame);

      av_image_fill_arrays(cleanup.rgbFrame->data, cleanup.rgbFrame->linesize,
                           cleanup.buffer, AV_PIX_FMT_RGB24, dstW, dstH, 1);
      sws_scale(cleanup.swsCtx, finalFrame->data, finalFrame->linesize, 0,
                finalFrame->height, cleanup.rgbFrame->data,
                cleanup.rgbFrame->linesize);

      QImage tmp((const uchar *)cleanup.rgbFrame->data[0], dstW, dstH,
                 cleanup.rgbFrame->linesize[0], QImage::Format_RGB888);
      return tmp.copy();
    }

    retryCount++;
    timeMs = (cleanup.fmtCtx->duration != AV_NOPTS_VALUE)
                 ? (cleanup.fmtCtx->duration * (0.1 * retryCount)) / 1000
                 : timeMs + 2000;
  }

  return resultImage;
}

QVariantMap VideoThumbnailer::getMetadata(const QString &path) {
  QVariantMap meta;
  AVFormatContext *fmtCtx = nullptr;
  std::string pathStr = path.toStdString();

  if (avformat_open_input(&fmtCtx, pathStr.c_str(), nullptr, nullptr) != 0) {
    return meta;
  }

  if (avformat_find_stream_info(fmtCtx, nullptr) >= 0) {
    if (fmtCtx->duration != AV_NOPTS_VALUE) {
      int64_t duration = fmtCtx->duration / AV_TIME_BASE;
      meta["Duration"] = QString("%1:%2:%3")
                             .arg(duration / 3600, 2, 10, QChar('0'))
                             .arg((duration % 3600) / 60, 2, 10, QChar('0'))
                             .arg(duration % 60, 2, 10, QChar('0'));
    }

    int streamIdx =
        av_find_best_stream(fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (streamIdx >= 0) {
      AVStream *stream = fmtCtx->streams[streamIdx];
      meta["Resolution"] = QString("%1x%2")
                               .arg(stream->codecpar->width)
                               .arg(stream->codecpar->height);
      meta["Codec"] = QString(avcodec_get_name(stream->codecpar->codec_id));
    }

    AVDictionaryEntry *tag = nullptr;
    tag = av_dict_get(fmtCtx->metadata, "creation_time", nullptr, 0);
    if (tag)
      meta["Created"] = QString::fromLocal8Bit(tag->value);
  }

  avformat_close_input(&fmtCtx);
  return meta;
}