#include "FrameBudgetScheduler.h"
#include <QDebug>
#include <QTimer>


FrameBudgetScheduler::FrameBudgetScheduler(QObject *parent) : QObject(parent) {
  m_frameTimer.start();

  // Check frame boundary every 16ms (60 FPS)
  QTimer *frameChecker = new QTimer(this);
  connect(frameChecker, &QTimer::timeout, this,
          &FrameBudgetScheduler::checkFrameBoundary);
  frameChecker->start(16);
}

void FrameBudgetScheduler::setFrameBudget(int budget) {
  if (m_frameBudget != budget) {
    m_frameBudget = budget;
    emit frameBudgetChanged();
    qDebug() << "Frame budget set to:" << budget;
  }
}

void FrameBudgetScheduler::setEnabled(bool enabled) {
  if (m_enabled != enabled) {
    m_enabled = enabled;
    emit enabledChanged();
    qDebug() << "Frame budget" << (enabled ? "enabled" : "disabled");

    if (enabled) {
      // Reset frame counter
      m_completionsThisFrame = 0;
      m_frameTimer.restart();
    } else {
      // Process all deferred tasks immediately
      while (!m_deferredTasks.isEmpty()) {
        auto task = m_deferredTasks.dequeue();
        task();
      }
    }
  }
}

void FrameBudgetScheduler::onTaskCompleted(
    const std::function<void()> &callback) {
  if (!m_enabled) {
    // Budget disabled, execute immediately
    callback();
    emit taskReadyImmediate();
    return;
  }

  // Check if we've crossed frame boundary
  if (m_frameTimer.elapsed() >= 16) {
    // New frame started
    m_completionsThisFrame = 0;
    m_frameTimer.restart();
    emit completionsThisFrameChanged();

    // Process some deferred tasks from previous frame
    int processed = 0;
    while (!m_deferredTasks.isEmpty() && processed < m_frameBudget / 2) {
      auto task = m_deferredTasks.dequeue();
      task();
      processed++;
      m_completionsThisFrame++;
    }
  }

  if (m_completionsThisFrame < m_frameBudget) {
    // Within budget, execute immediately
    callback();
    m_completionsThisFrame++;
    emit completionsThisFrameChanged();
    emit taskReadyImmediate();
  } else {
    // Over budget, defer to next frame
    m_deferredTasks.enqueue(callback);
    emit taskReadyDeferred();
  }
}

void FrameBudgetScheduler::checkFrameBoundary() {
  if (m_frameTimer.elapsed() >= 16) {
    // Frame boundary crossed
    int previousCount = m_completionsThisFrame;
    m_completionsThisFrame = 0;
    m_frameTimer.restart();

    if (previousCount > 0) {
      emit completionsThisFrameChanged();
    }

    // Process deferred tasks
    int processed = 0;
    while (!m_deferredTasks.isEmpty() && processed < m_frameBudget) {
      auto task = m_deferredTasks.dequeue();
      task();
      processed++;
      m_completionsThisFrame++;
    }

    if (processed > 0) {
      emit completionsThisFrameChanged();
    }
  }
}
