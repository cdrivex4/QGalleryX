#include "TaskScheduler.h"
#include "LogManager.h"
#include "SystemMonitor.h"
#include <QSettings>
#include <iostream>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

TaskScheduler &TaskScheduler::instance() {
  static TaskScheduler instance;
  return instance;
}

TaskScheduler::TaskScheduler()
    : m_running(true), m_paused(false), m_backgroundPaused(false),
      m_sequenceCounter(0) {
  int cpuCores = std::thread::hardware_concurrency();

  QSettings settings("SamsungClone", "Gallery");
  int settingsThreads = settings.value("concurrentThreads", -1).toInt();

  int cpuWorkerCount;
  if (settingsThreads > 0) {
    cpuWorkerCount = settingsThreads;
  } else {
    cpuWorkerCount = std::max(2, cpuCores - 1);
  }

  int ioWorkerCount = 2;

  for (int i = 0; i < cpuWorkerCount; ++i) {
    m_cpuThreads.emplace_back(&TaskScheduler::cpuWorkerLoop, this);
  }

  for (int i = 0; i < ioWorkerCount; ++i) {
    m_ioThreads.emplace_back(&TaskScheduler::ioWorkerLoop, this);
  }

  qDebug() << "[TaskScheduler] Initialized with" << cpuWorkerCount
           << "CPU workers and" << ioWorkerCount << "IO workers.";
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

void TaskScheduler::setSchedulerGovernor(int gov) {
  if (m_governor.load() != gov) {
    m_governor.store(gov);
    emit schedulerGovernorChanged();
  }
}

bool TaskScheduler::addTask(Task task, TaskType type, Priority priority,
                            const QString &taskKey) {
  if (!m_running)
    return false;

  if (type == CPU_BOUND) {
    RingBufferDispatcher::RingPriority rp;
    if (priority == Immediate || priority == High)
      rp = RingBufferDispatcher::Ring0_Immediate;
    else if (priority == Normal)
      rp = RingBufferDispatcher::Ring1_Lookahead;
    else
      rp = RingBufferDispatcher::Ring2_Precache;

    if (!m_dispatcher.push(task, rp, taskKey)) {
      return false; // Ring buffer full!
    }
    m_cpuCondition.wakeOne();
    return true;
  } else {
    QMutexLocker lock(&m_ioMutex);

    TaskEntry entry;
    entry.task = task;
    entry.key = taskKey;
    entry.sequence = m_sequenceCounter++;

    if (!taskKey.isEmpty() && m_ioQueue.contains(priority)) {
      for (const auto &existing : m_ioQueue[priority]) {
        if (existing.key == taskKey) {
          return true; // Already queued
        }
      }
    }

    m_ioQueue[priority].append(entry);
    m_ioCondition.wakeOne();
    return true;
  }
}

int TaskScheduler::getQueueSize(Priority p) const {
  if (p == Immediate || p == High)
    return m_dispatcher.size(RingBufferDispatcher::Ring0_Immediate);
  if (p == Normal)
    return m_dispatcher.size(RingBufferDispatcher::Ring1_Lookahead);
  if (p == Low)
    return m_dispatcher.size(RingBufferDispatcher::Ring2_Precache);
  return 0;
}

bool TaskScheduler::hasImmediateTasks() const {
  return m_dispatcher.size(RingBufferDispatcher::Ring0_Immediate) > 0;
}

void TaskScheduler::waitForImmediateTasksToFinish() {
  QMutexLocker lock(&m_urgentTaskMutex);
  while (hasImmediateTasks() && m_running.load()) {
    m_urgentTaskCondition.wait(&m_urgentTaskMutex, 50);
  }
}

void TaskScheduler::clear() {
  {
    QMutexLocker lock(&m_cpuMutex);
    m_dispatcher.clear();
  }
  {
    QMutexLocker lock(&m_ioMutex);
    m_ioQueue.clear();
  }
  qDebug() << "[TaskScheduler] Cleared all pending tasks.";
}

void TaskScheduler::pause() { m_paused = true; }

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

bool TaskScheduler::isPaused() const { return m_paused; }

bool TaskScheduler::isRunning() const { return m_running; }

void TaskScheduler::cpuWorkerLoop() {
#ifdef Q_OS_WIN
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
#endif
  while (m_running) {
    Task task;
    bool found = false;

    {
      QMutexLocker lock(&m_cpuMutex);

      while ((m_paused || m_dispatcher.isEmpty()) && m_running) {
        m_cpuCondition.wait(&m_cpuMutex);
      }

      if (!m_running)
        break;

      RingBufferDispatcher::DispatchEntry entry;
      if (m_dispatcher.pop(entry, m_governor.load())) {
        // Skip background tasks if background is paused
        if (m_backgroundPaused &&
            entry.priority != RingBufferDispatcher::Ring0_Immediate) {
          // Re-queue the task — pop() is destructive, can't just skip
          m_dispatcher.push(entry.task, entry.priority, entry.key);
          m_cpuCondition.wait(&m_cpuMutex, 100); // Back off to avoid spin
          continue;
        }

        // Starvation Protection
        if (entry.priority == RingBufferDispatcher::Ring2_Precache &&
            m_activeCpuThreads.load() >=
                std::max(1, static_cast<int>(m_cpuThreads.size()) - 1)) {
          // Re-queue it (push back) and continue
          m_dispatcher.push(entry.task, entry.priority, entry.key);
          continue;
        }

        task = entry.task;
        found = true;
      }
    }

    if (found && task) {
      m_activeCpuThreads++;
      try {
        task();
      } catch (const std::exception &e) {
        std::cerr << "[TaskScheduler] CPU Task Exception: " << e.what()
                  << std::endl;
      } catch (...) {
        std::cerr << "[TaskScheduler] CPU Task Unknown Exception" << std::endl;
      }
      m_activeCpuThreads--;
    } else if (!found && m_running) {
      QMutexLocker lock(&m_cpuMutex);
      m_cpuCondition.wait(&m_cpuMutex, 50);
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

      while ((m_paused || m_ioQueue.isEmpty()) && m_running) {
        m_ioCondition.wait(&m_ioMutex);
      }

      if (!m_running)
        break;

      for (auto it = m_ioQueue.begin(); it != m_ioQueue.end(); ++it) {
        if (it.value().isEmpty())
          continue;

        if (m_backgroundPaused && it.key() != Immediate && it.key() != High) {
          continue;
        }

        task = it.value().takeFirst().task;
        found = true;
        break;
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
    } else if (!found && m_running) {
      QMutexLocker lock(&m_ioMutex);
      m_ioCondition.wait(&m_ioMutex, 50);
    }
  }
}
