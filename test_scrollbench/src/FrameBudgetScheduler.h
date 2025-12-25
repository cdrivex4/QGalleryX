#ifndef FRAMEBUDGETSCHEDULER_H
#define FRAMEBUDGETSCHEDULER_H

#include <QElapsedTimer>
#include <QObject>
#include <QQueue>
#include <functional>

class FrameBudgetScheduler : public QObject {
  Q_OBJECT
  Q_PROPERTY(int frameBudget READ frameBudget WRITE setFrameBudget NOTIFY
                 frameBudgetChanged)
  Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
  Q_PROPERTY(int completionsThisFrame READ completionsThisFrame NOTIFY
                 completionsThisFrameChanged)
  Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged)

public:
  explicit FrameBudgetScheduler(QObject *parent = nullptr);

  int frameBudget() const { return m_frameBudget; }
  void setFrameBudget(int budget);

  bool enabled() const { return m_enabled; }
  void setEnabled(bool enabled);

  int completionsThisFrame() const { return m_completionsThisFrame; }

  bool paused() const { return m_paused; }
  void setPaused(bool paused);

  // Called when a task completes
  Q_INVOKABLE void onTaskCompleted(const std::function<void()> &callback);

signals:
  void frameBudgetChanged();
  void enabledChanged();
  void completionsThisFrameChanged();
  void pausedChanged();
  void taskReadyImmediate(); // Emit immediately
  void taskReadyDeferred();  // Emit on next frame

private slots:
  void checkFrameBoundary();

private:
  int m_frameBudget = 10; // Max completions per 16ms frame
  bool m_enabled = true;
  bool m_paused = false;
  int m_completionsThisFrame = 0;
  QElapsedTimer m_frameTimer;
  QQueue<std::function<void()>> m_deferredTasks;
};

#endif // FRAMEBUDGETSCHEDULER_H
