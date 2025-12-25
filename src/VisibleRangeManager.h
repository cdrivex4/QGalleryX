#ifndef VISIBLERANGEMANAGER_H
#define VISIBLERANGEMANAGER_H

#include <QDebug>
#include <QMutex>
#include <QMutexLocker>
#include <QSet>
#include <QString>

class VisibleRangeManager {
public:
  static VisibleRangeManager &instance() {
    static VisibleRangeManager inst;
    return inst;
  }

  void setVisiblePaths(const QSet<QString> &paths) {
    QMutexLocker lock(&m_mutex);
    m_visiblePaths.clear();
    for (const QString &p : paths) {
      m_visiblePaths.insert(normalizePath(p));
    }
    if (!m_visiblePaths.isEmpty()) {
      qDebug() << "[VRM] Total visible paths:" << m_visiblePaths.size()
               << "Sample:" << *m_visiblePaths.begin();
    }
  }

  bool isPathVisible(const QString &path) {
    QMutexLocker lock(&m_mutex);
    return m_visiblePaths.contains(normalizePath(path));
  }

private:
  QString normalizePath(const QString &p) {
    QString n = p;
    n.replace("\\", "/");
    if (n.startsWith("file:///"))
      n = n.mid(8);
    else if (n.startsWith("file://"))
      n = n.mid(7);
    return n;
  }

private:
  VisibleRangeManager() {}
  QSet<QString> m_visiblePaths;
  QMutex m_mutex;
};

#endif // VISIBLERANGEMANAGER_H
