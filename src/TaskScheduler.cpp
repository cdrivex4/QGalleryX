#include "TaskScheduler.h"
#include "LogManager.h"
#include <QDebug>
#include <QSettings>
#include <algorithm>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

TaskScheduler &TaskScheduler::instance() {
  static TaskScheduler instance;
  return instance;
}

TaskScheduler::TaskScheduler() : m_running(true), m_sequenceCounter(0) {
  // Determine thread counts
  int cpuCores = std::thread::hardware_concurrency();

  // Check settings
  QSettings settings("SamsungClone", "Gallery");
  int settingsThreads = settings.value("concurrentThreads", -1).toInt();

  int cpuWorkerCount;
  if (settingsThreads > 0) {
    cpuWorkerCount = settingsThreads;
  } else {
    // Cap at 8 threads to prevent saturating memory bus and causing lag
    cpuWorkerCount = std::clamp(cpuCores - 1, 2, 8);
  }

  int ioWorkerCount = 2; // Dedicated IO threads

  // Start CPU Workers
  for (int i = 0; i < cpuWorkerCount; ++i) {
    m_cpuThreads.emplace_back(&TaskScheduler::cpuWorkerLoop, this);
  }

  // Start IO Workers
  for (int i = 0; i < ioWorkerCount; ++i) {
    m_ioThreads.emplace_back(&TaskScheduler::ioWorkerLoop, this);
  }

  qDebug() << "[TaskScheduler] Started" << cpuWorkerCount << "CPU threads and"
           << ioWorkerCount << "IO threads.";
}

TaskScheduler::~TaskScheduler() { stop(); }

void TaskScheduler::stop() {
  m_running = false;
  m_isPaused = false; // Ensure threads can wake up to terminate
  m_cpuCondition.wakeAll();
  m_ioCondition.wakeAll();

  for (auto &t : m_cpuThreads) {
    if (t.joinable())
      t.join();
  }
  for (auto &t : m_ioThreads) {
    if (t.joinable())
      t.join();
  }
}

void TaskScheduler::pause() {
  m_isPaused.store(true);
  qDebug() << "[TaskScheduler] Paused.";
}

void TaskScheduler::resume() {
  m_isPaused.store(false);
  qDebug() << "[TaskScheduler] Resumed.";
  // Wake threads up so they can start processing again
  m_cpuCondition.wakeAll();
  m_ioCondition.wakeAll();
}

void TaskScheduler::togglePause(bool paused) {
  if (paused) {
    pause();
  } else {
    resume();
  }
}

int TaskScheduler::activeTaskCount() const { return m_activeTaskCount.load(); }

void TaskScheduler::triggerCountUpdate() {
  if (m_updatePending.exchange(true))
    return;
  QMetaObject::invokeMethod(this, "emitCountChanged", Qt::QueuedConnection);
}

void TaskScheduler::emitCountChanged() {
  m_updatePending.store(false);
  emit activeTaskCountChanged();
}

void TaskScheduler::addTask(Task task, TaskType type, Priority priority) {
  if (!m_running)
    return;

  m_activeTaskCount++;
  triggerCountUpdate();

  if (m_activeTaskCount % 100 == 0) {
    qDebug() << "[TaskScheduler] Task added. Active count:"
             << m_activeTaskCount.load();
  }

  if (type == CPU_BOUND) {
    QMutexLocker lock(&m_cpuMutex);
    // Use prepend (LIFO) for CPU tasks to prioritize most recent viewport
    m_cpuQueue[priority].prepend(task);
    m_cpuCondition.wakeOne();
  } else {
    QMutexLocker lock(&m_ioMutex);
    // Use append (FIFO) for IO tasks to maintain scan order
    m_ioQueue[priority].append(task);
    m_ioCondition.wakeOne();
  }
}

void TaskScheduler::clear() {
  clear(CPU_BOUND);
  clear(IO_BOUND);
}

void TaskScheduler::clear(TaskType type) {
  int removedCount = 0;
  if (type == CPU_BOUND) {
    QMutexLocker lock(&m_cpuMutex);
    for (const auto &queue : m_cpuQueue)
      removedCount += queue.size();
    m_cpuQueue.clear();
  } else {
    QMutexLocker lock(&m_ioMutex);
    for (const auto &queue : m_ioQueue)
      removedCount += queue.size();
    m_ioQueue.clear();
  }

  // Decrement counter by the number of tasks that will now never run
  if (removedCount > 0) {
    m_activeTaskCount -= removedCount;
    triggerCountUpdate();
  }

  qDebug() << "[TaskScheduler] Cleared" << (type == CPU_BOUND ? "CPU" : "IO")
           << "queue. Removed:" << removedCount << "tasks.";
}

void TaskScheduler::cpuWorkerLoop() {
#ifdef Q_OS_WIN
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
#endif
  while (m_running) {
    Task task;
    bool found = false;

    {
      QMutexLocker lock(&m_cpuMutex);
      bool hasWork = false;
      for (const auto &queue : m_cpuQueue) {
        if (!queue.isEmpty()) {
          hasWork = true;
          break;
        }
      }

      while ((!hasWork || m_isPaused) && m_running) {
        m_cpuCondition.wait(&m_cpuMutex);

        // Re-check after waking up
        hasWork = false;
        if (m_running && !m_isPaused) {
          for (const auto &queue : m_cpuQueue) {
            if (!queue.isEmpty()) {
              hasWork = true;
              break;
            }
          }
        }
      }

      if (!m_running)
        break;

      // Find highest priority task
      // Keys are sorted: 0 (Immediate) ... 3 (Low)
      // Iterate map keys in ascending order
      auto it = m_cpuQueue.begin();
      while (it != m_cpuQueue.end()) {
        if (!it.value().isEmpty()) {
          task = it.value().takeFirst();
          found = true;
          break;
        }
        ++it;
      }
    }

    if (found && task) {
      try {
        task();
      } catch (const std::exception &e) {
        qCritical() << "[TaskScheduler] CPU Task Exception:" << e.what();
      } catch (...) {
        qCritical() << "[TaskScheduler] CPU Task Unknown Exception";
      }
      m_activeTaskCount--;
      triggerCountUpdate();
    }
  }
}

void TaskScheduler::ioWorkerLoop() {
#ifdef Q_OS_WIN
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
#endif
  while (m_running) {
    Task task;
    bool found = false;

    {
      QMutexLocker lock(&m_ioMutex);
      bool hasWork = false;
      for (const auto &queue : m_ioQueue) {
        if (!queue.isEmpty()) {
          hasWork = true;
          break;
        }
      }

      while ((!hasWork || m_isPaused) && m_running) {
        m_ioCondition.wait(&m_ioMutex);

        // Re-check
        hasWork = false;
        if (m_running && !m_isPaused) {
          for (const auto &queue : m_ioQueue) {
            if (!queue.isEmpty()) {
              hasWork = true;
              break;
            }
          }
        }
      }

      if (!m_running)
        break;

      auto it = m_ioQueue.begin();
      while (it != m_ioQueue.end()) {
        if (!it.value().isEmpty()) {
          task = it.value().takeFirst();
          found = true;
          break;
        }
        ++it;
      }
    }

    if (found && task) {
      try {
        task();
      } catch (const std::exception &e) {
        qCritical() << "[TaskScheduler] IO Task Exception:" << e.what();
      } catch (...) {
        qCritical() << "[TaskScheduler] IO Task Unknown Exception";
      }
      m_activeTaskCount--;
      triggerCountUpdate();
    }
  }
}
