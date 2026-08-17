#ifndef TASKSCHEDULER_H
#define TASKSCHEDULER_H

#include <QMap>
#include <QMutex>
#include <QObject>
#include <QQueue>
#include <QThread>
#include <QTimer>
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
  Q_PROPERTY(
      int activeTaskCount READ activeTaskCount NOTIFY activeTaskCountChanged)
  Q_PROPERTY(int schedulerGovernor READ getSchedulerGovernor WRITE setSchedulerGovernor NOTIFY schedulerGovernorChanged)

public:
  enum SchedulerGovernor {
      Governor_FIFO = 0,
      Governor_LIFO = 1,
      Governor_Adaptive = 2,
      Governor_RoundRobin = 3
  };

  static TaskScheduler &instance();
  void pause();
  void resume();

  struct Task {
      std::function<void()> run;
      std::function<bool()> isNeeded;
      std::function<void()> onDropped;
      Task() = default;
      template <typename Callable>
      Task(Callable r) : run(std::move(r)), isNeeded(nullptr), onDropped(nullptr) {}
      template <typename Callable1, typename Callable2>
      Task(Callable1 r, Callable2 i) : run(std::move(r)), isNeeded(std::move(i)), onDropped(nullptr) {}
      template <typename Callable1, typename Callable2, typename Callable3>
      Task(Callable1 r, Callable2 i, Callable3 d) : run(std::move(r)), isNeeded(std::move(i)), onDropped(std::move(d)) {}
      operator bool() const { return run != nullptr; }
      void operator()() const { if (run) run(); }
  };
  enum TaskType { CPU_BOUND, IO_BOUND };
  enum TaskCategory { ImageTask = 0, VideoTask = 1 };
  enum Priority { Immediate = 0, Normal = 1, Low = 2, Background = 3 };

  TaskScheduler();
  ~TaskScheduler();

  int activeTaskCount() const;
  bool isPaused() const { return m_isPaused.load(); }
  bool hasUrgentTasks() const { return m_urgentTaskCount.load() > 0; }

  int getSchedulerGovernor() const { return m_governor.load(); }
  void setSchedulerGovernor(int gov);

  void addTask(Task task, TaskType type = CPU_BOUND,
               Priority priority = Normal, TaskCategory category = ImageTask);
  void clear();
  void clear(TaskType type);
  void stop();
  void expandIOPool(int count = 1);

  Q_INVOKABLE void togglePause(bool paused);

signals:
  void activeTaskCountChanged();
  void schedulerGovernorChanged();

private slots:
  void emitCountChanged();

private:
  void cpuWorkerLoop();
  void ioWorkerLoop();
  void triggerCountUpdate();

  std::atomic<int> m_activeTaskCount{0};
  std::atomic<int> m_urgentTaskCount{0};
  std::atomic<bool> m_updatePending{false};
  std::atomic<int> m_governor{0}; // Default to FIFO

  // CPU Pool
  std::vector<std::thread> m_cpuThreads;
  QMap<Priority, QList<Task>> m_cpuQueues[2]; // 0: Image, 1: Video
  QMutex m_cpuMutex;
  QWaitCondition m_cpuCondition;
  std::atomic<int> m_cpuRatioCounter{0};

  // IO Pool (Usually 1-2 threads)
  std::vector<std::thread> m_ioThreads;
  QMap<Priority, QList<Task>> m_ioQueues[2]; // 0: Image, 1: Video
  QMutex m_ioMutex;
  QWaitCondition m_ioCondition;
  std::atomic<int> m_ioRatioCounter{0};

  std::atomic<bool> m_running;
  std::atomic<bool> m_isPaused{false};
  std::atomic<quint64> m_sequenceCounter;
};

#endif // TASKSCHEDULER_H
