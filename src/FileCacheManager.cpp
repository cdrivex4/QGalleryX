#include "FileCacheManager.h"
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <QUrl>
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
    
    struct KeyDate {
        QString key;
        qint64 date;
    };
    QList<KeyDate> sorted;
    sorted.reserve(m_db.size());
    
    for (auto it = m_db.begin(); it != m_db.end(); ++it) {
        sorted.append({it.key(), it.value().lastAccessed});
    }
    
    std::sort(sorted.begin(), sorted.end(), [](const KeyDate& a, const KeyDate& b) {
        return a.date < b.date;
    });
    
    QList<QString> result;
    for (int i = 0; i < qMin(limit, sorted.size()); ++i) {
        result.append(sorted[i].key);
    }
    return result;
}

QList<QString> QHashCacheDatabase::getAllKeys() {
    QMutexLocker lock(&m_mutex);
    return m_db.keys();
}

void QHashCacheDatabase::clear() {
    QMutexLocker lock(&m_mutex);
    m_db.clear();
    m_totalBytes = 0;
}

// --- MmapCacheDatabase Implementation ---

MmapCacheDatabase::MmapCacheDatabase() : m_capacity(0) {}

MmapCacheDatabase::~MmapCacheDatabase() {
    if (m_mappedData) {
        m_file.unmap(m_mappedData);
    }
    if (m_file.isOpen()) {
        m_file.close();
    }
}

void MmapCacheDatabase::load(const QString& dbPath) {
    QMutexLocker lock(&m_mutex);
    m_file.setFileName(dbPath);
    
    // Create if not exists or if we can't open read/write
    if (!m_file.open(QIODevice::ReadWrite)) {
        qWarning() << "Failed to open mmap cache file:" << dbPath;
        return;
    }

    m_capacity = 1024LL * 1024LL * 1024LL; // 1GB fixed capacity for ring buffer
    if (m_file.size() < m_capacity) {
        m_file.resize(m_capacity);
    }

    m_mappedData = m_file.map(0, m_capacity);
    if (!m_mappedData) {
        qWarning() << "Failed to mmap cache file!";
        m_file.close();
        return;
    }

    RingHeader* header = reinterpret_cast<RingHeader*>(m_mappedData);
    if (header->magic != 0x4D4D4150) { // 'MMAP'
        // Initialize new ring buffer
        header->magic = 0x4D4D4150;
        header->head = sizeof(RingHeader);
        header->tail = sizeof(RingHeader);
        header->capacity = m_capacity;
    }

    // Rebuild index by walking the ring buffer from tail to head
    quint64 current = header->tail;
    while (current != header->head) {
        if (current >= m_capacity - sizeof(RecordHeader)) {
            current = sizeof(RingHeader); // Wrap
            continue;
        }
        
        RecordHeader* rec = reinterpret_cast<RecordHeader*>(m_mappedData + current);
        if (rec->keyLen == 0xFFFFFFFF) {
            current = sizeof(RingHeader); // Wrap marker found
            continue;
        }
        if (rec->keyLen == 0 || rec->keyLen > 1024 || rec->dataLen > 1024*1024*10) {
            // Corruption detected, reset ring
            header->head = sizeof(RingHeader);
            header->tail = sizeof(RingHeader);
            m_index.clear();
            m_offsets.clear();
            break;
        }

        quint64 nextCurrent = current + sizeof(RecordHeader) + rec->keyLen + rec->dataLen;
        if (nextCurrent > m_capacity) {
            current = sizeof(RingHeader); // Wrap
            continue;
        }

        QString key = QString::fromUtf8(reinterpret_cast<const char*>(m_mappedData + current + sizeof(RecordHeader)), rec->keyLen);
        
        CacheEntry entry;
        entry.fileSizeBytes = rec->dataLen;
        m_index.insert(key, entry);
        m_offsets.insert(key, current);

        current = nextCurrent;
    }
}

void MmapCacheDatabase::save(const QString& dbPath) { Q_UNUSED(dbPath); } // Synced automatically via OS pages

bool MmapCacheDatabase::contains(const QString& key) {
    QMutexLocker lock(&m_mutex);
    return m_index.contains(key);
}

CacheEntry MmapCacheDatabase::get(const QString& key) {
    QMutexLocker lock(&m_mutex);
    return m_index.value(key);
}

void MmapCacheDatabase::insert(const QString& key, const CacheEntry& entry) { Q_UNUSED(key); Q_UNUSED(entry); }
void MmapCacheDatabase::remove(const QString& key) {
    QMutexLocker lock(&m_mutex);
    m_index.remove(key);
    m_offsets.remove(key);
}

qint64 MmapCacheDatabase::totalSizeBytes() { return 0; } // Managed by ring capacity
QList<QString> MmapCacheDatabase::getOldestKeys(int limit) { Q_UNUSED(limit); return {}; }
QList<QString> MmapCacheDatabase::getAllKeys() {
    QMutexLocker lock(&m_mutex);
    return m_index.keys();
}
void MmapCacheDatabase::clear() {
    QMutexLocker lock(&m_mutex);
    clearInternal();
}

void MmapCacheDatabase::clearInternal() {
    if (m_mappedData) {
        RingHeader* header = reinterpret_cast<RingHeader*>(m_mappedData);
        header->head = sizeof(RingHeader);
        header->tail = sizeof(RingHeader);
    }
    m_index.clear();
    m_offsets.clear();
}

QByteArray MmapCacheDatabase::getRawData(const QString& key) {
    QMutexLocker lock(&m_mutex);
    if (!m_mappedData || !m_offsets.contains(key)) return QByteArray();
    
    quint64 offset = m_offsets.value(key);
    if (offset + sizeof(RecordHeader) > m_capacity) return QByteArray(); // Bounds check
    
    RecordHeader* rec = reinterpret_cast<RecordHeader*>(m_mappedData + offset);
    
    if (rec->keyLen > 1024 || rec->dataLen > 1024*1024*50) return QByteArray(); // Sanity check
    if (offset + sizeof(RecordHeader) + rec->keyLen + rec->dataLen > m_capacity) return QByteArray(); // Bounds check
    
    // Deep copy to prevent corruption if another thread wraps the ring buffer during decode
    return QByteArray(reinterpret_cast<const char*>(m_mappedData + offset + sizeof(RecordHeader) + rec->keyLen), rec->dataLen);
}

bool MmapCacheDatabase::advanceHead(quint64 requiredSize) {
    if (!m_mappedData) return false;
    RingHeader* header = reinterpret_cast<RingHeader*>(m_mappedData);
    
    if (m_index.isEmpty()) {
        header->head = sizeof(RingHeader);
        header->tail = sizeof(RingHeader);
    }
    
    // Check if we need to wrap head to start
    if (header->head + requiredSize > m_capacity) {
        if (header->head + sizeof(RecordHeader) <= m_capacity) {
            RecordHeader* wrapMarker = reinterpret_cast<RecordHeader*>(m_mappedData + header->head);
            wrapMarker->keyLen = 0xFFFFFFFF; // Special wrap marker
            wrapMarker->dataLen = 0;
        }
        header->head = sizeof(RingHeader); // Wrap head
    }
    
    // Evict old tail entries while head + requiredSize overlaps tail
    while (!m_index.isEmpty() && header->head <= header->tail && (header->head + requiredSize) > header->tail) {
        if (header->tail >= m_capacity - sizeof(RecordHeader)) {
            header->tail = sizeof(RingHeader);
            continue;
        }
        RecordHeader* tailRec = reinterpret_cast<RecordHeader*>(m_mappedData + header->tail);
        if (tailRec->keyLen == 0xFFFFFFFF) {
            header->tail = sizeof(RingHeader);
            continue;
        }
        if (tailRec->keyLen > 1024 || tailRec->dataLen > 1024*1024*50 || header->tail + sizeof(RecordHeader) + tailRec->keyLen > m_capacity) {
            clearInternal();
            return true;
        }
        QString tailKey = QString::fromUtf8(reinterpret_cast<const char*>(m_mappedData + header->tail + sizeof(RecordHeader)), tailRec->keyLen);
        m_index.remove(tailKey);
        m_offsets.remove(tailKey);
        
        quint64 nextTail = header->tail + sizeof(RecordHeader) + tailRec->keyLen + tailRec->dataLen;
        if (nextTail > m_capacity) header->tail = sizeof(RingHeader);
        else header->tail = nextTail;
    }
    
    return true;
}

void MmapCacheDatabase::insertRawData(const QString& key, const QString& originalPath, const QString& sizeKey, const QByteArray& data) {
    QMutexLocker lock(&m_mutex);
    if (!m_mappedData) return;
    
    QByteArray keyBytes = key.toUtf8();
    quint64 totalRequired = sizeof(RecordHeader) + keyBytes.size() + data.size();
    
    if (totalRequired > m_capacity / 2) return; // Too large for ring
    
    RingHeader* header = reinterpret_cast<RingHeader*>(m_mappedData);
    advanceHead(totalRequired);
    
    quint64 offset = header->head;
    RecordHeader* rec = reinterpret_cast<RecordHeader*>(m_mappedData + offset);
    rec->keyLen = keyBytes.size();
    rec->dataLen = data.size();
    
    memcpy(m_mappedData + offset + sizeof(RecordHeader), keyBytes.constData(), rec->keyLen);
    memcpy(m_mappedData + offset + sizeof(RecordHeader) + rec->keyLen, data.constData(), rec->dataLen);
    
    CacheEntry entry;
    entry.originalPath = originalPath;
    entry.sizeKey = sizeKey;
    entry.fileSizeBytes = data.size();
    entry.lastAccessed = QDateTime::currentMSecsSinceEpoch();
    
    m_index.insert(key, entry);
    m_offsets.insert(key, offset);
    
    header->head += totalRequired;
}

// --- FileCacheManager Implementation ---

FileCacheManager& FileCacheManager::instance() {
    static FileCacheManager instance;
    return instance;
}

FileCacheManager::FileCacheManager() : m_maxBytes(1024LL * 1024LL * 1024LL) /* 1GB default */ {
    QSettings settings("SamsungClone", "Gallery");
    int dbType = settings.value("diskCacheDatabaseType", 1).toInt();
    
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
        m_db = std::make_unique<MmapCacheDatabase>();
        m_dbPath = cacheDir + "/FileCache.mmap";
        qDebug() << "FileCacheManager: Initialized with Memory-Mapped Ring Buffer at" << m_dbPath;
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
    MmapCacheDatabase* mmapDb = dynamic_cast<MmapCacheDatabase*>(m_db.get());
    if (mmapDb && !mmapDb->isMapped()) {
        qWarning() << "FileCacheManager: Mmap failed to allocate/map memory. Falling back to QHashCacheDatabase.";
        m_db = std::make_unique<QHashCacheDatabase>();
        m_dbPath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/thumbnails/FileCache.db";
        m_db->load(m_dbPath);
    }
    rebuildKeyIndex();
    // Run maintenance every 60 seconds
    m_maintenanceTimer->start(60000);
}

void FileCacheManager::rebuildKeyIndex() {
    QMutexLocker lock(&m_keyMutex);
    m_knownKeys.clear();
    if (!m_db) return;
    
    QList<QString> keys = m_db->getAllKeys();
    for (const QString& k : keys) {
        m_knownKeys.insert(k);
    }
}

void FileCacheManager::setMaxDiskCacheSizeMB(int megabytes) {
    m_maxBytes = static_cast<qint64>(megabytes) * 1024LL * 1024LL;
}

QString FileCacheManager::getCoalesceKey(const QString &id, const QSize &size) {
    // Strip query strings if present (e.g. from AsyncImageProvider)
    QString realPath = id;
    int qIdx = realPath.indexOf('?');
    if (qIdx != -1) realPath = realPath.left(qIdx);
    
    // Convert QUrl file:/// paths to native OS file paths
    QUrl url(realPath);
    if (url.isValid() && url.isLocalFile()) {
        realPath = url.toLocalFile();
    }
    realPath = QDir::toNativeSeparators(realPath);
    
    QFileInfo fi(realPath);
    if (!fi.exists()) {
        // Fallback for non-local files or virtual paths
        return QString("%1_%2x%3").arg(id).arg(size.width()).arg(size.height());
    }
    
    qint64 modTime = fi.lastModified().isValid() ? fi.lastModified().toMSecsSinceEpoch() : 0;
    
    return QString("%1_%2_%3_%4x%5")
        .arg(fi.fileName())
        .arg(fi.size())
        .arg(modTime)
        .arg(size.width())
        .arg(size.height());
}

QString FileCacheManager::getCachedPath(const QString& id, const QSize& requestedSize) {
    if (!m_canWrite || !m_db) return QString();
    QString key = getCoalesceKey(id, requestedSize);
    
    {
        QMutexLocker lock(&m_keyMutex);
        if (!m_knownKeys.isEmpty() && !m_knownKeys.contains(key)) {
            // Fast O(1) reject — key not in DB index
            return QString();
        }
    }

    if (m_db->contains(key)) {
        CacheEntry entry = m_db->get(key);
        // "detect and rebuild" - verify the file actually exists on OS disk before returning it
        if (QFile::exists(entry.thumbPath)) {
            m_dirty = true; // access time was updated
            return entry.thumbPath;
        } else {
            // Rebuild: The file was deleted outside the app, so drop it from the index
            m_db->remove(key);
            {
                QMutexLocker lock(&m_keyMutex);
                m_knownKeys.remove(key);
            }
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
    {
        QMutexLocker lock(&m_keyMutex);
        m_knownKeys.insert(key);
    }
    m_dirty = true;
}

QByteArray FileCacheManager::getCachedData(const QString& id, const QSize& requestedSize) {
    if (!m_canWrite || !m_db) return QByteArray();
    
    // Only MmapCacheDatabase supports this direct extraction
    MmapCacheDatabase* mmapDb = dynamic_cast<MmapCacheDatabase*>(m_db.get());
    if (mmapDb) {
        QString key = getCoalesceKey(id, requestedSize);
        return mmapDb->getRawData(key);
    }
    return QByteArray();
}

void FileCacheManager::registerCachedData(const QString& id, const QSize& requestedSize, const QByteArray& data) {
    if (!m_canWrite || !m_db) return;
    
    MmapCacheDatabase* mmapDb = dynamic_cast<MmapCacheDatabase*>(m_db.get());
    if (mmapDb) {
        QString key = getCoalesceKey(id, requestedSize);
        QString sizeKey = QString("%1x%2").arg(requestedSize.width()).arg(requestedSize.height());
        mmapDb->insertRawData(key, id, sizeKey, data);
        {
            QMutexLocker lock(&m_keyMutex);
            m_knownKeys.insert(key);
        }
        m_dirty = true;
    }
}

void FileCacheManager::performMaintenance() {
    if (!m_db) return;
    
    qint64 currentSize = m_db->totalSizeBytes();
    if (currentSize <= m_maxBytes) return; // No pruning needed
    
    qint64 bytesToFree = currentSize - (m_maxBytes * 0.8); // Free down to 80%
    QList<QString> oldKeys = m_db->getOldestKeys(100);
    
    for (const QString& key : oldKeys) {
        if (bytesToFree <= 0) break;
        CacheEntry entry = m_db->get(key);
        if (QFile::exists(entry.thumbPath)) {
            QFile::remove(entry.thumbPath);
            bytesToFree -= entry.fileSizeBytes;
            m_db->remove(key);
            {
                QMutexLocker lock(&m_keyMutex);
                m_knownKeys.remove(key);
            }
        }
        m_dirty = true;
    }
    
    if (m_dirty) {
        m_db->save(m_dbPath);
    }
}

void FileCacheManager::clearCache() {
    if (m_db) {
        m_db->clear();
    }
    {
        QMutexLocker lock(&m_keyMutex);
        m_knownKeys.clear();
    }
    m_dirty = true;
    if (m_db) {
        m_db->save(m_dbPath);
    }
    
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/thumbnails";
    QDir dir(cacheDir);
    if (dir.exists()) {
        dir.removeRecursively();
    }
    QDir().mkpath(cacheDir);
}

void FileCacheManager::nukeCache() {
    qInfo() << "[FileCacheManager] NUKE CACHE requested by user. Wiping all database files and thumbnails...";
    
    if (m_db) m_db->clear();
    {
        QMutexLocker lock(&m_keyMutex);
        m_knownKeys.clear();
    }
    m_dirty = false;

    QFile::remove(m_dbPath);
    QFile::remove(m_dbPath + ".tmp");
    
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/thumbnails";
    QDir dir(cacheDir);
    if (dir.exists()) dir.removeRecursively();
    QDir().mkpath(cacheDir);

    if (m_db) m_db->load(m_dbPath);
    qInfo() << "[FileCacheManager] Nuke Cache complete.";
}
