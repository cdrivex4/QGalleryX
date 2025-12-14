#include "ScrollBenchImageModel.h"
#include "../src/TaskScheduler.h"
#include <QColor>
#include <QDebug>
#include <QDirIterator>
#include <QRandomGenerator>

ScrollBenchImageModel::ScrollBenchImageModel(QObject *parent)
    : QAbstractListModel(parent) {}

int ScrollBenchImageModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return m_items.count();
}

QVariant ScrollBenchImageModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= m_items.count())
    return QVariant();

  const ImageItem &item = m_items[index.row()];

  switch (role) {
  case PathRole:
    return item.path;
  case FileNameRole:
    return item.fileName;
  case IndexRole:
    return index.row();
  case IsLoadedRole:
    return item.isLoaded;
  case ColorRole:
    return item.color;
  case IsSelectedRole:
    return item.isSelected;
  default:
    return QVariant();
  }
}

QHash<int, QByteArray> ScrollBenchImageModel::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[PathRole] = "path";
  roles[FileNameRole] = "fileName";
  roles[IndexRole] = "imageIndex";
  roles[IsLoadedRole] = "isLoaded";
  roles[ColorRole] = "testColor";
  roles[IsSelectedRole] = "isSelected";
  return roles;
}

void ScrollBenchImageModel::setVisibleStartIndex(int index) {
  if (m_visibleStartIndex != index) {
    m_visibleStartIndex = index;
    emit visibleRangeChanged();

    if (m_viewportCullingEnabled) {
      updateVisibleRange();
    }
  }
}

void ScrollBenchImageModel::setVisibleEndIndex(int index) {
  if (m_visibleEndIndex != index) {
    m_visibleEndIndex = index;
    emit visibleRangeChanged();

    if (m_viewportCullingEnabled) {
      updateVisibleRange();
    }
  }
}

void ScrollBenchImageModel::setViewportCullingEnabled(bool enabled) {
  if (m_viewportCullingEnabled != enabled) {
    m_viewportCullingEnabled = enabled;
    emit viewportCullingEnabledChanged();

    if (enabled) {
      cancelPendingRequests();
      updateVisibleRange();
    } else {
      for (int i = 0; i < m_items.count(); ++i) {
        requestThumbnail(i);
      }
    }
  }
}

void ScrollBenchImageModel::generateTestData(int count) {
  beginResetModel();
  m_items.clear();
  m_items.reserve(count);

  QStringList colors = {"#FF6B6B", "#4ECDC4", "#45B7D1", "#FFA07A", "#98D8C8",
                        "#F7DC6F", "#BB8FCE", "#85C1E2", "#F8B739", "#52B788"};

  for (int i = 0; i < count; ++i) {
    ImageItem item;
    item.fileName = QString("test_image_%1.jpg").arg(i, 5, 10, QChar('0'));
    item.path = QString("synthetic://test/%1").arg(item.fileName);
    item.color = colors[i % colors.size()];
    item.isLoaded = false;
    m_items.append(item);
  }

  m_totalItems = count;
  endResetModel();

  qDebug() << "Generated" << count << "test items";

  if (m_viewportCullingEnabled && m_visibleEndIndex > 0) {
    updateVisibleRange();
  }
}

void ScrollBenchImageModel::clearData() {
  beginResetModel();
  m_items.clear();
  m_totalItems = 0;
  endResetModel();
}

void ScrollBenchImageModel::scanDirectory(const QString &path) {
  if (m_isLoading)
    return;

  m_isLoading = true;
  emit isLoadingChanged();

  beginResetModel();
  m_items.clear();
  m_visibleStartIndex = 0;
  m_visibleEndIndex = 0;
  // Clear previous data
  endResetModel();

  cancelPendingRequests();

  // Async Scan via TaskScheduler
  TaskScheduler::instance().addTask(
      [this, path]() {
        QElapsedTimer timer;
        timer.start();

        QString cleanPath = path;
        // Basic naive URL cleaning (similar to main app but simplified for
        // ScrollBench test)
        if (cleanPath.startsWith("file:///")) {
          cleanPath = cleanPath.mid(8);
        }
        cleanPath = QDir::toNativeSeparators(cleanPath);

        if (!QDir(cleanPath).exists()) {
          qWarning() << "Directory does not exist:" << cleanPath;
          QMetaObject::invokeMethod(this, [this]() {
            m_isLoading = false;
            emit isLoadingChanged();
          });
          return;
        }

        QStringList filters;
        filters << "*.jpg" << "*.jpeg" << "*.png" << "*.bmp" << "*.gif"
                << "*.webp" << "*.cr2" << "*.nef" << "*.arw" << "*.dng";

        QDirIterator it(cleanPath, filters, QDir::Files | QDir::Readable,
                        QDirIterator::Subdirectories);

        QVector<ImageItem> batch;
        batch.reserve(100);
        int totalFound = 0;

        while (it.hasNext() && !m_scanCancelled) {
          QString filePath = it.next();
          QFileInfo fileInfo(filePath);

          ImageItem item;
          item.fileName = fileInfo.fileName();
          item.path = fileInfo.absoluteFilePath();
          item.color = "#444444";
          item.isLoaded = false;

          batch.append(item);
          totalFound++;

          // Update UI in batches
          if (batch.count() >= 100) {
            QMetaObject::invokeMethod(this, [this, batch]() {
              beginInsertRows(QModelIndex(), m_items.count(),
                              m_items.count() + batch.count() - 1);
              m_items.append(batch);
              endInsertRows();

              // If culling is on, we might need to trigger requests if they
              // fall in visible range But usually updateVisibleRange() is
              // called by QML changing scroll
            });
            batch.clear();
          }
        }

        // Final batch
        if (!batch.isEmpty()) {
          QMetaObject::invokeMethod(this, [this, batch]() {
            beginInsertRows(QModelIndex(), m_items.count(),
                            m_items.count() + batch.count() - 1);
            m_items.append(batch);
            endInsertRows();
          });
        }

        QMetaObject::invokeMethod(this, [this, timer, totalFound, cleanPath]() {
          m_isLoading = false;
          m_totalItems = m_items.count();
          emit isLoadingChanged();
          qDebug() << "Async Scan finished:" << totalFound << "items in"
                   << timer.elapsed() << "ms from" << cleanPath;
          if (m_viewportCullingEnabled) {
            updateVisibleRange();
          }
        });
      },
      TaskScheduler::IO_BOUND, TaskScheduler::Normal);
}

void ScrollBenchImageModel::updateVisibleRange() {
  if (!m_viewportCullingEnabled) {
    return;
  }

  int startIdx = qMax(0, m_visibleStartIndex - BUFFER_SIZE);
  int endIdx = qMin(m_items.count() - 1, m_visibleEndIndex + BUFFER_SIZE);

  qDebug() << "Visible range updated:" << startIdx << "to" << endIdx
           << "(viewport:" << m_visibleStartIndex << "-" << m_visibleEndIndex
           << ")";

  for (int i = startIdx; i <= endIdx; ++i) {
    if (i >= 0 && i < m_items.count() && !m_items[i].isLoaded) {
      requestThumbnail(i);
    }
  }
}

void ScrollBenchImageModel::requestThumbnail(int index) {
  if (index < 0 || index >= m_items.count()) {
    return;
  }

  // If already loaded or synthetic, skip
  if (m_items[index].isLoaded ||
      m_items[index].path.startsWith("synthetic://")) {
    return;
  }

  // Real image logic
  // Check if already cached in AsyncImageProvider
  QString cacheKey = m_items[index].path;
  // Note: key format depends on provider implementation, typically just path
  // for basic checks

  // We simulate the "request" here by marking strict processing
  // In a real app, the View requests the image via image:// source.
  // BUT for "pre-loading" or budget management, we can query the provider.

  // For ScrollBench, the View (ImageView) triggers the load via source binding.
  // HOWEVER, this model method is called by updateVisibleRange() to "warm up"
  // or track budget.

  // Integrating with FrameBudgetScheduler effectively means we throttle the
  // *notification* or the actual image request.

  // To keep it simple and aligned with the "Budget" concept:
  // We mark it as 'requesting'. The actual QImage load happens in QML.
  // But we can check if it's *ready* to be shown.

  m_pendingDecodes++;
  emit pendingDecodeCountChanged();

  // Simulate async delay or check cache
  // In the full migration, we would call AsyncImageProvider::requestImage()
  // manually if prefetching. For now, let's trust the QML Image element to
  // drive the provider. We just update the 'isLoaded' state to true so we don't
  // request again.

  QMetaObject::invokeMethod(
      this,
      [this, index]() {
        if (index >= 0 && index < m_items.count()) {
          m_items[index].isLoaded = true;
          m_pendingDecodes = qMax(0, m_pendingDecodes - 1);
          emit pendingDecodeCountChanged();
          emit dataChanged(createIndex(index, 0), createIndex(index, 0),
                           {IsLoadedRole});
        }
      },
      Qt::QueuedConnection); // This effectively simulates a "dispatch"
}

void ScrollBenchImageModel::cancelPendingRequests() {
  m_pendingDecodes = 0;
  emit pendingDecodeCountChanged();
}

void ScrollBenchImageModel::cancelScan() { m_scanCancelled = true; }

// Selection methods
void ScrollBenchImageModel::toggleSelection(int index) {
  if (index < 0 || index >= m_items.count())
    return;

  m_items[index].isSelected = !m_items[index].isSelected;

  QModelIndex modelIndex = createIndex(index, 0);
  emit dataChanged(modelIndex, modelIndex, {IsSelectedRole});
  emit selectedCountChanged();
}

void ScrollBenchImageModel::selectRange(int start, int end) {
  if (start < 0 || end < 0 || start >= m_items.count() ||
      end >= m_items.count()) {
    return;
  }

  int realStart = qMin(start, end);
  int realEnd = qMax(start, end);

  bool changed = false;
  for (int i = realStart; i <= realEnd; ++i) {
    if (!m_items[i].isSelected) {
      m_items[i].isSelected = true;
      changed = true;
    }
  }

  if (changed) {
    emit dataChanged(createIndex(realStart, 0), createIndex(realEnd, 0),
                     {IsSelectedRole});
    emit selectedCountChanged();
  }
}

void ScrollBenchImageModel::selectVisualRect(int colMin, int colMax, int rowMin,
                                             int rowMax, int columns) {
  if (columns <= 0)
    return;

  bool changed = false;
  int firstAffected = -1;
  int lastAffected = -1;

  for (int r = rowMin; r <= rowMax; ++r) {
    for (int c = colMin; c <= colMax; ++c) {
      int index = r * columns + c;
      if (index >= 0 && index < m_items.count()) {
        if (!m_items[index].isSelected) {
          m_items[index].isSelected = true;
          changed = true;
          if (firstAffected == -1)
            firstAffected = index;
          lastAffected = index;
        }
      }
    }
  }

  if (changed) {
    // We emit a range covering the min/max index touched, which might include
    // untouched items in between, but that's safe for a simple redraw hint.
    emit dataChanged(createIndex(firstAffected, 0),
                     createIndex(lastAffected, 0), {IsSelectedRole});
    emit selectedCountChanged();
  }
}

void ScrollBenchImageModel::selectAll() {
  for (int i = 0; i < m_items.count(); ++i) {
    m_items[i].isSelected = true;
  }

  if (!m_items.isEmpty()) {
    emit dataChanged(createIndex(0, 0), createIndex(m_items.count() - 1, 0),
                     {IsSelectedRole});
    emit selectedCountChanged();
  }
}

void ScrollBenchImageModel::clearSelection() {
  for (int i = 0; i < m_items.count(); ++i) {
    m_items[i].isSelected = false;
  }

  if (!m_items.isEmpty()) {
    emit dataChanged(createIndex(0, 0), createIndex(m_items.count() - 1, 0),
                     {IsSelectedRole});
    emit selectedCountChanged();
  }
}

void ScrollBenchImageModel::invertSelection() {
  for (int i = 0; i < m_items.count(); ++i) {
    m_items[i].isSelected = !m_items[i].isSelected;
  }

  if (!m_items.isEmpty()) {
    emit dataChanged(createIndex(0, 0), createIndex(m_items.count() - 1, 0),
                     {IsSelectedRole});
    emit selectedCountChanged();
  }
}

void ScrollBenchImageModel::deleteSelected() {
  // Remove items in reverse order to maintain indices
  for (int i = m_items.count() - 1; i >= 0; --i) {
    if (m_items[i].isSelected) {
      beginRemoveRows(QModelIndex(), i, i);
      m_items.removeAt(i);
      endRemoveRows();
    }
  }

  m_totalItems = m_items.count();
  emit selectedCountChanged();
}

int ScrollBenchImageModel::selectedCount() const {
  int count = 0;
  for (const auto &item : m_items) {
    if (item.isSelected)
      ++count;
  }
  return count;
}
