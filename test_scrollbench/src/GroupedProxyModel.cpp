#include "GroupedProxyModel.h"
#include "ImageModel.h"

// ... (existing includes)

// ... (existing code)

QVariantList GroupedProxyModel::getYearDistribution() const {
  QVariantList list;
  if (!m_sourceModel)
    return list;

  int lastYear = -1;
  ImageModel *im = qobject_cast<ImageModel *>(m_sourceModel);

  for (int i = 0; i < m_index.count(); ++i) {
    const IndexItem &item = m_index[i];

    if (item.type == RowItem) {
      int sourceIndex = item.sourceStartIndex;
      if (sourceIndex >= 0 && sourceIndex < m_sourceModel->rowCount()) {
        int year = 0;
        if (im) {
          year = static_cast<int>(im->getGroupKey(sourceIndex, ImageModel::SectionYearRole));
        } else {
          QModelIndex srcIdx = m_sourceModel->index(sourceIndex, 0);
          year = m_sourceModel->data(srcIdx, ImageModel::SectionYearRole).toInt();
        }

        if (year != lastYear && year > 1900) {
          QVariantMap map;
          map["year"] = year;
          map["proxyIndex"] = i;
          list.append(map);
          lastYear = year;
        }
      }
    }
  }
  return list;
}
#include <QDebug>

GroupedProxyModel::GroupedProxyModel(QObject *parent)
    : QAbstractListModel(parent) {}

QAbstractListModel *GroupedProxyModel::sourceModel() const {
  return m_sourceModel;
}

void GroupedProxyModel::deleteSelected() {
  if (ImageModel *im = qobject_cast<ImageModel *>(m_sourceModel)) {
    im->deleteSelected();
  }
}

int GroupedProxyModel::selectedCount() const {
  if (ImageModel *im = qobject_cast<ImageModel *>(m_sourceModel)) {
    return im->selectedCount();
  }
  return 0;
}

QStringList GroupedProxyModel::getSelectedPaths() const {
  if (ImageModel *im = qobject_cast<ImageModel *>(m_sourceModel)) {
    return im->getSelectedPaths();
  }
  return QStringList();
}

qint64 GroupedProxyModel::getSelectedTotalSizeBytes() const {
  if (ImageModel *im = qobject_cast<ImageModel *>(m_sourceModel)) {
    return im->getSelectedTotalSizeBytes();
  }
  return 0;
}

void GroupedProxyModel::pauseBackgroundTasks() {
  if (ImageModel *im = qobject_cast<ImageModel *>(m_sourceModel)) {
    im->pauseBackgroundTasks();
  }
}

void GroupedProxyModel::resumeBackgroundTasks() {
  if (ImageModel *im = qobject_cast<ImageModel *>(m_sourceModel)) {
    im->resumeBackgroundTasks();
  }
}

void GroupedProxyModel::setSourceModel(QAbstractListModel *model) {
  if (m_sourceModel == model)
    return;

    if (m_sourceModel) {
      disconnect(m_sourceModel, &QAbstractListModel::modelReset, this,
                 &GroupedProxyModel::onSourceModelReset);
      disconnect(m_sourceModel, &QAbstractListModel::rowsInserted, this,
                 &GroupedProxyModel::onSourceModelReset);
      disconnect(m_sourceModel, &QAbstractListModel::rowsRemoved, this,
                 &GroupedProxyModel::onSourceModelReset);
                 
      if (ImageModel *im = qobject_cast<ImageModel *>(m_sourceModel)) {
          disconnect(im, &ImageModel::precacheModeChanged, this, &GroupedProxyModel::precacheModeChanged);
          disconnect(im, &ImageModel::totalCountChanged, this, &GroupedProxyModel::totalCountChanged);
      }
    }

  m_sourceModel = model;

    if (m_sourceModel) {
      connect(m_sourceModel, &QAbstractListModel::modelReset, this,
              &GroupedProxyModel::onSourceModelReset);
      connect(m_sourceModel, &QAbstractListModel::rowsInserted, this,
              &GroupedProxyModel::onSourceModelReset);
      connect(m_sourceModel, &QAbstractListModel::rowsRemoved, this,
              &GroupedProxyModel::onSourceModelReset);
              
      if (ImageModel *im = qobject_cast<ImageModel *>(m_sourceModel)) {
          connect(im, &ImageModel::precacheModeChanged, this, &GroupedProxyModel::precacheModeChanged);
          connect(im, &ImageModel::totalCountChanged, this, &GroupedProxyModel::totalCountChanged);
      }
    }

  rebuildIndex();
  emit sourceModelChanged();
}

int GroupedProxyModel::precacheMode() const {
    if (ImageModel *im = qobject_cast<ImageModel *>(m_sourceModel)) {
        return im->precacheMode();
    }
    return 1;
}

void GroupedProxyModel::setPrecacheMode(int mode) {
    if (ImageModel *im = qobject_cast<ImageModel *>(m_sourceModel)) {
        im->setPrecacheMode(mode);
    }
}

int GroupedProxyModel::totalCount() const {
    if (ImageModel *im = qobject_cast<ImageModel *>(m_sourceModel)) {
        return im->totalCount();
    }
    return rowCount();
}

int GroupedProxyModel::indexOfPath(const QString &path) const {
    if (ImageModel *im = qobject_cast<ImageModel *>(m_sourceModel)) {
        int sourceIndex = im->indexOfPath(path);
        if (sourceIndex >= 0) {
            QModelIndex proxyIdx = getProxyIndexForSourceIndex(sourceIndex);
            if (proxyIdx.isValid()) {
                return proxyIdx.row();
            }
        }
    }
    return -1;
}

int GroupedProxyModel::columns() const { return m_columns; }

void GroupedProxyModel::setColumns(int columns) {
  if (m_columns == columns || columns < 1)
    return;
  m_columns = columns;
  rebuildIndex();
  emit columnsChanged();
}

int GroupedProxyModel::groupRole() const { return m_groupRole; }

void GroupedProxyModel::setGroupRole(int role) {
  if (m_groupRole == role)
    return;
  m_groupRole = role;
  rebuildIndex();
  emit groupRoleChanged();
}

int GroupedProxyModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return m_index.count();
}

QVariant GroupedProxyModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() >= m_index.count())
    return QVariant();

  const IndexItem &item = m_index[index.row()];

  switch (role) {
  case TypeRole:
    return item.type;
  case HeaderTitleRole:
    return item.headerTitle;
  case RowStartIndexRole:
    return item.sourceStartIndex;
  case RowCountRole:
    return item.count;
  default:
    return QVariant();
  }
}

QHash<int, QByteArray> GroupedProxyModel::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[TypeRole] = "type";
  roles[HeaderTitleRole] = "headerTitle";
  roles[RowStartIndexRole] = "rowStartIndex";
  roles[RowCountRole] = "rowCount";
  return roles;
}

QModelIndex
GroupedProxyModel::getProxyIndexForSourceIndex(int sourceIndex) const {
  int row = getProxyRowForSourceIndex(sourceIndex);
  if (row >= 0) return index(row, 0);
  return QModelIndex();
}

int GroupedProxyModel::getProxyRowForSourceIndex(int sourceIndex) const {
  for (int i = 0; i < m_index.count(); ++i) {
    const IndexItem &item = m_index[i];
    if (item.type == RowItem) {
      if (sourceIndex >= item.sourceStartIndex &&
          sourceIndex < item.sourceStartIndex + item.count) {
        return i;
      }
    }
  }
  return -1;
}

void GroupedProxyModel::onSourceModelReset() { rebuildIndex(); }

void GroupedProxyModel::rebuildIndex() {
  beginResetModel();
  m_index.clear();

  if (!m_sourceModel || m_sourceModel->rowCount() == 0) {
    endResetModel();
    return;
  }

  int totalCount = m_sourceModel->rowCount();
  m_index.reserve(totalCount / m_columns + 100);

  ImageModel *im = qobject_cast<ImageModel *>(m_sourceModel);
  qint64 currentGroupKey = -999999999999LL;
  QString currentGroup = "";
  int currentRowStart = -1;
  int itemsInCurrentRow = 0;

  for (int i = 0; i < totalCount; ++i) {
    bool groupChanged = false;
    if (im) {
      qint64 key = im->getGroupKey(i, m_groupRole);
      if (key != currentGroupKey) {
        groupChanged = true;
        currentGroupKey = key;
        currentGroup = im->data(im->index(i, 0), m_groupRole).toString();
      }
    } else {
      QString group = m_sourceModel->data(m_sourceModel->index(i, 0), m_groupRole).toString();
      if (group != currentGroup) {
        groupChanged = true;
        currentGroup = group;
      }
    }

    // If group changed, finish current row and add header
    if (groupChanged) {
      if (itemsInCurrentRow > 0) {
        m_index.append({RowItem, "", currentRowStart, itemsInCurrentRow});
        itemsInCurrentRow = 0;
      }

      m_index.append({HeaderItem, currentGroup, -1, 0});
      currentRowStart = i;
    }

    // Add item to current row
    if (itemsInCurrentRow == 0) {
      currentRowStart = i;
    }
    itemsInCurrentRow++;

    // If row is full, push it
    if (itemsInCurrentRow >= m_columns) {
      m_index.append({RowItem, "", currentRowStart, itemsInCurrentRow});
      itemsInCurrentRow = 0;
    }
  }

  // Push remaining items
  if (itemsInCurrentRow > 0) {
    m_index.append({RowItem, "", currentRowStart, itemsInCurrentRow});
  }

  endResetModel();
}

QString GroupedProxyModel::getLabelForProxyIndex(int proxyIndex) const {
  if (proxyIndex < 0 || proxyIndex >= m_index.count())
    return "";

  const IndexItem &item = m_index[proxyIndex];

  // If Header, return title
  if (item.type == HeaderItem) {
    return item.headerTitle;
  }

  // If Row, get date of first item
  if (item.type == RowItem && m_sourceModel) {
    int sourceIndex = item.sourceStartIndex;
    if (sourceIndex >= 0 && sourceIndex < m_sourceModel->rowCount()) {
      QModelIndex srcIdx = m_sourceModel->index(sourceIndex, 0);

      // Show next level of detail
      int displayRole = m_groupRole;

      if (m_groupRole == ImageModel::SectionYearRole) {
        displayRole = ImageModel::SectionMonthRole;
      } else if (m_groupRole == ImageModel::SectionMonthRole) {
        displayRole = ImageModel::SectionDayRole;
      } else if (m_groupRole == ImageModel::SectionWeekRole) {
        displayRole = ImageModel::SectionDayRole;
      } else {
        displayRole = ImageModel::SectionDayRole;
      }

      return m_sourceModel->data(srcIdx, displayRole).toString();
    }
  }

  return "";
}
