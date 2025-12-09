#include "ScrollBenchImageModel.h"
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
  beginResetModel();
  m_items.clear();

  QStringList filters;
  filters << "*.jpg" << "*.jpeg" << "*.png" << "*.bmp" << "*.gif"
          << "*.webp" << "*.cr2" << "*.nef" << "*.arw" << "*.dng";

  // Recursive scan using QDirIterator
  QDirIterator it(path, filters, QDir::Files | QDir::Readable,
                  QDirIterator::Subdirectories);

  while (it.hasNext()) {
    QString filePath = it.next();
    QFileInfo fileInfo(filePath);

    ImageItem item;
    item.fileName = fileInfo.fileName();
    item.path = fileInfo.absoluteFilePath();
    item.color = "#666666";
    item.isLoaded = false;
    m_items.append(item);
  }

  m_totalItems = m_items.count();
  endResetModel();

  qDebug() << "Loaded" << m_totalItems << "images from" << path
           << "(recursive)";

  if (m_viewportCullingEnabled && m_visibleEndIndex > 0) {
    updateVisibleRange();
  }
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

  m_pendingDecodes++;
  emit pendingDecodeCountChanged();

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
      Qt::QueuedConnection);
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
