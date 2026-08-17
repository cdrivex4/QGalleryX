#include "FileCacheManager.h"
#include <QStandardPaths>
#include <QFile>
#include <QDataStream>
#include <QDebug>
#include <QSettings>
#include <QCryptographicHash>
#include <QUrl>
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

static constexpr quint64 MMAP_GROW_CHUNK = 512LL * 1024LL * 1024LL; // Grow by 512MB at a time
static constexpr quint32 MMAP_MAGIC   = 0x4D4D4150; // 'MMAP'
static constexpr quint32 MMAP_VERSION = 3;           // Append-only log

MmapCacheDatabase::MmapCacheDatabase() : m_capacity(0) {}

MmapCacheDatabase::~MmapCacheDatabase() {
    if (m_mappedData) {
        m_file.unmap(m_mappedData);
    }
    if (m_file.isOpen()) {
        m_file.close();
    }
}

bool MmapCacheDatabase::growFile(quint64 newCapacity) {
    // Unmap current region, resize file, remap.
    if (m_mappedData) {
        m_file.unmap(m_mappedData);
        m_mappedData = nullptr;
    }
    if (!m_file.resize(newCapacity)) {
        qWarning() << "[MmapCacheDatabase] Failed to grow file to" << newCapacity / (1024*1024) << "MB";
        return false;
    }
    m_mappedData = m_file.map(0, newCapacity);
    if (!m_mappedData) {
        qWarning() << "[MmapCacheDatabase] Failed to remap after grow!";
        m_file.close();
        return false;
    }
    m_capacity = newCapacity;
    reinterpret_cast<RingHeader*>(m_mappedData)->capacity = newCapacity;
    qInfo() << "[MmapCacheDatabase] Grown to" << newCapacity / (1024*1024) << "MB";
    return true;
}

void MmapCacheDatabase::load(const QString& dbPath) {
    QMutexLocker lock(&m_mutex);
    m_file.setFileName(dbPath);

    QFileInfo fi(dbPath);
    QDir().mkpath(fi.absolutePath());

    if (!m_file.open(QIODevice::ReadWrite)) {
        qWarning() << "[MmapCacheDatabase] Failed to open:" << dbPath;
        return;
    }

    // Start with at least one grow-chunk mapped
    quint64 existingSize = (quint64)m_file.size();
    m_capacity = qMax(existingSize, MMAP_GROW_CHUNK);
    if ((quint64)m_file.size() < m_capacity) {
        m_file.resize(m_capacity);
    }

    m_mappedData = m_file.map(0, m_capacity);
    if (!m_mappedData) {
        qWarning() << "[MmapCacheDatabase] Failed to mmap:" << dbPath;
        m_file.close();
        return;
    }

    RingHeader* header = reinterpret_cast<RingHeader*>(m_mappedData);

    // Detect old v2 ring-buffer file or corrupt header — re-initialise cleanly.
    // This migrates the old 1GB ring to the new append-only format.
    if (header->magic != MMAP_MAGIC || header->version != MMAP_VERSION ||
        header->head < sizeof(RingHeader) || header->head > m_capacity) {

        qInfo() << "[MmapCacheDatabase] Initialising append-only log (v3) at" << dbPath
                << "(was version" << header->version << ")";
        header->magic      = MMAP_MAGIC;
        header->version    = MMAP_VERSION;
        header->head       = sizeof(RingHeader);
        header->capacity   = m_capacity;
        header->entryCount = 0;
        header->_reserved  = 0;
        if (m_capacity > sizeof(RingHeader) + sizeof(RecordHeader))
            memset(m_mappedData + sizeof(RingHeader), 0, sizeof(RecordHeader));
        m_index.clear();
        m_offsets.clear();
        return;
    }

    // Walk forward from the start of the data region to head, rebuilding the index.
    quint64 current = sizeof(RingHeader);
    quint64 writeHead = header->head;
    int restored = 0;

    while (current < writeHead) {
        if (current + sizeof(RecordHeader) > writeHead) break;

        RecordHeader* rec = reinterpret_cast<RecordHeader*>(m_mappedData + current);
        if (rec->keyLen == 0 || rec->keyLen > 2048 ||
            rec->dataLen == 0 || rec->dataLen > 1024*1024*50 ||
            current + sizeof(RecordHeader) + rec->keyLen + rec->dataLen > writeHead) {
            // Corrupt / unwritten boundary — head must have advanced past real data
            header->head = current;
            break;
        }

        QString key = QString::fromUtf8(
            reinterpret_cast<const char*>(m_mappedData + current + sizeof(RecordHeader)),
            rec->keyLen);

        CacheEntry entry;
        entry.fileSizeBytes = rec->dataLen;
        entry.lastAccessed  = QDateTime::currentMSecsSinceEpoch();
        // Newer entry overwrites older one (deduplication at load time)
        m_index.insert(key, entry);
        m_offsets.insert(key, current);
        restored++;

        current += sizeof(RecordHeader) + rec->keyLen + rec->dataLen;
    }

    header->entryCount = restored;
    qInfo() << "[MmapCacheDatabase] Restored" << restored << "entries from"
            << dbPath << "| Write head at" << writeHead / (1024*1024) << "MB";
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
        header->magic      = MMAP_MAGIC;
        header->version    = MMAP_VERSION;
        header->head       = sizeof(RingHeader);
        header->capacity   = m_capacity;
        header->entryCount = 0;
        header->_reserved  = 0;
        if (m_capacity > sizeof(RingHeader) + sizeof(RecordHeader))
            memset(m_mappedData + sizeof(RingHeader), 0, sizeof(RecordHeader));
    }
    m_index.clear();
    m_offsets.clear();
}

QByteArray MmapCacheDatabase::getRawData(const QString& key) {
    QMutexLocker lock(&m_mutex);
    if (!m_mappedData || !m_offsets.contains(key)) return QByteArray();

    quint64 offset = m_offsets.value(key);
    if (offset + sizeof(RecordHeader) > m_capacity) {
        m_offsets.remove(key);
        m_index.remove(key);
        return QByteArray();
    }

    RecordHeader* rec = reinterpret_cast<RecordHeader*>(m_mappedData + offset);
    if (rec->keyLen == 0 || rec->keyLen > 2048 || rec->dataLen == 0 || rec->dataLen > 1024*1024*50 ||
        offset + sizeof(RecordHeader) + rec->keyLen + rec->dataLen > m_capacity) {
        m_offsets.remove(key);
        m_index.remove(key);
        return QByteArray();
    }

    // Zero-copy slice directly from the mmap page. OS page cache handles RAM residency.
    return QByteArray(reinterpret_cast<const char*>(
        m_mappedData + offset + sizeof(RecordHeader) + rec->keyLen), rec->dataLen);
}

// advanceHead() removed — append-only log never evicts.


void MmapCacheDatabase::insertRawData(const QString& key, const QString& originalPath, const QString& sizeKey, const QByteArray& data) {
    QMutexLocker lock(&m_mutex);
    if (!m_mappedData) return;

    // Skip duplicates — already stored permanently at existing offset
    if (m_offsets.contains(key)) return;

    QByteArray keyBytes = key.toUtf8();
    quint64 totalRequired = sizeof(RecordHeader) + keyBytes.size() + data.size();

    RingHeader* header = reinterpret_cast<RingHeader*>(m_mappedData);

    // Grow the file if this record won't fit in the remaining space
    if (header->head + totalRequired > m_capacity) {
        quint64 newCapacity = m_capacity + MMAP_GROW_CHUNK;
        if (!growFile(newCapacity)) return; // Out of disk space?
        header = reinterpret_cast<RingHeader*>(m_mappedData); // Re-read after remap
    }

    quint64 offset = header->head;
    RecordHeader* rec = reinterpret_cast<RecordHeader*>(m_mappedData + offset);
    rec->keyLen  = keyBytes.size();
    rec->dataLen = data.size();

    memcpy(m_mappedData + offset + sizeof(RecordHeader),                 keyBytes.constData(), rec->keyLen);
    memcpy(m_mappedData + offset + sizeof(RecordHeader) + rec->keyLen,  data.constData(),     rec->dataLen);

    CacheEntry entry;
    entry.originalPath  = originalPath;
    entry.sizeKey       = sizeKey;
    entry.fileSizeBytes = data.size();
    entry.lastAccessed  = QDateTime::currentMSecsSinceEpoch();

    m_index.insert(key, entry);
    m_offsets.insert(key, offset);

    header->head += totalRequired;
    header->entryCount++;
}

// --- FileCacheManager Implementation ---

FileCacheManager& FileCacheManager::instance() {
    static FileCacheManager instance;
    return instance;
}

QString FileCacheManager::getCacheDirectory() {
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (cacheDir.isEmpty()) {
        cacheDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/QGalleryX";
    }
    QString thumbDir = cacheDir + "/thumbnails";
    QDir().mkpath(thumbDir);
    return thumbDir;
}

FileCacheManager::FileCacheManager() : m_maxBytes(1024LL * 1024LL * 1024LL) /* 1GB default */ {
    QString cacheDir = getCacheDirectory();
    QDir().mkpath(cacheDir);

    // Test writability
    QFile testFile(cacheDir + "/.write_test");
    m_canWrite = testFile.open(QIODevice::WriteOnly);
    if (m_canWrite) {
        testFile.close();
        testFile.remove();
    } else {
        // Fallback to temp folder if AppData is restricted
        cacheDir = QDir::tempPath() + "/QGalleryX/thumbnails";
        QDir().mkpath(cacheDir);
        QFile tempTest(cacheDir + "/.write_test");
        m_canWrite = tempTest.open(QIODevice::WriteOnly);
        if (m_canWrite) {
            tempTest.close();
            tempTest.remove();
        }
    }

    // Always use high-performance MmapCacheDatabase (1GB ring buffer)
    m_db = std::make_unique<MmapCacheDatabase>();
    m_dbPath = cacheDir + "/FileCache.mmap";
    qDebug() << "FileCacheManager: Initialized with Memory-Mapped Ring Buffer at" << m_dbPath << "(Writability:" << m_canWrite << ")";

    m_maintenanceTimer = new QTimer(this);
    connect(m_maintenanceTimer, &QTimer::timeout, this, &FileCacheManager::performMaintenance);
}

FileCacheManager::~FileCacheManager() {
    if (m_dirty) {
        m_db->save(m_dbPath);
    }
}

QString FileCacheManager::getStatsIniPath() const {
    QString cacheDir = FileCacheManager::getCacheDirectory();
    return cacheDir + "/cache_stats.ini";
}

void FileCacheManager::initialize() {
    m_db->load(m_dbPath);
    
    // Load persisted root stats across all cached drives from local cache_stats.ini (Zero Registry)
    {
        QSettings settings(getStatsIniPath(), QSettings::IniFormat);
        QStringList groups = settings.childGroups();
        QMutexLocker lock(&m_rootStatsMutex);
        for (const QString& root : groups) {
            settings.beginGroup(root);
            qint64 c = settings.value("count", 0).toLongLong();
            qint64 b = settings.value("bytes", 0).toLongLong();
            settings.endGroup();
            if (c > 0) {
                m_rootStats[root].count = c;
                m_rootStats[root].bytes = b;
            }
        }
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
        CacheEntry entry = m_db->get(k);
        if (!entry.originalPath.isEmpty()) {
            trackRootStat(entry.originalPath, entry.fileSizeBytes);
        }
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

    realPath = QUrl::fromPercentEncoding(realPath.toUtf8());

    // Canonicalize file:/// URIs into raw local filesystem paths
    if (realPath.startsWith("file:", Qt::CaseInsensitive)) {
        QUrl url(realPath);
        if (url.isLocalFile()) {
            realPath = url.toLocalFile();
        } else if (realPath.startsWith("file:///", Qt::CaseInsensitive)) {
            realPath = realPath.mid(8);
        } else if (realPath.startsWith("file://", Qt::CaseInsensitive)) {
            realPath = realPath.mid(7);
        } else if (realPath.startsWith("file:", Qt::CaseInsensitive)) {
            realPath = realPath.mid(5);
        }
    }

    if (realPath.startsWith("/") && realPath.length() > 2 && realPath[2] == ':') {
        realPath = realPath.mid(1);
    }

    realPath = QDir::cleanPath(realPath);
    realPath = QDir::toNativeSeparators(realPath).toLower();

    // Fast CPU-only MD5 hash of normalized file path — ZERO network SMB roundtrips!
    QString pathHash = QString::fromUtf8(QCryptographicHash::hash(realPath.toUtf8(), QCryptographicHash::Md5).toHex());

    // Single unified thumbnail bucket ONLY for grid/overview/semantic thumbnail sizes (width & height <= 384px).
    // Full photo viewer requests (size is invalid, empty, or >384px) MUST NOT return _thumb
    // so they decode the full-resolution original image!
    if (size.isValid() && !size.isEmpty() && size.width() <= 384 && size.height() <= 384) {
        return QString("%1_thumb").arg(pathHash);
    }

    if (size.isValid() && !size.isEmpty()) {
        return QString("%1_%2x%3")
            .arg(pathHash)
            .arg(size.width())
            .arg(size.height());
    }

    // Full-resolution original photo viewer request
    return QString("%1_full").arg(pathHash);
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
    trackRootStat(id, sizeBytes);
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

bool FileCacheManager::isCached(const QString& id, const QSize& requestedSize) {
    if (!m_canWrite || !m_db) return false;
    QString key = getCoalesceKey(id, requestedSize);
    // O(1) — just checks the in-memory index, zero disk I/O
    QMutexLocker lock(&m_keyMutex);
    return m_knownKeys.contains(key);
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
        trackRootStat(id, data.size());
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
    {
        QMutexLocker lock(&m_rootStatsMutex);
        m_rootStats.clear();
        QSettings settings(getStatsIniPath(), QSettings::IniFormat);
        settings.clear();
    }
    m_dirty = false;
    
    emit cacheCleared();
    qInfo() << "[FileCacheManager] Thumbnail cache cleared.";
}

void FileCacheManager::nukeCache() {
    qInfo() << "[FileCacheManager] NUKE CACHE requested by user. Wiping all databases, folder caches, and thumbnails...";
    
    if (m_db) m_db->clear();
    {
        QMutexLocker lock(&m_keyMutex);
        m_knownKeys.clear();
    }
    {
        QMutexLocker lock(&m_rootStatsMutex);
        m_rootStats.clear();
        QSettings settings(getStatsIniPath(), QSettings::IniFormat);
        settings.clear();
    }
    m_dirty = false;

    // Wipe serialized folder metadata databases (.bin)
    QString folderCacheDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/folder_caches";
    QDir folderDir(folderCacheDir);
    if (folderDir.exists()) {
        folderDir.removeRecursively();
        folderDir.mkpath(".");
    }

    emit cacheCleared();
    qInfo() << "[FileCacheManager] Nuke Cache complete.";
}

QString FileCacheManager::extractRoot(const QString& originalPath) const {
    if (originalPath.isEmpty()) return QString();
    QString path = QDir::toNativeSeparators(originalPath);
    if (path.length() >= 2 && path[1] == ':') {
        return path.left(2).toUpper();
    }
    if (path.startsWith("\\\\")) {
        int nextSlash = path.indexOf('\\', 2);
        if (nextSlash != -1) {
            nextSlash = path.indexOf('\\', nextSlash + 1);
            if (nextSlash != -1) return path.left(nextSlash).toUpper();
            return path.toUpper();
        }
    }
    return "/";
}

void FileCacheManager::trackRootStat(const QString& originalPath, qint64 bytes) {
    QString root = extractRoot(originalPath);
    if (root.isEmpty()) return;
    QMutexLocker lock(&m_rootStatsMutex);
    m_rootStats[root].count++;
    m_rootStats[root].bytes += bytes;

    QSettings settings(getStatsIniPath(), QSettings::IniFormat);
    settings.beginGroup(root);
    settings.setValue("count", m_rootStats[root].count);
    settings.setValue("bytes", m_rootStats[root].bytes);
    settings.endGroup();
}

QStringList FileCacheManager::getTrackedRootPaths() {
    QMutexLocker lock(&m_rootStatsMutex);
    return m_rootStats.keys();
}

QVariantMap FileCacheManager::getTrackedRootPathStats() {
    QVariantMap map;
    qint64 totalCount = 0;
    qint64 totalBytes = 0;

    QSettings settings(getStatsIniPath(), QSettings::IniFormat);
    QStringList groups = settings.childGroups();

    {
        QMutexLocker lock(&m_rootStatsMutex);
        for (const QString& root : groups) {
            settings.beginGroup(root);
            qint64 c = settings.value("count", 0).toLongLong();
            qint64 b = settings.value("bytes", 0).toLongLong();
            settings.endGroup();
            if (c > 0) {
                m_rootStats[root].count = std::max(m_rootStats[root].count, c);
                m_rootStats[root].bytes = std::max(m_rootStats[root].bytes, b);
            }
        }

        for (auto it = m_rootStats.begin(); it != m_rootStats.end(); ++it) {
            if (it.value().count > 0 || it.key() != "__total__") {
                QVariantMap sub;
                sub["count"] = it.value().count;
                sub["bytes"] = it.value().bytes;
                map[it.key()] = sub;
                totalCount += it.value().count;
                totalBytes += it.value().bytes;
            }
        }
    }

    QVariantMap total;
    total["count"] = totalCount;
    total["bytes"] = totalBytes;
    if (m_db) {
        qint64 dbBytes = m_db->totalSizeBytes();
        if (dbBytes > 0) {
            total["bytes"] = dbBytes;
        }
    }
    map["__total__"] = total;
    return map;
}

void FileCacheManager::nukeCacheForPrefix(const QString& pathPrefix) {
    if (pathPrefix.isEmpty() || !m_db) return;
    qInfo() << "[FileCacheManager] Nuking cache for path prefix:" << pathPrefix;

    QString root = extractRoot(pathPrefix);
    QList<QString> allKeys = m_db->getAllKeys();
    int removed = 0;

    for (const QString& key : allKeys) {
        CacheEntry entry = m_db->get(key);
        if (extractRoot(entry.originalPath) == root) {
            if (!entry.thumbPath.isEmpty() && QFile::exists(entry.thumbPath)) {
                QFile::remove(entry.thumbPath);
            }
            m_db->remove(key);
            {
                QMutexLocker lock(&m_keyMutex);
                m_knownKeys.remove(key);
            }
            removed++;
        }
    }

    {
        QMutexLocker lock(&m_rootStatsMutex);
        m_rootStats.remove(root);
        QSettings settings(getStatsIniPath(), QSettings::IniFormat);
        settings.remove(root);
    }

    if (removed > 0) {
        m_dirty = true;
        m_db->save(m_dbPath);
        emit cacheCleared();
    }
    qInfo() << "[FileCacheManager] Nuked" << removed << "entries for prefix:" << pathPrefix;
}

int FileCacheManager::pruneStaleEntries(const QString& folderPrefix, const QSet<QString>& validFilePaths, const QSize& thumbSize) {
    if (!m_canWrite || !m_db || folderPrefix.isEmpty()) return 0;

    QString cleanPrefix = QDir::cleanPath(folderPrefix);
    cleanPrefix = QDir::toNativeSeparators(cleanPrefix);
    if (!cleanPrefix.endsWith('\\')) cleanPrefix += "\\";

    // Build the set of valid keys from the current file list for fast lookup
    QSet<QString> validKeys;
    validKeys.reserve(validFilePaths.size());
    for (const QString& path : validFilePaths)
        validKeys.insert(getCoalesceKey(path, thumbSize));

    // Walk the in-memory index — only examine entries matching cleanPrefix
    QList<QString> allKeys;
    {
        QMutexLocker lock(&m_keyMutex);
        allKeys = m_knownKeys.values();
    }

    int pruned = 0;
    qint64 orphanedBytes = 0;

    for (const QString& key : allKeys) {
        CacheEntry entry = m_db->get(key);
        if (entry.originalPath.isEmpty()) continue;

        QString entryPath = QDir::toNativeSeparators(entry.originalPath);
        // CRITICAL GUARD: Only evaluate entries that strictly belong to the scanned folder prefix!
        // Entries from other folders, other drives (C:, D:, I:, etc.) are NEVER touched!
        if (entryPath.startsWith(cleanPrefix, Qt::CaseInsensitive)) {
            if (!validKeys.contains(key) && !QFile::exists(entry.originalPath)) {
                orphanedBytes += entry.fileSizeBytes;
                m_db->remove(key);
                {
                    QMutexLocker lock(&m_keyMutex);
                    m_knownKeys.remove(key);
                }
                pruned++;
            }
        }
    }

    if (pruned > 0) {
        qInfo() << "[FileCacheManager] Safely pruned" << pruned << "deleted entries from" << cleanPrefix
                << "(" << orphanedBytes / 1024 << "KB orphaned).";
        m_dirty = true;
    }
    return pruned;
}

void FileCacheManager::compact() {
    if (!m_canWrite || !m_db) return;

    MmapCacheDatabase* mmapDb = dynamic_cast<MmapCacheDatabase*>(m_db.get());
    if (!mmapDb) return;

    QString tmpPath = m_dbPath + ".compact.tmp";
    QFile::remove(tmpPath);

    // Create a fresh database for compaction output
    auto freshDb = std::make_unique<MmapCacheDatabase>();
    freshDb->load(tmpPath);

    // Copy all currently-indexed (valid) entries from the live mmap to the fresh one
    QList<QString> validKeys;
    {
        QMutexLocker lock(&m_keyMutex);
        validKeys = m_knownKeys.values();
    }

    int copied = 0;
    for (const QString& key : validKeys) {
        CacheEntry entry = m_db->get(key);
        QByteArray data = mmapDb->getRawData(key);
        if (!data.isEmpty()) {
            freshDb->insertRawData(key, entry.originalPath, entry.sizeKey, data);
            copied++;
        }
    }

    // Swap: unmap current, replace file, reload
    m_db.reset();   // Closes and unmaps current mmap
    freshDb.reset(); // Flushes the tmp mmap

    if (QFile::remove(m_dbPath) && QFile::rename(tmpPath, m_dbPath)) {
        m_db = std::make_unique<MmapCacheDatabase>();
        m_db->load(m_dbPath);
        rebuildKeyIndex();
        qInfo() << "[FileCacheManager] Compaction complete. Kept" << copied << "entries. New size:"
                << QFileInfo(m_dbPath).size() / (1024 * 1024) << "MB";
    } else {
        // Rollback: reload original
        qWarning() << "[FileCacheManager] Compaction swap failed — reloading original.";
        m_db = std::make_unique<MmapCacheDatabase>();
        m_db->load(m_dbPath);
        rebuildKeyIndex();
        QFile::remove(tmpPath);
    }
}

