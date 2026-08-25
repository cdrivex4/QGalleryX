#ifndef NATIVEMEDIAPLAYER_H
#define NATIVEMEDIAPLAYER_H

#include <QObject>
#include <QString>
#include <QPointer>
#include <QVideoSink>
#include <QVideoFrame>
#include <QAudioSink>
#include <QAudioFormat>
#include <QMediaDevices>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QElapsedTimer>
#include <atomic>
#include <memory>

class NativeMediaPlayerWorker;

class NativeMediaPlayer : public QObject {
  Q_OBJECT

  Q_PROPERTY(QObject* videoSink READ videoSink WRITE setVideoSink NOTIFY videoSinkChanged)
  Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
  Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
  Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
  Q_PROPERTY(int playbackState READ playbackState NOTIFY playbackStateChanged)
  Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)
  Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY mutedChanged)
  Q_PROPERTY(bool hasVideo READ hasVideo NOTIFY hasVideoChanged)
  Q_PROPERTY(bool hasAudio READ hasAudio NOTIFY hasAudioChanged)

public:
  enum PlaybackState {
    StoppedState = 0,
    PlayingState = 1,
    PausedState = 2
  };
  Q_ENUM(PlaybackState)

  explicit NativeMediaPlayer(QObject *parent = nullptr);
  ~NativeMediaPlayer() override;

  QObject* videoSink() const;
  void setVideoSink(QObject *sink);

  QString source() const;
  void setSource(const QString &src);

  qint64 position() const;
  qint64 duration() const;

  int playbackState() const;
  qreal volume() const;
  void setVolume(qreal vol);

  bool muted() const;
  void setMuted(bool mute);

  bool hasVideo() const;
  bool hasAudio() const;

public slots:
  void play();
  void pause();
  void stop();
  void setPosition(qint64 positionMs);

signals:
  void videoSinkChanged();
  void sourceChanged();
  void positionChanged();
  void durationChanged();
  void playbackStateChanged();
  void volumeChanged();
  void mutedChanged();
  void hasVideoChanged();
  void hasAudioChanged();
  void endOfMedia();

private slots:
  void onWorkerFrameReady(const QVideoFrame &frame, qint64 ptsMs);
  void onWorkerDurationChanged(qint64 durationMs);
  void onWorkerStateChanged(int state);
  void onWorkerHasStreams(bool hasVid, bool hasAud);

private:
  void cleanupWorker();

  QPointer<QVideoSink> m_videoSink;
  QString m_source;
  qint64 m_position = 0;
  qint64 m_duration = 0;
  int m_playbackState = StoppedState;
  qreal m_volume = 1.0;
  bool m_muted = false;
  bool m_hasVideo = false;
  bool m_hasAudio = false;

  NativeMediaPlayerWorker *m_worker = nullptr;
  QThread *m_workerThread = nullptr;
};

#endif // NATIVEMEDIAPLAYER_H
