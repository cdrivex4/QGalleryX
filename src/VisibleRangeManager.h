#ifndef VISIBLERANGEMANAGER_H
#define VISIBLERANGEMANAGER_H

#include <QDebug>
#include <QDir>
#include <QMutex>
#include <QMutexLocker>
#include <QSet>
#include <QString>

#include <QUrl>

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

  int visiblePathCount() {
    QMutexLocker lock(&m_mutex);
    return m_visiblePaths.size();
  }

private:
  QString normalizePath(const QString &p) {
    QString input = p;
    if (input.startsWith("image://async/"))
      input = input.mid(14);

    // Use QUrl to parse reliably, especially for UNC/Network paths
    QUrl url(input);
    QString path;

    if (url.scheme() == "file" || url.isLocalFile()) {
      path = url.toLocalFile();
    } else {
      // Fallback for raw paths
      path = p;
      int qIdx = path.indexOf('?');
      if (qIdx != -1)
        path = path.left(qIdx);

      if (path.startsWith("file:///"))
        path = path.mid(8);
      else if (path.startsWith("file://"))
        path = path.mid(7);

      path = QUrl::fromPercentEncoding(path.toUtf8());
    }

    path = QDir::cleanPath(path);

#ifdef Q_OS_WIN
    if (path.length() >= 2 && path[1] == ':') {
      path[0] = path[0].toUpper();
    }
#endif

    return path;
  }

private:
  VisibleRangeManager() {}
  QSet<QString> m_visiblePaths;
  QMutex m_mutex;
};

#endif // VISIBLERANGEMANAGER_H
