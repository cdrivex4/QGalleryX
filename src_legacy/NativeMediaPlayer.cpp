#include "NativeMediaPlayer.h"
#include <QDebug>
#include <QDir>
#include <QUrl>
#include <QFileInfo>
#include <QAudioOutput>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QCoreApplication>
#include <algorithm>
#include <chrono>
#include <thread>
#include <deque>
#include <mutex>
#include <condition_variable>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
}

static enum AVPixelFormat get_player_hw_format(AVCodecContext *ctx,
                                               const enum AVPixelFormat *pix_fmts) {
  Q_UNUSED(ctx);
  const enum AVPixelFormat *p;
  // If D3D11 is available, try it
  for (p = pix_fmts; *p != -1; p++) {
    if (*p == AV_PIX_FMT_D3D11) {
      return *p;
    }
  }
  // Software Fallback: Seamless CPU decoding
  for (p = pix_fmts; *p != -1; p++) {
    if (*p != AV_PIX_FMT_D3D11 && *p != AV_PIX_FMT_DXVA2_VLD &&
        *p != AV_PIX_FMT_CUDA && *p != AV_PIX_FMT_VAAPI &&
        *p != AV_PIX_FMT_VIDEOTOOLBOX && *p != AV_PIX_FMT_QSV &&
        *p != AV_PIX_FMT_D3D12) {
      return *p;
    }
  }
  return pix_fmts[0];
}

struct QueuedVideoFrame {
  QVideoFrame frame;
  qint64 ptsMs = 0;
};

class NativeMediaPlayerWorker : public QObject {
  Q_OBJECT
public:
  explicit NativeMediaPlayerWorker(const QString &sourcePath, QObject *parent = nullptr)
      : QObject(parent), m_filePath(sourcePath) {}

  ~NativeMediaPlayerWorker() override {
    stopWorker();
  }

  void stopWorker() {
    m_running.store(false);
    m_paused.store(false);
    m_seekRequested.store(-1);
    m_queueCond.notify_all();
  }

  void requestPause(bool pause) {
    m_paused.store(pause);
    if (!pause) {
      m_queueCond.notify_all();
    }
  }

  void requestSeek(qint64 targetMs) {
    m_seekRequested.store(targetMs);
    m_queueCond.notify_all();
  }

  void setVolume(qreal vol, bool muted) {
    m_volume.store(muted ? 0.0f : (float)vol);
    if (m_audioSink) {
      m_audioSink->setVolume(muted ? 0.0f : (float)vol);
    }
  }

signals:
  void frameReady(const QVideoFrame &frame, qint64 ptsMs);
  void durationChanged(qint64 durationMs);
  void stateChanged(int state);
  void hasStreams(bool hasVid, bool hasAud);
  void finished();

public slots:
  void process() {
    emit stateChanged(NativeMediaPlayer::PlayingState);

    AVFormatContext *fmtCtx = nullptr;
    AVCodecContext *vCodecCtx = nullptr;
    AVCodecContext *aCodecCtx = nullptr;
    SwsContext *swsCtx = nullptr;
    SwrContext *swrCtx = nullptr;
    AVBufferRef *hwDevCtx = nullptr;

    std::string pathStr = m_filePath.toLocal8Bit().constData();
    AVDictionary *options = nullptr;
    av_dict_set(&options, "probesize", "5000000", 0);
    av_dict_set(&options, "analyzeduration", "2000000", 0);
    av_dict_set(&options, "flags", "low_delay", 0);

    if (avformat_open_input(&fmtCtx, pathStr.c_str(), nullptr, &options) != 0) {
      if (options) av_dict_free(&options);
      qWarning() << "[NativeMediaPlayer] Failed to open input:" << m_filePath;
      emit stateChanged(NativeMediaPlayer::StoppedState);
      emit finished();
      return;
    }
    if (options) av_dict_free(&options);

    if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
      qWarning() << "[NativeMediaPlayer] Failed to find stream info:" << m_filePath;
      avformat_close_input(&fmtCtx);
      emit stateChanged(NativeMediaPlayer::StoppedState);
      emit finished();
      return;
    }

    qint64 totalDurationMs = 0;
    if (fmtCtx->duration != AV_NOPTS_VALUE) {
      totalDurationMs = fmtCtx->duration / (AV_TIME_BASE / 1000);
    }
    emit durationChanged(totalDurationMs);

    int videoStreamIdx = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    int audioStreamIdx = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

    emit hasStreams(videoStreamIdx >= 0, audioStreamIdx >= 0);

    // 1. Initialize Video Codec
    if (videoStreamIdx >= 0) {
      AVStream *vStream = fmtCtx->streams[videoStreamIdx];
      const AVCodec *vDecoder = avcodec_find_decoder(vStream->codecpar->codec_id);
      if (vDecoder) {
        vCodecCtx = avcodec_alloc_context3(vDecoder);
        avcodec_parameters_to_context(vCodecCtx, vStream->codecpar);
        int idealThreads = std::clamp((int)QThread::idealThreadCount() / 2, 2, 8);
        vCodecCtx->thread_count = idealThreads;
        vCodecCtx->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;
        vCodecCtx->get_format = get_player_hw_format;

        // Try creating D3D11 HW context
        if (av_hwdevice_ctx_create(&hwDevCtx, AV_HWDEVICE_TYPE_D3D11VA, nullptr, nullptr, 0) >= 0) {
          vCodecCtx->hw_device_ctx = av_buffer_ref(hwDevCtx);
          av_buffer_unref(&hwDevCtx);
        }

        if (avcodec_open2(vCodecCtx, vDecoder, nullptr) < 0) {
          avcodec_free_context(&vCodecCtx);
          vCodecCtx = nullptr;
        }
      }
    }

    // 2. Initialize Audio Codec & QAudioSink
    QIODevice *audioIO = nullptr;
    if (audioStreamIdx >= 0) {
      AVStream *aStream = fmtCtx->streams[audioStreamIdx];
      const AVCodec *aDecoder = avcodec_find_decoder(aStream->codecpar->codec_id);
      if (aDecoder) {
        aCodecCtx = avcodec_alloc_context3(aDecoder);
        avcodec_parameters_to_context(aCodecCtx, aStream->codecpar);
        if (avcodec_open2(aCodecCtx, aDecoder, nullptr) == 0) {
          QAudioFormat audioFmt;
          audioFmt.setSampleRate(48000);
          audioFmt.setChannelConfig(QAudioFormat::ChannelConfigStereo);
          audioFmt.setSampleFormat(QAudioFormat::Int16);

          QAudioDevice defaultDevice = QMediaDevices::defaultAudioOutput();
          m_audioSink = std::make_unique<QAudioSink>(defaultDevice, audioFmt);
          // Set 1-second audio buffer to guarantee zero buffer underruns
          m_audioSink->setBufferSize(48000 * 2 * sizeof(int16_t));
          m_audioSink->setVolume(m_volume.load());
          audioIO = m_audioSink->start();

          // Initialize resampler to 48kHz Stereo Int16
          AVChannelLayout outLayout;
          av_channel_layout_default(&outLayout, 2);
          swr_alloc_set_opts2(&swrCtx,
                              &outLayout, AV_SAMPLE_FMT_S16, 48000,
                              &aCodecCtx->ch_layout, aCodecCtx->sample_fmt, aCodecCtx->sample_rate,
                              0, nullptr);
          if (swrCtx) {
            swr_init(swrCtx);
          }
        } else {
          avcodec_free_context(&aCodecCtx);
          aCodecCtx = nullptr;
        }
      }
    }

    AVFrame *frame = av_frame_alloc();
    AVFrame *swFrame = av_frame_alloc();
    AVPacket *pkt = av_packet_alloc();

    QElapsedTimer systemWallClock;
    systemWallClock.start();
    qint64 startPtsMs = 0;
    qint64 lastVideoPtsMs = 0;
    qint64 audioFirstPtsMs = -1;

    std::deque<QueuedVideoFrame> videoQueue;
    const size_t MAX_VIDEO_QUEUE = 25; // 25 frames (~800ms of lookahead runway)
    bool eofReached = false;

    m_running.store(true);

    auto getMasterClock = [&]() -> qint64 {
      return startPtsMs + systemWallClock.elapsed();
    };

    while (m_running.load()) {
      // 1. Handle Seek Request
      qint64 seekTarget = m_seekRequested.exchange(-1);
      if (seekTarget >= 0) {
        int64_t targetTs = seekTarget * (AV_TIME_BASE / 1000);
        av_seek_frame(fmtCtx, -1, targetTs, AVSEEK_FLAG_BACKWARD);
        if (vCodecCtx) avcodec_flush_buffers(vCodecCtx);
        if (aCodecCtx) avcodec_flush_buffers(aCodecCtx);
        if (m_audioSink) m_audioSink->reset();
        if (m_audioSink && audioIO) audioIO = m_audioSink->start();
        videoQueue.clear();
        startPtsMs = -1; // Re-anchor clock to exact keyframe PTS upon first frame decode
        lastVideoPtsMs = seekTarget;
        audioFirstPtsMs = -1;
        eofReached = false;
        systemWallClock.restart();
      }

      // 2. Handle Pause
      while (m_paused.load() && m_running.load() && m_seekRequested.load() < 0) {
        if (m_audioSink && m_audioSink->state() == QAudio::ActiveState) {
          m_audioSink->suspend();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        systemWallClock.restart();
        startPtsMs = lastVideoPtsMs;
      }
      if (m_audioSink && m_audioSink->state() == QAudio::SuspendedState && !m_paused.load()) {
        m_audioSink->resume();
      }

      if (!m_running.load()) break;

      // 3. Demux Packets continuously
      while (!eofReached && videoQueue.size() < MAX_VIDEO_QUEUE && m_running.load()) {
        int ret = av_read_frame(fmtCtx, pkt);
        if (ret < 0) {
          eofReached = true;
          break;
        }

        // Process Audio Packet: Decode and buffer straight to QAudioSink
        if (pkt->stream_index == audioStreamIdx && aCodecCtx && audioIO) {
          if (avcodec_send_packet(aCodecCtx, pkt) == 0) {
            while (avcodec_receive_frame(aCodecCtx, frame) == 0) {
              if (audioFirstPtsMs < 0 && frame->pts != AV_NOPTS_VALUE) {
                AVRational tb = fmtCtx->streams[audioStreamIdx]->time_base;
                audioFirstPtsMs = (frame->pts * tb.num * 1000) / tb.den;
                startPtsMs = audioFirstPtsMs;
                systemWallClock.restart();
              }

              if (swrCtx) {
                uint8_t *outBuffer = nullptr;
                int outSamples = av_rescale_rnd(swr_get_delay(swrCtx, 48000) + frame->nb_samples,
                                                48000, frame->sample_rate, AV_ROUND_UP);
                int bufferSize = av_samples_alloc(&outBuffer, nullptr, 2, outSamples, AV_SAMPLE_FMT_S16, 0);
                if (bufferSize > 0) {
                  int converted = swr_convert(swrCtx, &outBuffer, outSamples,
                                              (const uint8_t **)frame->data, frame->nb_samples);
                  if (converted > 0 && audioIO && m_audioSink->state() != QAudio::StoppedState) {
                    qint64 dataSize = (qint64)converted * 2 * sizeof(int16_t);
                    const char *ptr = (const char *)outBuffer;
                    qint64 totalWritten = 0;
                    while (totalWritten < dataSize && m_running.load()) {
                      qint64 written = audioIO->write(ptr + totalWritten, dataSize - totalWritten);
                      if (written > 0) {
                        totalWritten += written;
                      } else {
                        std::this_thread::sleep_for(std::chrono::milliseconds(2));
                      }
                    }
                  }
                  av_freep(&outBuffer);
                }
              }
            }
          }
        }
        // Process Video Packet: Decode into queued frame
        else if (pkt->stream_index == videoStreamIdx && vCodecCtx) {
          if (avcodec_send_packet(vCodecCtx, pkt) == 0) {
            while (avcodec_receive_frame(vCodecCtx, frame) == 0) {
              AVFrame *renderFrame = frame;
              if (frame->format == AV_PIX_FMT_D3D11) {
                if (av_hwframe_transfer_data(swFrame, frame, 0) == 0) {
                  renderFrame = swFrame;
                }
              }

              qint64 ptsMs = 0;
              if (renderFrame->pts != AV_NOPTS_VALUE) {
                AVRational tb = fmtCtx->streams[videoStreamIdx]->time_base;
                ptsMs = (renderFrame->pts * tb.num * 1000) / tb.den;
              } else {
                ptsMs = lastVideoPtsMs + 33;
              }
              lastVideoPtsMs = ptsMs;

              if (startPtsMs < 0) {
                startPtsMs = ptsMs;
                systemWallClock.restart();
              }

              int dstW = renderFrame->width;
              int dstH = renderFrame->height;

              if (!swsCtx || vCodecCtx->width != dstW || vCodecCtx->height != dstH) {
                if (swsCtx) sws_freeContext(swsCtx);
                swsCtx = sws_getContext(dstW, dstH, (enum AVPixelFormat)renderFrame->format,
                                        dstW, dstH, AV_PIX_FMT_RGBA,
                                        SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
              }

              if (swsCtx) {
                QImage qimg(dstW, dstH, QImage::Format_RGBA8888);
                uint8_t *dstData[4] = { qimg.bits(), nullptr, nullptr, nullptr };
                int dstLinesize[4] = { (int)qimg.bytesPerLine(), 0, 0, 0 };
                sws_scale(swsCtx, renderFrame->data, renderFrame->linesize, 0, dstH, dstData, dstLinesize);

                QVideoFrame vFrame(qimg);
                videoQueue.push_back({vFrame, ptsMs});
              }
            }
          }
        }
        av_packet_unref(pkt);
      }

      // 4. Video Presenter (Sync against monotonic Master Clock)
      if (!videoQueue.empty()) {
        qint64 masterNow = getMasterClock();
        const auto &nextFrame = videoQueue.front();
        qint64 delta = nextFrame.ptsMs - masterNow;

        // Frame is ready to display (or within 10ms tolerance)
        if (delta <= 10) {
          // If frame is drastically late (>150ms), drop frames to catch up smoothly
          if (delta < -150 && videoQueue.size() > 1) {
            videoQueue.pop_front();
          } else {
            emit frameReady(nextFrame.frame, nextFrame.ptsMs);
            videoQueue.pop_front();
          }
        } else {
          // Frame is in the future: short slice sleep (<=10ms)
          int sleepMs = (int)std::clamp(delta, (qint64)1, (qint64)10);
          std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
        }
      } else if (eofReached) {
        // Video file finished
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        break;
      } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
      }
    }

    // Cleanup
    if (m_audioSink) {
      m_audioSink->stop();
      m_audioSink.reset();
    }
    if (swsCtx) sws_freeContext(swsCtx);
    if (swrCtx) swr_free(&swrCtx);
    if (vCodecCtx) avcodec_free_context(&vCodecCtx);
    if (aCodecCtx) avcodec_free_context(&aCodecCtx);
    if (frame) av_frame_free(&frame);
    if (swFrame) av_frame_free(&swFrame);
    if (pkt) av_packet_free(&pkt);
    if (fmtCtx) avformat_close_input(&fmtCtx);

    emit stateChanged(NativeMediaPlayer::StoppedState);
    emit finished();
  }

private:
  QString m_filePath;
  std::atomic<bool> m_running{false};
  std::atomic<bool> m_paused{false};
  std::atomic<qint64> m_seekRequested{-1};
  std::atomic<float> m_volume{1.0f};
  std::mutex m_queueMutex;
  std::condition_variable m_queueCond;
  std::unique_ptr<QAudioSink> m_audioSink;
};

NativeMediaPlayer::NativeMediaPlayer(QObject *parent) : QObject(parent) {}

NativeMediaPlayer::~NativeMediaPlayer() {
  cleanupWorker();
}

QObject* NativeMediaPlayer::videoSink() const {
  return m_videoSink.data();
}

void NativeMediaPlayer::setVideoSink(QObject *sink) {
  QVideoSink *vs = qobject_cast<QVideoSink*>(sink);
  if (m_videoSink != vs) {
    m_videoSink = vs;
    emit videoSinkChanged();
  }
}

QString NativeMediaPlayer::source() const {
  return m_source;
}

void NativeMediaPlayer::setSource(const QString &src) {
  if (m_source != src) {
    stop();
    m_source = src;
    emit sourceChanged();
  }
}

qint64 NativeMediaPlayer::position() const {
  return m_position;
}

qint64 NativeMediaPlayer::duration() const {
  return m_duration;
}

int NativeMediaPlayer::playbackState() const {
  return m_playbackState;
}

qreal NativeMediaPlayer::volume() const {
  return m_volume;
}

void NativeMediaPlayer::setVolume(qreal vol) {
  vol = std::clamp(vol, 0.0, 1.0);
  if (!qFuzzyCompare(m_volume, vol)) {
    m_volume = vol;
    if (m_worker) {
      m_worker->setVolume(m_volume, m_muted);
    }
    emit volumeChanged();
  }
}

bool NativeMediaPlayer::muted() const {
  return m_muted;
}

void NativeMediaPlayer::setMuted(bool mute) {
  if (m_muted != mute) {
    m_muted = mute;
    if (m_worker) {
      m_worker->setVolume(m_volume, m_muted);
    }
    emit mutedChanged();
  }
}

bool NativeMediaPlayer::hasVideo() const {
  return m_hasVideo;
}

bool NativeMediaPlayer::hasAudio() const {
  return m_hasAudio;
}

void NativeMediaPlayer::play() {
  if (m_playbackState == PausedState && m_worker) {
    m_worker->requestPause(false);
    m_playbackState = PlayingState;
    emit playbackStateChanged();
    return;
  }

  if (m_source.isEmpty()) return;

  cleanupWorker();

  QString cleanPath = m_source;
  if (cleanPath.startsWith("file:", Qt::CaseInsensitive)) {
    QUrl url(cleanPath);
    if (url.isLocalFile()) cleanPath = url.toLocalFile();
  }
  if (cleanPath.startsWith("/") && cleanPath.length() > 2 && cleanPath[2] == ':') {
    cleanPath = cleanPath.mid(1);
  }
  cleanPath = QDir::toNativeSeparators(cleanPath);

  m_workerThread = new QThread(this);
  m_worker = new NativeMediaPlayerWorker(cleanPath);
  m_worker->moveToThread(m_workerThread);
  m_worker->setVolume(m_volume, m_muted);

  connect(m_workerThread, &QThread::started, m_worker, &NativeMediaPlayerWorker::process);
  connect(m_worker, &NativeMediaPlayerWorker::frameReady, this, &NativeMediaPlayer::onWorkerFrameReady);
  connect(m_worker, &NativeMediaPlayerWorker::durationChanged, this, &NativeMediaPlayer::onWorkerDurationChanged);
  connect(m_worker, &NativeMediaPlayerWorker::stateChanged, this, &NativeMediaPlayer::onWorkerStateChanged);
  connect(m_worker, &NativeMediaPlayerWorker::hasStreams, this, &NativeMediaPlayer::onWorkerHasStreams);
  connect(m_worker, &NativeMediaPlayerWorker::finished, this, [this]() {
    emit endOfMedia();
  });

  m_workerThread->start(QThread::TimeCriticalPriority);
  m_playbackState = PlayingState;
  emit playbackStateChanged();
}

void NativeMediaPlayer::pause() {
  if (m_playbackState == PlayingState && m_worker) {
    m_worker->requestPause(true);
    m_playbackState = PausedState;
    emit playbackStateChanged();
  }
}

void NativeMediaPlayer::stop() {
  cleanupWorker();
  m_playbackState = StoppedState;
  m_position = 0;
  emit playbackStateChanged();
  emit positionChanged();
}

void NativeMediaPlayer::setPosition(qint64 positionMs) {
  if (m_worker) {
    m_worker->requestSeek(positionMs);
  }
  m_position = positionMs;
  emit positionChanged();
}

void NativeMediaPlayer::onWorkerFrameReady(const QVideoFrame &frame, qint64 ptsMs) {
  if (m_videoSink) {
    m_videoSink->setVideoFrame(frame);
  }
  m_position = ptsMs;
  emit positionChanged();
}

void NativeMediaPlayer::onWorkerDurationChanged(qint64 durationMs) {
  m_duration = durationMs;
  emit durationChanged();
}

void NativeMediaPlayer::onWorkerStateChanged(int state) {
  if (m_playbackState != state) {
    m_playbackState = state;
    emit playbackStateChanged();
  }
}

void NativeMediaPlayer::onWorkerHasStreams(bool hasVid, bool hasAud) {
  m_hasVideo = hasVid;
  m_hasAudio = hasAud;
  emit hasVideoChanged();
  emit hasAudioChanged();
}

void NativeMediaPlayer::cleanupWorker() {
  if (m_worker) {
    m_worker->stopWorker();
  }
  if (m_workerThread) {
    m_workerThread->quit();
    m_workerThread->wait(1000);
    delete m_worker;
    m_worker = nullptr;
    delete m_workerThread;
    m_workerThread = nullptr;
  }
}

#include "NativeMediaPlayer.moc"
