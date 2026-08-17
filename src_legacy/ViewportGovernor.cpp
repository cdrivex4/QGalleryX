#include "ViewportGovernor.h"
#include "TaskScheduler.h"
#include <QDebug>
#include <algorithm>

ViewportGovernor &ViewportGovernor::instance() {
  static ViewportGovernor gov;
  return gov;
}

ViewportGovernor::ViewportGovernor(QObject *parent) : QObject(parent) {}

void ViewportGovernor::updateViewport(int firstVisible, int lastVisible, int totalCount, int scrollDelta) {
  if (firstVisible < 0 || lastVisible < firstVisible) return;

  int count = lastVisible - firstVisible + 1;
  int minBound = 0;
  int maxBound = std::max(0, totalCount - 1);

  // 2x Lookahead Logic: 1x count ahead, 1x count behind
  int lMin = std::max(minBound, firstVisible - count);
  int lMax = std::min(maxBound, lastVisible + count);

  m_firstVisible.store(firstVisible);
  m_lastVisible.store(lastVisible);
  m_lookaheadMin.store(lMin);
  m_lookaheadMax.store(lMax);

  // Fast scroll fling detection (|scrollDelta| > 40)
  bool fastScroll = (std::abs(scrollDelta) > 40);
  m_isFastScrolling.store(fastScroll);

  if (fastScroll) {
    // Suspend low priority background tasks only during active fast scroll fling
    TaskScheduler::instance().pauseBackground(true);
  } else {
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
