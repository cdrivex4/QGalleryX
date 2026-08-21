#ifndef VIEWPORTGOVERNOR_H
#define VIEWPORTGOVERNOR_H

#include <QObject>
#include <QMutex>
#include <QTimer>
#include <atomic>

class ViewportGovernor : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool batterySaverMode READ isBatterySaverMode WRITE setBatterySaverMode NOTIFY batterySaverModeChanged)
  Q_PROPERTY(int firstVisible READ firstVisible NOTIFY viewportChanged)
  Q_PROPERTY(int lastVisible READ lastVisible NOTIFY viewportChanged)
  Q_PROPERTY(int lookaheadMin READ lookaheadMin NOTIFY viewportChanged)
  Q_PROPERTY(int lookaheadMax READ lookaheadMax NOTIFY viewportChanged)

public:
  static ViewportGovernor &instance();

  Q_INVOKABLE void updateViewport(int firstVisible, int lastVisible, int totalCount, int scrollDelta = 0);
  Q_INVOKABLE bool isLookaheadTile(int index) const;
  Q_INVOKABLE bool isOutOfLookaheadBounds(int index) const;

  bool isBatterySaverMode() const { return m_batterySaverMode.load(); }
  void setBatterySaverMode(bool enable);

  int firstVisible() const { return m_firstVisible.load(); }
  int lastVisible() const { return m_lastVisible.load(); }
  int lookaheadMin() const { return m_lookaheadMin.load(); }
  int lookaheadMax() const { return m_lookaheadMax.load(); }

signals:
  void viewportChanged();
  void batterySaverModeChanged();

private:
  explicit ViewportGovernor(QObject *parent = nullptr);
  ~ViewportGovernor() override = default;

  std::atomic<int> m_firstVisible{0};
  std::atomic<int> m_lastVisible{0};
  std::atomic<int> m_lookaheadMin{0};
  std::atomic<int> m_lookaheadMax{0};
  std::atomic<bool> m_batterySaverMode{false};
  std::atomic<bool> m_isFastScrolling{false};
  QTimer m_flingTimer;
  mutable QMutex m_mutex;
};

#endif // VIEWPORTGOVERNOR_H

