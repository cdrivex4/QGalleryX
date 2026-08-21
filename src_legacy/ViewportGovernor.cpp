#include "ViewportGovernor.h"
#include "TaskScheduler.h"
#include <QDebug>
#include <algorithm>

ViewportGovernor &ViewportGovernor::instance() {
  static ViewportGovernor gov;
  return gov;
}

ViewportGovernor::ViewportGovernor(QObject *parent) : QObject(parent) {
  m_flingTimer.setSingleShot(true);
  connect(&m_flingTimer, &QTimer::timeout, this, [this]() {
    m_isFastScrolling.store(false);
    TaskScheduler::instance().pauseBackground(false);
  });
}

void ViewportGovernor::updateViewport(int firstVisible, int lastVisible, int totalCount, int scrollDelta) {
  if (firstVisible < 0 || lastVisible < firstVisible) return;

  int count = lastVisible - firstVisible + 1;
  int minBound = 0;
  int maxBound = std::max(0, totalCount - 1);

  // Directional Lookahead Logic based on scroll trajectory
  int lMin, lMax;
  if (scrollDelta > 0) {
    // Scrolling down / forward: heavy lookahead ahead of current position
    lMin = std::max(minBound, firstVisible - count / 2);
    lMax = std::min(maxBound, lastVisible + 2 * count);
  } else if (scrollDelta < 0) {
    // Scrolling up / backward: heavy lookahead behind current position
    lMin = std::max(minBound, firstVisible - 2 * count);
    lMax = std::min(maxBound, lastVisible + count / 2);
  } else {
    // Idle / Centered lookahead
    lMin = std::max(minBound, firstVisible - count);
    lMax = std::min(maxBound, lastVisible + count);
  }

  m_firstVisible.store(firstVisible);
  m_lastVisible.store(lastVisible);
  m_lookaheadMin.store(lMin);
  m_lookaheadMax.store(lMax);

  // Fast scroll fling detection (|scrollDelta| > 40)
  bool fastScroll = (std::abs(scrollDelta) > 40);

  if (fastScroll) {
    m_isFastScrolling.store(true);
    // Suspend low priority background tasks only during active fast scroll fling
    TaskScheduler::instance().pauseBackground(true);
    // Restart debounce timer — automatically unpauses 150ms after scrolling stops
    m_flingTimer.start(150);
  } else if (!m_isFastScrolling.load()) {
    TaskScheduler::instance().pauseBackground(false);
  }

  emit viewportChanged();
}


bool ViewportGovernor::isLookaheadTile(int index) const {
  return (index >= m_lookaheadMin.load() && index <= m_lookaheadMax.load());
}

bool ViewportGovernor::isOutOfLookaheadBounds(int index) const {
  return (index < m_lookaheadMin.load() || index > m_lookaheadMax.load());
}

void ViewportGovernor::setBatterySaverMode(bool enable) {
  if (m_batterySaverMode.exchange(enable) != enable) {
    qDebug() << "[ViewportGovernor] Battery Saver Mode:" << (enable ? "ENABLED" : "DISABLED");
    emit batterySaverModeChanged();
  }
}
