#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include <QDateTime>
#include <QFile>
#include <QList>
#include <QMutex>
#include <QObject>
#include <QQueue>
#include <QTextStream>
#include <atomic>
#include <condition_variable>
#include <thread>

/**
 * @brief The LogManager class handles application logging asynchronously.
 * It features a Ring Buffer for crashdumps and disk I/O on a dedicated thread.
 */
class LogManager : public QObject {
  Q_OBJECT
public:
  static LogManager &instance();

  // Config
  void setLogFile(const QString &path);
  void setMaxRingBufferSize(int lines);
  void setLogLevel(int level);

  // Qt Message Handler integration
  static void messageHandler(QtMsgType type, const QMessageLogContext &context,
                             const QString &msg);

  // Dump Ring Buffer to file (e.g. on crash)
  Q_INVOKABLE void dumpCrashLog(const QString &path);

  // Retrieve last N logs for UI
  Q_INVOKABLE QStringList getLastLogs(int count = 100);

private:
  LogManager();
  ~LogManager();

  void processLog(QtMsgType type, const QMessageLogContext &context,
                  const QString &msg);
  void writerLoop();

private:
  QString m_logPath;
  QFile m_logFile;
  QTextStream m_fileStream;

  // Ring Buffer
  struct LogEntry {
    qint64 timestamp;
    QtMsgType type;
    QString message;
    QString category;
    quint64 threadId;
  };
  QList<LogEntry> m_ringBuffer;
  int m_maxRingSize = 1000;
  QMutex m_ringMutex;

  // Async Writer
  std::thread m_writerThread;
  std::atomic<bool> m_running;
  QQueue<QString> m_writeQueue;
  std::mutex m_queueMutex;
  std::condition_variable m_queueCv;

  std::atomic<int> m_logLevel{2}; // Default to Verbose (2)
};

#endif // LOGMANAGER_H
