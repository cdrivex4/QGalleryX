#include "TaskScheduler.h"
#include "LogManager.h"
#include <QSettings>
#include <iostream>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

TaskScheduler &TaskScheduler::instance() {
  static TaskScheduler instance;
  return instance;
}

TaskScheduler::TaskScheduler() : m_running(true), m_paused(false), m_sequenceCounter(0) {
  // Determine thread counts
  int cpuCores = std::thread::hardware_concurrency();

  // Check settings
  QSettings settings("SamsungClone", "Gallery");
  int settingsThreads = settings.value("concurrentThreads", -1).toInt();

  int cpuWorkerCount;
  if (settingsThreads > 0) {
    cpuWorkerCount = settingsThreads;
  } else {
    cpuWorkerCount = std::max(2, cpuCores - 1); // Default behavior
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

void TaskScheduler::addTask(Task task, TaskType type, Priority priority) {
  if (!m_running)
    return;

  if (type == CPU_BOUND) {
    QMutexLocker lock(&m_cpuMutex);
    m_cpuQueue[priority].enqueue(task);
    m_cpuCondition.wakeOne();
  } else {
    QMutexLocker lock(&m_ioMutex);
    m_ioQueue[priority].enqueue(task);
    m_ioCondition.wakeOne();
  }
}

void TaskScheduler::clear() {
  {
    QMutexLocker lock(&m_cpuMutex);
    m_cpuQueue.clear();
  }
  {
    QMutexLocker lock(&m_ioMutex);
    m_ioQueue.clear();
  }
  qDebug() << "[TaskScheduler] Cleared all pending tasks.";
}

void TaskScheduler::pause() {
  m_paused = true;
}

void TaskScheduler::resume() {
  m_paused = false;
  m_cpuCondition.wakeAll();
  m_ioCondition.wakeAll();
}

void TaskScheduler::pauseBackground(bool pause) {
  m_backgroundPaused = pause;
  if (!pause) {
    m_cpuCondition.wakeAll();
    m_ioCondition.wakeAll();
  }
}

bool TaskScheduler::isPaused() const {
  return m_paused;
}

bool TaskScheduler::isRunning() const {
  return m_running;
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
      for (auto it = m_cpuQueue.begin(); it != m_cpuQueue.end(); ++it) {
        if (!it.value().isEmpty()) {
          if (m_backgroundPaused && it.key() != Immediate)
            continue;
          hasWork = true;
          break;
        }
      }

      while ((!hasWork || m_paused) && m_running) {
        m_cpuCondition.wait(&m_cpuMutex);

        // Re-check after waking up
        hasWork = false;
        if (m_running && !m_paused) {
          for (auto it = m_cpuQueue.begin(); it != m_cpuQueue.end(); ++it) {
            if (!it.value().isEmpty()) {
              if (m_backgroundPaused && it.key() != Immediate)
                continue;
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
          if (m_backgroundPaused && it.key() != Immediate) {
            ++it;
            continue;
          }
          task = it.value().dequeue();
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
        std::cerr << "[TaskScheduler] CPU Task Exception: " << e.what()
                  << std::endl;
      } catch (...) {
        std::cerr << "[TaskScheduler] CPU Task Unknown Exception" << std::endl;
      }
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
      for (auto it = m_ioQueue.begin(); it != m_ioQueue.end(); ++it) {
        if (!it.value().isEmpty()) {
          if (m_backgroundPaused && it.key() != Immediate)
            continue;
          hasWork = true;
          break;
        }
      }

      while ((!hasWork || m_paused) && m_running) {
        m_ioCondition.wait(&m_ioMutex);

        // Re-check after waking up
        hasWork = false;
        if (m_running && !m_paused) {
          for (auto it = m_ioQueue.begin(); it != m_ioQueue.end(); ++it) {
            if (!it.value().isEmpty()) {
              if (m_backgroundPaused && it.key() != Immediate)
                continue;
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
          if (m_backgroundPaused && it.key() != Immediate) {
            ++it;
            continue;
          }
          task = it.value().dequeue();
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
        std::cerr << "[TaskScheduler] IO Task Exception: " << e.what()
                  << std::endl;
      } catch (...) {
        std::cerr << "[TaskScheduler] IO Task Unknown Exception" << std::endl;
      }
    }
  }
}
