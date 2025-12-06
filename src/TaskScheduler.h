#ifndef TASKSCHEDULER_H
#define TASKSCHEDULER_H

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
public:
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

  // Add a task to the queue
  void addTask(Task task, TaskType type = CPU_BOUND,
               Priority priority = Normal);

  // Stop all threads (for shutdown)
  void stop();

  // Clear all pending tasks
  void clear();

private:
  TaskScheduler();
  ~TaskScheduler();

  void cpuWorkerLoop();
  void ioWorkerLoop();

private:
  struct TaskEntry {
    Task task;
    Priority priority;
    quint64 sequence; // To maintain FIFO within same priority
  };

  // Priority Queue comparators or management
  // We use a Map<Priority, Queue> for simplicity and strict ordering

  // CPU Pool
  std::vector<std::thread> m_cpuThreads;
  QMap<Priority, QQueue<Task>> m_cpuQueue;
  QMutex m_cpuMutex;
  QWaitCondition m_cpuCondition;

  // IO Pool (Usually 1-2 threads)
  std::vector<std::thread> m_ioThreads;
  QMap<Priority, QQueue<Task>> m_ioQueue;
  QMutex m_ioMutex;
  QWaitCondition m_ioCondition;

  std::atomic<bool> m_running;
  std::atomic<quint64> m_sequenceCounter;
};

#endif // TASKSCHEDULER_H
