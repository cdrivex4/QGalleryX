import re

with open('src_legacy/ImageModel.cpp', 'r') as f:
    content = f.read()

# 1. Add QStorageInfo
content = content.replace('#include <QStandardPaths>', '#include <QStandardPaths>\n#include <QStorageInfo>')

# 2. Add m_allItems clearing
content = content.replace('  beginResetModel();\n  m_images.clear();\n  endResetModel();', '  beginResetModel();\n  m_allItems.clear();\n  m_images.clear();\n  endResetModel();')

# 3. Add network detection
scan_target = '        qDebug() << "Scanning directory:" << cleanPath;\n\n        QDirIterator it(cleanPath,'
scan_replace = '''        qDebug() << "Scanning directory:" << cleanPath;

        bool isNetworkPath = false;
        if (cleanPath.startsWith("\\\\\\\\")) {
          isNetworkPath = true;
          qDebug() << "[NetworkScan] Detected UNC path:" << cleanPath;
        } else if (cleanPath.length() >= 3 && cleanPath[1] == ':') {
          QStorageInfo storage(cleanPath);
          if (storage.isValid() && storage.device().startsWith("\\\\\\\\")) {
            isNetworkPath = true;
            qDebug() << "[NetworkScan] Detected mapped network drive:" << cleanPath;
          }
        }

        QDirIterator it(cleanPath,'''
content = content.replace(scan_target, scan_replace)

# 4. Modify batch insert
batch_target = '''          if (batch.size() >= BATCH_SIZE) {
            QMetaObject::invokeMethod(this, [this, batch]() {
              beginInsertRows(QModelIndex(), m_images.count(),
                              m_images.count() + batch.count() - 1);
              m_images.append(batch);
              endInsertRows();
            });
            batch.clear();
          }'''
batch_replace = '''          if (batch.size() >= BATCH_SIZE) {
            QMetaObject::invokeMethod(this, [this, batch, isNetworkPath]() {
              m_allItems.append(batch);
              if (!isNetworkPath) {
                if (m_filterQuery.isEmpty()) {
                  beginInsertRows(QModelIndex(), m_images.count(),
                                  m_images.count() + batch.count() - 1);
                  m_images.append(batch);
                  endInsertRows();
                } else {
                  applyFilter();
                }
              }
            });
            batch.clear();
          }'''
content = content.replace(batch_target, batch_replace)

# 5. Modify remaining append
rem_target = '''        if (!batch.isEmpty()) {
          QMetaObject::invokeMethod(this, [this, batch]() {
            beginInsertRows(QModelIndex(), m_images.count(),
                            m_images.count() + batch.count() - 1);
            m_images.append(batch);
            endInsertRows();
          });
        }'''
rem_replace = '''        if (!batch.isEmpty()) {
          QMetaObject::invokeMethod(this, [this, batch]() {
            m_allItems.append(batch);
            if (m_filterQuery.isEmpty()) {
              beginInsertRows(QModelIndex(), m_images.count(),
                              m_images.count() + batch.count() - 1);
              m_images.append(batch);
              endInsertRows();
            } else {
              applyFilter();
            }
          });
        }'''
content = content.replace(rem_target, rem_replace)

# 6. Modify final sort
sort_target = '''        QMetaObject::invokeMethod(this, [this, timer]() {
          beginResetModel();
          std::sort(m_images.begin(), m_images.end(),
                    [](const ImageInfo &a, const ImageInfo &b) {
                      return a.date > b.date;
                    });
          endResetModel();'''
sort_replace = '''        QMetaObject::invokeMethod(this, [this, timer]() {
          std::sort(m_allItems.begin(), m_allItems.end(),
                    [](const ImageInfo &a, const ImageInfo &b) {
                      return a.date > b.date;
                    });
          applyFilter();'''
content = content.replace(sort_target, sort_replace)

# 7. Add filter functions
filters = '''
void ImageModel::setFilterQuery(const QString &query) {
  if (m_filterQuery != query) {
    m_filterQuery = query;
    emit filterQueryChanged();
    applyFilter();
  }
}

void ImageModel::applyFilter() {
  beginResetModel();
  if (m_filterQuery.isEmpty()) {
    m_images = m_allItems;
  } else {
    m_images.clear();
    QString lowerQuery = m_filterQuery.toLower();
    for (const auto &item : m_allItems) {
      if (item.fileName.toLower().contains(lowerQuery)) {
        m_images.append(item);
      }
    }
  }
  endResetModel();
}
'''
content = content + filters

with open('src_legacy/ImageModel.cpp', 'w') as f:
    f.write(content)
print('Done!')
