#ifndef TASKSCHEDULER_H
#define TASKSCHEDULER_H

#include "RingBufferDispatcher.h"
#include <QMap>
#include <QMutex>
#include <QObject>
#include <QQueue>
#include <QThread>
#include <QWaitCondition>
#include <atomic>
#include <functional>
#include <vector>


/**
 * @brief The TaskScheduler class manages background work with priorities.
 * It strictly separates IO-bound work (metadata, file access) from
 * CPU-bound work (image decoding) to prevent UI thread starvation.
 */
class TaskScheduler : public QObject {
  Q_OBJECT
  Q_PROPERTY(int schedulerGovernor READ getSchedulerGovernor WRITE
                 setSchedulerGovernor NOTIFY schedulerGovernorChanged)
public:
  enum SchedulerGovernor {
    Governor_FIFO = 0,
    Governor_LIFO = 1,
    Governor_Adaptive = 2,
    Governor_RoundRobin = 3
  };
  enum TaskType {
    CPU_BOUND, // Image Decoding, Processing
    IO_BOUND   // Metadata read, Directory scan (Fast)
  };

  enum Priority {
    Immediate = 0, // Currently visible on screen
    High = 1,      // Likely to be visible soon
    Normal = 2,    // General background work
    Low = 3        // Prefetching far ahead
  };

  using Task = std::function<void()>;

  static TaskScheduler &instance();

  // Add a task to the queue. Returns true if successfully queued, false if rejected (e.g. shutdown or full)
  bool addTask(Task task, TaskType type = CPU_BOUND, Priority priority = Normal,
               const QString &taskKey = "");

  // Stop all threads (for shutdown)
  void stop();

  // Clear all pending tasks
  void clear();

  // Pause and Resume background processing
  Q_INVOKABLE void pause();
  Q_INVOKABLE void resume();
  Q_INVOKABLE void pauseBackground(bool pause);
  bool isPaused() const;
  bool isRunning() const;

  int getSchedulerGovernor() const { return m_governor.load(); }
  void setSchedulerGovernor(int gov);

  int getQueueSize(Priority p) const;
  bool hasImmediateTasks() const;
  void waitForImmediateTasksToFinish();

private:
  TaskScheduler();
  ~TaskScheduler();

signals:
  void schedulerGovernorChanged();

private:
  void cpuWorkerLoop();
  void ioWorkerLoop();

private:
  struct TaskEntry {
    Task task;
    QString key;
    quint64 sequence; // To maintain FIFO within same priority
  };

  // Priority Queue comparators or management
  // We use a Map<Priority, Queue> for simplicity and strict ordering

  // CPU Pool
  std::vector<std::thread> m_cpuThreads;
  QMutex m_cpuMutex;
  QWaitCondition m_cpuCondition;

  // IO Pool (Usually 1-2 threads)
  std::vector<std::thread> m_ioThreads;
  QMap<Priority, QList<TaskEntry>> m_ioQueue;
  QMutex m_ioMutex;
  QWaitCondition m_ioCondition;

  std::atomic<bool> m_running;
  std::atomic<bool> m_paused;
  std::atomic<bool> m_backgroundPaused;
  std::atomic<quint64> m_sequenceCounter;
  std::atomic<int> m_governor{0};

  std::atomic<int> m_activeCpuThreads{0};

  std::atomic<int> m_urgentTaskCount{0};
  QMutex m_urgentTaskMutex;
  QWaitCondition m_urgentTaskCondition;

  RingBufferDispatcher m_dispatcher;
};

#endif // TASKSCHEDULER_H
