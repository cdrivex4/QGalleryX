#include "FileCacheManager.h"
#include <QStandardPaths>
#include <QFile>
#include <QDataStream>
#include <QDebug>
#include <QSettings>
#include <algorithm>
#include <QCoreApplication>

// --- QHashCacheDatabase Implementation ---

void QHashCacheDatabase::load(const QString& dbPath) {
    QMutexLocker lock(&m_mutex);
    QFile file(dbPath);
    if (!file.open(QIODevice::ReadOnly))
        return;
        
    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_6_0);
    
    in >> m_db;
    
    m_totalBytes = 0;
    for (auto it = m_db.constBegin(); it != m_db.constEnd(); ++it) {
        m_totalBytes += it.value().fileSizeBytes;
    }
}

void QHashCacheDatabase::save(const QString& dbPath) {
    QMutexLocker lock(&m_mutex);
    QFile file(dbPath);
    // Write to a unique temporary file to avoid concurrent write corruption
    QString tmpPath = dbPath + ".tmp." + QString::number(QCoreApplication::applicationPid());
    QFile tempFile(tmpPath);
    if (!tempFile.open(QIODevice::WriteOnly))
        return;
        
    QDataStream out(&tempFile);
    out.setVersion(QDataStream::Qt_6_0);
    out << m_db;
    tempFile.close();
    
    if (file.exists()) file.remove();
    if (!tempFile.rename(dbPath)) {
        tempFile.remove(); // Cleanup if rename fails (e.g. locked by another instance)
    }
}

bool QHashCacheDatabase::contains(const QString& key) {
    QMutexLocker lock(&m_mutex);
    return m_db.contains(key);
}

CacheEntry QHashCacheDatabase::get(const QString& key) {
    QMutexLocker lock(&m_mutex);
    if (m_db.contains(key)) {
        // Update access time for LRU
        m_db[key].lastAccessed = QDateTime::currentMSecsSinceEpoch();
        return m_db[key];
    }
    return CacheEntry();
}

void QHashCacheDatabase::insert(const QString& key, const CacheEntry& entry) {
    QMutexLocker lock(&m_mutex);
    if (m_db.contains(key)) {
        m_totalBytes -= m_db[key].fileSizeBytes;
    }
    m_db.insert(key, entry);
    m_totalBytes += entry.fileSizeBytes;
}

void QHashCacheDatabase::remove(const QString& key) {
    QMutexLocker lock(&m_mutex);
    if (m_db.contains(key)) {
        m_totalBytes -= m_db[key].fileSizeBytes;
        m_db.remove(key);
    }
}

qint64 QHashCacheDatabase::totalSizeBytes() {
    QMutexLocker lock(&m_mutex);
    return m_totalBytes;
}

QList<QString> QHashCacheDatabase::getOldestKeys(int limit) {
    QMutexLocker lock(&m_mutex);
    QList<QPair<QString, qint64>> sortedList;
    sortedList.reserve(m_db.size());
    
    for (auto it = m_db.constBegin(); it != m_db.constEnd(); ++it) {
        sortedList.append({it.key(), it.value().lastAccessed});
    }
    
    std::sort(sortedList.begin(), sortedList.end(), [](const auto& a, const auto& b) {
        return a.second < b.second; // Smallest timestamp first (oldest)
    });
    
    QList<QString> result;
    for (int i = 0; i < qMin(limit, sortedList.size()); ++i) {
        result.append(sortedList[i].first);
    }
    return result;
}

void QHashCacheDatabase::clear() {
    QMutexLocker lock(&m_mutex);
    m_db.clear();
    m_totalBytes = 0;
}

// --- LmdbCacheDatabase Stub Implementation ---

void LmdbCacheDatabase::load(const QString& dbPath) { Q_UNUSED(dbPath); }
void LmdbCacheDatabase::save(const QString& dbPath) { Q_UNUSED(dbPath); }
bool LmdbCacheDatabase::contains(const QString& key) { Q_UNUSED(key); return false; }
CacheEntry LmdbCacheDatabase::get(const QString& key) { Q_UNUSED(key); return CacheEntry(); }
void LmdbCacheDatabase::insert(const QString& key, const CacheEntry& entry) { Q_UNUSED(key); Q_UNUSED(entry); }
void LmdbCacheDatabase::remove(const QString& key) { Q_UNUSED(key); }
qint64 LmdbCacheDatabase::totalSizeBytes() { return 0; }
QList<QString> LmdbCacheDatabase::getOldestKeys(int limit) { Q_UNUSED(limit); return QList<QString>(); }
void LmdbCacheDatabase::clear() {}

// --- FileCacheManager Implementation ---

FileCacheManager& FileCacheManager::instance() {
    static FileCacheManager instance;
    return instance;
}

FileCacheManager::FileCacheManager() : m_maxBytes(1024LL * 1024LL * 1024LL) /* 1GB default */ {
    QSettings settings("SamsungClone", "Gallery");
    int dbType = settings.value("diskCacheDatabaseType", 0).toInt();
    
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/thumbnails";
    QDir dir(cacheDir);
    if (!dir.exists()) dir.mkpath(".");

    // Test writability (e.g., if run from restrictive network share without user profile mapping)
    QFile testFile(cacheDir + "/.write_test");
    m_canWrite = testFile.open(QIODevice::WriteOnly);
    if (m_canWrite) {
        testFile.close();
        testFile.remove();
    } else {
        qWarning() << "FileCacheManager: Cannot write to CacheLocation. Disabling disk cache.";
        settings.setValue("useDiskCache", false);
    }

    if (dbType == 1) {
        m_db = std::make_unique<LmdbCacheDatabase>();
        m_dbPath = cacheDir + "/FileCache.lmdb";
        qDebug() << "FileCacheManager: Initialized with LMDB (Stub) at" << m_dbPath;
    } else {
        m_db = std::make_unique<QHashCacheDatabase>();
        m_dbPath = cacheDir + "/FileCache.db";
        qDebug() << "FileCacheManager: Initialized with Native QHash at" << m_dbPath;
    }
    
    m_maintenanceTimer = new QTimer(this);
    connect(m_maintenanceTimer, &QTimer::timeout, this, &FileCacheManager::performMaintenance);
}

FileCacheManager::~FileCacheManager() {
    if (m_dirty) {
        m_db->save(m_dbPath);
    }
}

void FileCacheManager::initialize() {
    m_db->load(m_dbPath);
    // Run maintenance every 60 seconds
    m_maintenanceTimer->start(60000);
}

void FileCacheManager::setMaxDiskCacheSizeMB(int megabytes) {
    m_maxBytes = static_cast<qint64>(megabytes) * 1024LL * 1024LL;
}

QString FileCacheManager::getCoalesceKey(const QString &id, const QSize &size) {
    return QString("%1_%2x%3").arg(id).arg(size.width()).arg(size.height());
}

QString FileCacheManager::getCachedPath(const QString& id, const QSize& requestedSize) {
    if (!m_canWrite || !m_db) return QString();
    QString key = getCoalesceKey(id, requestedSize);
    if (m_db->contains(key)) {
        CacheEntry entry = m_db->get(key);
        // "detect and rebuild" - verify the file actually exists on OS disk before returning it
        if (QFile::exists(entry.thumbPath)) {
            m_dirty = true; // access time was updated
            return entry.thumbPath;
        } else {
            // Rebuild: The file was deleted outside the app, so drop it from the index
            m_db->remove(key);
            m_dirty = true;
        }
    }
    return QString();
}

void FileCacheManager::registerCacheFile(const QString& id, const QSize& requestedSize, const QString& thumbPath, qint64 sizeBytes) {
    if (!m_canWrite || !m_db) return;
    QString key = getCoalesceKey(id, requestedSize);
    
    CacheEntry entry;
    entry.originalPath = id;
    entry.sizeKey = QString("%1x%2").arg(requestedSize.width()).arg(requestedSize.height());
    entry.thumbPath = thumbPath;
    entry.lastAccessed = QDateTime::currentMSecsSinceEpoch();
    entry.fileSizeBytes = sizeBytes;
    
    m_db->insert(key, entry);
    m_dirty = true;
}

void FileCacheManager::performMaintenance() {
    if (!m_canWrite || !m_db) return;
    if (m_db->totalSizeBytes() > m_maxBytes) {
        qint64 bytesToFree = m_db->totalSizeBytes() - (m_maxBytes * 0.8); // Free down to 80%
        
        QList<QString> oldestKeys = m_db->getOldestKeys(1000); // Check in batches
        for (const QString& key : oldestKeys) {
            if (bytesToFree <= 0) break;
            
            CacheEntry entry = m_db->get(key); // We can just peek because we're about to delete
            QFile::remove(entry.thumbPath);
            bytesToFree -= entry.fileSizeBytes;
            m_db->remove(key);
        }
        m_dirty = true;
    }
    
    if (m_dirty) {
        m_db->save(m_dbPath);
        m_dirty = false;
    }
}

void FileCacheManager::clearCache() {
    m_db->clear();
    m_dirty = true;
    m_db->save(m_dbPath);
    
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/thumbnails";
    QDir dir(cacheDir);
    if (dir.exists()) {
        dir.removeRecursively();
    }
    QDir().mkpath(cacheDir);
}
