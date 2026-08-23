#include "LogManager.h"
#include <QCoreApplication>
#include <QThread>
#include <iostream>

LogManager &LogManager::instance() {
  static LogManager instance;
  return instance;
}

LogManager::LogManager() : m_running(true) {
  m_writerThread = std::thread(&LogManager::writerLoop, this);
}

LogManager::~LogManager() {
  m_running = false;
  m_queueCv.notify_all();
  if (m_writerThread.joinable()) {
    m_writerThread.join();
  }
  if (m_logFile.isOpen()) {
    m_logFile.close();
  }
}

void LogManager::setLogFile(const QString &path) {
  std::lock_guard<std::mutex> lock(m_queueMutex);
  m_logPath = path;
  // File opening happens in writer thread or lazily?
  // Let's keep it simple: open it in writer thread or here if thread safe.
  // For safety, let's defer to writer.
}

void LogManager::setMaxRingBufferSize(int lines) {
  QMutexLocker lock(&m_ringMutex);
  m_maxRingSize = lines;
}

void LogManager::setLogLevel(int level) { m_logLevel = level; }

void LogManager::messageHandler(QtMsgType type,
                                const QMessageLogContext &context,
                                const QString &msg) {
  instance().processLog(type, context, msg);
}

void LogManager::processLog(QtMsgType type, const QMessageLogContext &context,
                            const QString &msg) {
  // Filter based on Log Level
  // 0 = None (Critical/Fatal only?) Or strictly nothing? Let's say
  // Critical/Fatal is always forced. 1 = Basic (Info, Warning, Critical, Fatal)
  // 2 = Verbose (Debug + all above)

  if (m_logLevel < 2 && type == QtDebugMsg)
    return;
  if (m_logLevel < 1 && (type == QtInfoMsg || type == QtWarningMsg))
    return;

  // Format Log
  QString timeStr =
      QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
  QString threadId =
      QString::number((quint64)(uintptr_t)QThread::currentThreadId());

  QString level;
  switch (type) {
  case QtDebugMsg:
    level = "Debug";
    break;
  case QtInfoMsg:
    level = "Info";
    break;
  case QtWarningMsg:
    level = "Warning";
    break;
  case QtCriticalMsg:
    level = "Critical";
    break;
  case QtFatalMsg:
    level = "Fatal";
    break;
  }

  QString formattedMsg =
      QString("[%1] [Thread %2] %3: %4").arg(timeStr, threadId, level, msg);

  // 1. Storage in Ring Buffer (Mutex Protected)
  {
    QMutexLocker lock(&m_ringMutex);
    LogEntry entry;
    entry.timestamp = QDateTime::currentMSecsSinceEpoch();
    entry.type = type;
    entry.message = msg;
    entry.threadId = (quint64)(uintptr_t)QThread::currentThreadId();

    m_ringBuffer.append(entry);
    if (m_ringBuffer.size() > m_maxRingSize) {
      m_ringBuffer.removeFirst();
    }
  }

  // 2. Output to Console (Immediate)
  if (type == QtCriticalMsg || type == QtFatalMsg) {
    std::cerr << formattedMsg.toStdString() << std::endl;
  } else {
    std::cout << formattedMsg.toStdString() << std::endl;
  }

  // 3. Async Write to Disk
  {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_writeQueue.enqueue(formattedMsg);
  }
  m_queueCv.notify_one();

  if (type == QtFatalMsg) {
    // Fatal means abort. We should verify we flushed logs?
    // But abort() is coming.
  }
}

void LogManager::writerLoop() {
  while (m_running) {
    QStringList buffer;
    {
      std::unique_lock<std::mutex> lock(m_queueMutex);
      m_queueCv.wait(lock,
                     [this] { return !m_writeQueue.isEmpty() || !m_running; });

      while (!m_writeQueue.isEmpty()) {
        buffer.append(m_writeQueue.dequeue());
      }
    }

    if (!buffer.isEmpty()) {
      if (!m_logFile.isOpen() && !m_logPath.isEmpty()) {
        m_logFile.setFileName(m_logPath);
        m_logFile.open(QIODevice::WriteOnly | QIODevice::Append |
                       QIODevice::Text);
      }

      if (m_logFile.isOpen()) {
        QTextStream ts(&m_logFile);
        for (const QString &line : buffer) {
          ts << line << Qt::endl;
        }
        m_logFile.flush();
      }
    }
  }
}

void LogManager::dumpCrashLog(const QString &path) {
  QFile dumpFile(path);
  if (dumpFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream ts(&dumpFile);
    QMutexLocker lock(&m_ringMutex);
    ts << "--- MSG DUMP ---" << Qt::endl;
    for (const auto &entry : m_ringBuffer) {
      ts << QDateTime::fromMSecsSinceEpoch(entry.timestamp)
                .toString(Qt::ISODate)
         << " " << entry.message << Qt::endl;
    }
  }
}

QStringList LogManager::getLastLogs(int count) {
  QMutexLocker lock(&m_ringMutex);
  QStringList logs;
  int start = std::max(0, (int)m_ringBuffer.size() - count);
  for (int i = start; i < m_ringBuffer.size(); ++i) {
    logs.append(m_ringBuffer[i].message);
  }
  return logs;
}
