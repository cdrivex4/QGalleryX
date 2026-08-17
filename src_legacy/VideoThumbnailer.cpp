#include "VideoThumbnailer.h"
#include "TaskScheduler.h"
#include <QDateTime>
#include <QDebug>
#include <QPainter>
#include <QLinearGradient>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/buffer.h>
#include <libavutil/hwcontext.h>
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


VideoThumbnailer::VideoThumbnailer() {
  av_log_set_callback(ffmpegLogCallback);
  av_log_set_level(AV_LOG_ERROR);
}

void VideoThumbnailer::warmup() {
  av_log_set_callback(ffmpegLogCallback);
  av_log_set_level(AV_LOG_ERROR);
  const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_H264);
  Q_UNUSED(codec);
  qDebug() << "[VideoThumbnailer] Pre-loaded FFmpeg decoders and DLL pages into RAM.";
}

VideoThumbnailer::~VideoThumbnailer() {}

QImage VideoThumbnailer::extractFrame(const QString &path, int timeMs,
                                      const QSize &targetSize,
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

  TimeoutData timeoutData = {QElapsedTimer(), cancelled, 1000}; // 1s timeout
  timeoutData.timer.start();

  cleanup.fmtCtx = avformat_alloc_context();
  cleanup.fmtCtx->interrupt_callback.callback = interrupt_cb;
  cleanup.fmtCtx->interrupt_callback.opaque = &timeoutData;

  AVDictionary *options = nullptr;
  av_dict_set(&options, "probesize", "5000000", 0); // 5MB limit
  av_dict_set(&options, "analyzeduration", "2000000", 0); // 2s limit

  if (avformat_open_input(&cleanup.fmtCtx, pathStr.c_str(), nullptr, &options) !=
      0) {
    if (options) av_dict_free(&options);
    cleanup.fmtCtx = nullptr; // Prevent double-free in ~FFmpegCleanup()
    qWarning() << "[" << timeLogStr
               << "][FFmpeg] Failed to open file (or timeout):" << path;
    return QImage();
  }

  if (cancelled && cancelled->load())
    return QImage();

  if (options) av_dict_free(&options);

  if (avformat_find_stream_info(cleanup.fmtCtx, nullptr) < 0)
    return QImage();

  int streamIdx = av_find_best_stream(cleanup.fmtCtx, AVMEDIA_TYPE_VIDEO, -1,
                                      -1, nullptr, 0);
  if (streamIdx < 0) {
    // Check if it is an audio file (e.g. OGG, MP3, WAV, FLAC, M4A, AAC, OPUS)
    int audioStreamIdx = av_find_best_stream(cleanup.fmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audioStreamIdx >= 0) {
      int w = (targetSize.isValid() && targetSize.width() > 0) ? targetSize.width() : 256;
      int h = (targetSize.isValid() && targetSize.height() > 0) ? targetSize.height() : 256;
      QImage audioTile(w, h, QImage::Format_RGB32);
      audioTile.fill(QColor("#181a20"));
      QPainter p(&audioTile);
      p.setRenderHint(QPainter::Antialiasing);

      // Sleek gradient background
      QLinearGradient grad(0, 0, w, h);
      grad.setColorAt(0.0, QColor("#2a1b3d"));
      grad.setColorAt(1.0, QColor("#141824"));
      p.fillRect(audioTile.rect(), grad);

      // Music Note Emoji / Icon
      p.setPen(QColor("#c084fc"));
      p.setFont(QFont("Segoe UI Emoji", std::max(16, w / 4), QFont::Bold));
      p.drawText(QRect(0, h / 6, w, h / 2), Qt::AlignCenter, "🎵");

      // File Extension badge
      p.setPen(QColor("#e2e8f0"));
      p.setFont(QFont("Segoe UI", std::max(9, w / 14), QFont::Bold));
      int dot = path.lastIndexOf('.');
      QString ext = dot >= 0 ? path.mid(dot + 1).toUpper() : "AUDIO";
      p.drawText(QRect(0, (h * 5) / 8, w, h / 4), Qt::AlignCenter, ext);

      return audioTile;
    }
    return QImage();
  }

  AVStream *stream = cleanup.fmtCtx->streams[streamIdx];
  const AVCodec *decoder = avcodec_find_decoder(stream->codecpar->codec_id);
  if (!decoder)
    return QImage();

  cleanup.codecCtx = avcodec_alloc_context3(decoder);
  avcodec_parameters_to_context(cleanup.codecCtx, stream->codecpar);
  
  // RESTRICT THREADS TO PREVENT CPU DEATH ON MULTIPLE CONCURRENT VIDEOS
  // Hardware acceleration logic removed for legacy build

  if (avcodec_open2(cleanup.codecCtx, decoder, nullptr) < 0)
    return QImage();

  int dstW = cleanup.codecCtx->width;
  int dstH = cleanup.codecCtx->height;
  if (targetSize.isValid() && targetSize.width() > 0 && dstH > 0) {
    double aspect = (double)dstW / dstH;
    dstW = targetSize.width();
    dstH = (int)(dstW / aspect);
  }
  // Guarantee even dimensions for swscale to prevent heap boundary write overruns
  dstW = std::max(2, (dstW / 2) * 2);
  dstH = std::max(2, (dstH / 2) * 2);

  int retryCount = 0;
  while (retryCount < 3) { // 3x retry on black frames
    if (cancelled && cancelled->load())
      return QImage();

    bool isHeic = path.endsWith(".heic", Qt::CaseInsensitive) || path.endsWith(".heif", Qt::CaseInsensitive);
    if (!isHeic) {
      // Smart seek to skip fade-ins and guarantee a representative frame instantly
      if (timeMs <= 0 && cleanup.fmtCtx->duration != AV_NOPTS_VALUE) {
        timeMs = (cleanup.fmtCtx->duration / 1000) * 0.15;
      }

      if (timeMs > 0) {
        int64_t timestamp = (int64_t)timeMs * 1000;
        timestamp = av_rescale_q(timestamp, {1, 1000000}, stream->time_base);
        av_seek_frame(cleanup.fmtCtx, streamIdx, timestamp, AVSEEK_FLAG_BACKWARD);
      }
    }

    if (cleanup.frame)
      av_frame_unref(cleanup.frame);
    else
      cleanup.frame = av_frame_alloc();

    if (cleanup.packet)
      av_packet_unref(cleanup.packet);
    else
      cleanup.packet = av_packet_alloc();

    avcodec_flush_buffers(cleanup.codecCtx);

    bool frameFound = false;
    int maxPackets = 1500; // Increased to 1500 to prevent normal MP4s with lots of audio from failing

    while (maxPackets > 0 &&
           av_read_frame(cleanup.fmtCtx, cleanup.packet) >= 0) {
      maxPackets--;
      
      // Periodic cancellation/pause check
      if (maxPackets % 50 == 0) {
        if (cancelled && cancelled->load())
          return QImage();
      }

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
    }

    if (!frameFound) {
      qWarning() << "[" << timeLogStr
                 << "][VideoThumbnailer] Failed to find frame after 150 video packets at"
                 << timeMs << "ms for" << path;
      retryCount++;
      timeMs += std::max(2000, (int)((cleanup.fmtCtx->duration / 1000) * 0.1));
      continue;
    }

    if (frameFound) {
      AVFrame *finalFrame = cleanup.frame;
      enum AVPixelFormat hwFmt = AV_PIX_FMT_NONE;
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

      if (finalFrame->width <= 0 || finalFrame->height <= 0) {
        retryCount++;
        continue;
      }

      if (cleanup.swsCtx) {
        sws_freeContext(cleanup.swsCtx);
        cleanup.swsCtx = nullptr;
      }

      cleanup.swsCtx = sws_getContext(finalFrame->width, finalFrame->height,
                                      (enum AVPixelFormat)finalFrame->format,
                                      dstW, dstH, AV_PIX_FMT_BGRA,
                                      SWS_BILINEAR, nullptr, nullptr, nullptr);
      if (!cleanup.swsCtx)
        return QImage();

      uint8_t *dst_data[4] = {nullptr};
      int dst_linesize[4] = {0};
      int allocRes = av_image_alloc(dst_data, dst_linesize, dstW, dstH, AV_PIX_FMT_BGRA, 32);
      if (allocRes < 0)
        return QImage();

      sws_scale(cleanup.swsCtx, finalFrame->data, finalFrame->linesize, 0,
                finalFrame->height, dst_data, dst_linesize);

      QImage tmp(dst_data[0], dstW, dstH, dst_linesize[0], QImage::Format_RGB32);
      QImage frameCopy = tmp.copy();
      av_freep(&dst_data[0]);

      // Calculate average brightness to avoid pure black frames
      long long totalLuminance = 0;
      int step = std::max(1, dstH / 20); // sample evenly
      int samples = 0;
      for (int y = 0; y < dstH; y += step) {
          const QRgb* line = reinterpret_cast<const QRgb*>(frameCopy.constScanLine(y));
          for (int x = 0; x < dstW; x += step) {
              QRgb pixel = line[x];
              int r = qRed(pixel);
              int g = qGreen(pixel);
              int b = qBlue(pixel);
              totalLuminance += (r * 299 + g * 587 + b * 114) / 1000;
              samples++;
          }
      }
      int avgLuminance = samples > 0 ? (totalLuminance / samples) : 0;
      
      if (isHeic || avgLuminance > 15 || retryCount >= 2) {
          return frameCopy;
      }
      
      // Black frame detected, retry further into video
      qDebug() << "[VideoThumbnailer] Black frame detected (Luminance: " << avgLuminance << "). Retrying...";
      timeMs += std::max(2000, (int)((cleanup.fmtCtx->duration / 1000) * 0.1));
      retryCount++;
    }
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