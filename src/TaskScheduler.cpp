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
    // Scale CPU workers to 80% of available cores to keep UI stable
    cpuWorkerCount = std::max(2, static_cast<int>(cpuCores * 0.8));
  }

  int ioWorkerCount = 4; // Increased from 2 to allow better burst handling

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

void TaskScheduler::expandIOPool(int count) {
  if (!m_running)
    return;
  // Note: m_ioThreads modification is not mutex protected against stop(),
  // but stop() is only called on shutdown.
  for (int i = 0; i < count; ++i) {
    m_ioThreads.emplace_back(&TaskScheduler::ioWorkerLoop, this);
  }
  qInfo() << "[TaskScheduler] Expanded IO pool by" << count
          << "threads. New Total:" << m_ioThreads.size();
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

void TaskScheduler::addTask(Task task, TaskType type, Priority priority, TaskCategory category) {
  if (!m_running)
    return;

  m_activeTaskCount++;
  triggerCountUpdate();

  if (m_activeTaskCount % 100 == 0) {
    qDebug() << "[TaskScheduler] Task added. Active count:"
             << m_activeTaskCount.load();
  }

  int catIdx = static_cast<int>(category);

  if (type == CPU_BOUND) {
    QMutexLocker lock(&m_cpuMutex);
    m_cpuQueues[catIdx][priority].prepend(task);
    m_cpuCondition.wakeOne();
  } else {
    QMutexLocker lock(&m_ioMutex);
    m_ioQueues[catIdx][priority].prepend(task);
    m_ioCondition.wakeOne();
  }
}

void TaskScheduler::clear() {
  clear(CPU_BOUND);
  clear(IO_BOUND);
}

void TaskScheduler::clear(TaskType type) {
  int removedCount = 0;
  
  auto processMap = [&](auto &queues) {
    for (int i = 0; i < 2; ++i) {
      for (auto it = queues[i].begin(); it != queues[i].end(); ++it) {
        for (const auto &task : it.value()) {
          if (task.onDropped)
            task.onDropped();
        }
        removedCount += it.value().size();
      }
      queues[i].clear();
    }
  };

  if (type == CPU_BOUND) {
    QMutexLocker lock(&m_cpuMutex);
    processMap(m_cpuQueues);
  } else {
    QMutexLocker lock(&m_ioMutex);
    processMap(m_ioQueues);
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
      for (int i = 0; i < 2; ++i) {
        for (const auto &queue : m_cpuQueues[i]) {
          if (!queue.isEmpty()) { hasWork = true; break; }
        }
      }

      while ((!hasWork || m_isPaused) && m_running) {
        m_cpuCondition.wait(&m_cpuMutex);
        hasWork = false;
        if (m_running && !m_isPaused) {
          for (int i = 0; i < 2; ++i) {
            for (const auto &queue : m_cpuQueues[i]) {
              if (!queue.isEmpty()) { hasWork = true; break; }
            }
          }
        }
      }

      if (!m_running)
        break;

      int turn = m_cpuRatioCounter.fetch_add(1) % 6;
      int preferredCategory = (turn < 5) ? 0 : 1;
      int fallbackCategory = (preferredCategory == 0) ? 1 : 0;

      auto tryPop = [&](int cat) -> bool {
        auto it = m_cpuQueues[cat].begin();
        while (it != m_cpuQueues[cat].end()) {
          while (!it.value().isEmpty()) {
            task = it.value().takeFirst();
            if (task.isNeeded && !task.isNeeded()) {
              if (task.onDropped) {
                task.onDropped();
              }
              m_activeTaskCount--;
              triggerCountUpdate();
              continue;
            }
            found = true;
            return true;
          }
          ++it;
        }
        return false;
      };

      if (!tryPop(preferredCategory)) {
        tryPop(fallbackCategory);
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
      for (int i = 0; i < 2; ++i) {
        for (const auto &queue : m_ioQueues[i]) {
          if (!queue.isEmpty()) { hasWork = true; break; }
        }
      }

      while ((!hasWork || m_isPaused) && m_running) {
        m_ioCondition.wait(&m_ioMutex);
        hasWork = false;
        if (m_running && !m_isPaused) {
          for (int i = 0; i < 2; ++i) {
            for (const auto &queue : m_ioQueues[i]) {
              if (!queue.isEmpty()) { hasWork = true; break; }
            }
          }
        }
      }

      if (!m_running)
        break;

      int turn = m_ioRatioCounter.fetch_add(1) % 6;
      int preferredCategory = (turn < 5) ? 0 : 1;
      int fallbackCategory = (preferredCategory == 0) ? 1 : 0;

      auto tryPop = [&](int cat) -> bool {
        auto it = m_ioQueues[cat].begin();
        while (it != m_ioQueues[cat].end()) {
          while (!it.value().isEmpty()) {
            task = it.value().takeFirst();
            if (task.isNeeded && !task.isNeeded()) {
              m_activeTaskCount--;
              triggerCountUpdate();
              continue;
            }
            found = true;
            return true;
          }
          ++it;
        }
        return false;
      };

      if (!tryPop(preferredCategory)) {
        tryPop(fallbackCategory);
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
