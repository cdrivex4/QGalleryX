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

public:
  using Task = std::function<void()>;
  enum TaskType { CPU_BOUND, IO_BOUND };
  enum Priority { Immediate = 0, Normal = 1, Low = 2, Background = 3 };

  static TaskScheduler &instance();

  TaskScheduler();
  ~TaskScheduler();

  int activeTaskCount() const;
  bool isPaused() const { return m_isPaused.load(); }

  void addTask(Task task, TaskType type = CPU_BOUND,
               Priority priority = Normal);
  void clear();
  void clear(TaskType type);
  void stop();

  Q_INVOKABLE void togglePause(bool paused);
  void pause();
  void resume();

signals:
  void activeTaskCountChanged();

private slots:
  void emitCountChanged();

private:
  void cpuWorkerLoop();
  void ioWorkerLoop();
  void triggerCountUpdate();

  std::atomic<int> m_activeTaskCount{0};
  std::atomic<bool> m_updatePending{false};

  // CPU Pool
  std::vector<std::thread> m_cpuThreads;
  QMap<Priority, QList<Task>> m_cpuQueue;
  QMutex m_cpuMutex;
  QWaitCondition m_cpuCondition;

  // IO Pool (Usually 1-2 threads)
  std::vector<std::thread> m_ioThreads;
  QMap<Priority, QList<Task>> m_ioQueue;
  QMutex m_ioMutex;
  QWaitCondition m_ioCondition;

  std::atomic<bool> m_running;
  std::atomic<bool> m_isPaused{false};
  std::atomic<quint64> m_sequenceCounter;
};

#endif // TASKSCHEDULER_H
