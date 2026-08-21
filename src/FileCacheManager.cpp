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

static constexpr quint64 MMAP_GROW_CHUNK = 16LL * 1024LL * 1024LL; // Start and grow incrementally by 16MB
static constexpr quint32 MMAP_MAGIC   = 0x4D4D4150; // 'MMAP'
static constexpr quint32 MMAP_VERSION = 4;           // Append-only log with persistent originalPath

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

    struct RecordHeaderV3 {
        quint32 keyLen;
        quint32 dataLen;
    };

    RingHeader* header = reinterpret_cast<RingHeader*>(m_mappedData);

    // Accept both v3 and v4 append-only logs!
    if (header->magic != MMAP_MAGIC || (header->version != 3 && header->version != 4) ||
        header->head < sizeof(RingHeader) || header->head > m_capacity) {

        qInfo() << "[MmapCacheDatabase] Initialising fresh append-only log (v4) at" << dbPath
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
        m_dataOffsets.clear();
        m_dataLengths.clear();
        return;
    }

    quint32 fileVer = header->version;

    // Walk forward from the start of the data region to head, rebuilding the index.
    quint64 current = sizeof(RingHeader);
    quint64 writeHead = header->head;
    int restored = 0;

    while (current < writeHead) {
        if (fileVer == 3) {
            if (current + sizeof(RecordHeaderV3) > writeHead) break;
            RecordHeaderV3* rec = reinterpret_cast<RecordHeaderV3*>(m_mappedData + current);
            if (rec->keyLen == 0 || rec->keyLen > 2048 ||
                rec->dataLen == 0 || rec->dataLen > 1024*1024*50 ||
                current + sizeof(RecordHeaderV3) + rec->keyLen + rec->dataLen > writeHead) {
                header->head = current;
                break;
            }

            QString key = QString::fromUtf8(
                reinterpret_cast<const char*>(m_mappedData + current + sizeof(RecordHeaderV3)),
                rec->keyLen);

            quint64 dataOffset = current + sizeof(RecordHeaderV3) + rec->keyLen;
            quint32 dataLen = rec->dataLen;

            CacheEntry entry;
            entry.fileSizeBytes = dataLen;
            entry.lastAccessed  = QDateTime::currentMSecsSinceEpoch();

            m_index.insert(key, entry);
            m_offsets.insert(key, current);
            m_dataOffsets.insert(key, dataOffset);
            m_dataLengths.insert(key, dataLen);
            restored++;

            current += sizeof(RecordHeaderV3) + rec->keyLen + rec->dataLen;
        } else { // Version 4
            if (current + sizeof(RecordHeader) > writeHead) break;
            RecordHeader* rec = reinterpret_cast<RecordHeader*>(m_mappedData + current);
            if (rec->keyLen == 0 || rec->keyLen > 2048 ||
                rec->pathLen > 4096 ||
                rec->dataLen == 0 || rec->dataLen > 1024*1024*50 ||
                current + sizeof(RecordHeader) + rec->keyLen + rec->pathLen + rec->dataLen > writeHead) {
                header->head = current;
                break;
            }

            QString key = QString::fromUtf8(
                reinterpret_cast<const char*>(m_mappedData + current + sizeof(RecordHeader)),
                rec->keyLen);

            QString origPath;
            if (rec->pathLen > 0) {
                origPath = QString::fromUtf8(
                    reinterpret_cast<const char*>(m_mappedData + current + sizeof(RecordHeader) + rec->keyLen),
                    rec->pathLen);
            }

            quint64 dataOffset = current + sizeof(RecordHeader) + rec->keyLen + rec->pathLen;
            quint32 dataLen = rec->dataLen;

            CacheEntry entry;
            entry.originalPath  = origPath;
            entry.fileSizeBytes = dataLen;
            entry.lastAccessed  = QDateTime::currentMSecsSinceEpoch();

            m_index.insert(key, entry);
            m_offsets.insert(key, current);
            m_dataOffsets.insert(key, dataOffset);
            m_dataLengths.insert(key, dataLen);
            restored++;

            current += sizeof(RecordHeader) + rec->keyLen + rec->pathLen + rec->dataLen;
        }
    }

    header->entryCount = restored;
    qInfo() << "[MmapCacheDatabase] Restored" << restored << "entries (Format v" << fileVer << ") from"
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

void MmapCacheDatabase::insert(const QString& key, const CacheEntry& entry) {
    QMutexLocker lock(&m_mutex);
    if (m_index.contains(key)) {
        m_index[key] = entry;
    }
}

void MmapCacheDatabase::remove(const QString& key) {
    QMutexLocker lock(&m_mutex);
    m_index.remove(key);
    m_offsets.remove(key);
    m_dataOffsets.remove(key);
    m_dataLengths.remove(key);
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
    m_dataOffsets.clear();
    m_dataLengths.clear();
}

QByteArray MmapCacheDatabase::getRawData(const QString& key) {
    QMutexLocker lock(&m_mutex);
    if (!m_mappedData || !m_dataOffsets.contains(key)) return QByteArray();

    quint64 dataOffset = m_dataOffsets.value(key);
    quint32 dataLen = m_dataLengths.value(key);
    if (dataOffset + dataLen > m_capacity) {
        m_dataOffsets.remove(key);
        m_dataLengths.remove(key);
        m_offsets.remove(key);
        m_index.remove(key);
        return QByteArray();
    }

    // Zero-copy slice directly from the mmap page. OS page cache handles RAM residency.
    return QByteArray(reinterpret_cast<const char*>(m_mappedData + dataOffset), dataLen);
}

void MmapCacheDatabase::insertRawData(const QString& key, const QString& originalPath, const QString& sizeKey, const QByteArray& data) {
    QMutexLocker lock(&m_mutex);
    if (!m_mappedData) return;

    // Skip duplicates — already stored permanently at existing offset
    if (m_offsets.contains(key)) return;

    QByteArray keyBytes = key.toUtf8();
    QByteArray pathBytes = originalPath.toUtf8();
    quint64 totalRequired = sizeof(RecordHeader) + keyBytes.size() + pathBytes.size() + data.size();

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
    rec->pathLen = pathBytes.size();
    rec->dataLen = data.size();

    memcpy(m_mappedData + offset + sizeof(RecordHeader),                               keyBytes.constData(),  rec->keyLen);
    if (rec->pathLen > 0) {
        memcpy(m_mappedData + offset + sizeof(RecordHeader) + rec->keyLen,             pathBytes.constData(), rec->pathLen);
    }
    memcpy(m_mappedData + offset + sizeof(RecordHeader) + rec->keyLen + rec->pathLen, data.constData(),      rec->dataLen);

    quint64 dataOffset = offset + sizeof(RecordHeader) + rec->keyLen + rec->pathLen;

    CacheEntry entry;
    entry.originalPath  = originalPath;
    entry.sizeKey       = sizeKey;
    entry.fileSizeBytes = data.size();
    entry.lastAccessed  = QDateTime::currentMSecsSinceEpoch();

    m_index.insert(key, entry);
    m_offsets.insert(key, offset);
    m_dataOffsets.insert(key, dataOffset);
    m_dataLengths.insert(key, data.size());

    header->head += totalRequired;
    header->entryCount++;
}

quint64 MmapCacheDatabase::writeHead() const {
    if (!m_mappedData) return 0;
    return reinterpret_cast<const RingHeader*>(m_mappedData)->head;
}

quint64 MmapCacheDatabase::validPayloadBytes() const {
    quint64 total = 0;
    for (auto it = m_dataLengths.begin(); it != m_dataLengths.end(); ++it) {
        total += it.value();
    }
    return total;
}

void MmapCacheDatabase::compact(const QString& dbPath) {
    QMutexLocker lock(&m_mutex);
    if (!m_mappedData || m_index.isEmpty()) return;

    QString tmpPath = dbPath + ".compact.tmp";
    QFile::remove(tmpPath);

    quint64 neededCapacity = sizeof(RingHeader);
    for (auto it = m_index.begin(); it != m_index.end(); ++it) {
        neededCapacity += sizeof(RecordHeader) + it.key().toUtf8().size() + it.value().originalPath.toUtf8().size() + it.value().fileSizeBytes;
    }
    neededCapacity = qMax(neededCapacity + MMAP_GROW_CHUNK, MMAP_GROW_CHUNK);

    auto freshDb = std::make_unique<MmapCacheDatabase>();
    freshDb->load(tmpPath);

    for (auto it = m_index.begin(); it != m_index.end(); ++it) {
        QString key = it.key();
        CacheEntry entry = it.value();
        quint64 dataOffset = m_dataOffsets.value(key, 0);
        quint32 dataLen = m_dataLengths.value(key, 0);
        if (dataOffset > 0 && dataLen > 0 && dataOffset + dataLen <= m_capacity) {
            QByteArray payload = QByteArray::fromRawData(reinterpret_cast<const char*>(m_mappedData + dataOffset), dataLen);
            freshDb->insertRawData(key, entry.originalPath, entry.sizeKey, payload);
        }
    }

    if (m_mappedData) {
        m_file.unmap(m_mappedData);
        m_mappedData = nullptr;
    }
    m_file.close();
    freshDb.reset();

    if (QFile::remove(dbPath) && QFile::rename(tmpPath, dbPath)) {
        load(dbPath);
        qInfo() << "[MmapCacheDatabase] Compacted" << dbPath << "successfully.";
    } else {
        load(dbPath);
        QFile::remove(tmpPath);
    }
}

// --- FileCacheManager Implementation ---

FileCacheManager& FileCacheManager::instance() {
    static FileCacheManager instance;
    return instance;
}

QString FileCacheManager::getCacheDirectory() {
    QString cacheDir;
    QStringList args = QCoreApplication::arguments();
    int cacheDirIdx = args.indexOf("--cache-dir");
    if (cacheDirIdx != -1 && cacheDirIdx + 1 < args.size()) {
        cacheDir = args.at(cacheDirIdx + 1);
    } else {
        cacheDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
        if (cacheDir.isEmpty()) {
            cacheDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/QGalleryX";
        }
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

    m_dbPath = cacheDir + "/FileCache.mmap";
    qDebug() << "FileCacheManager: Initialized with Per-Location Memory-Mapped DBs at" << cacheDir << "(Writability:" << m_canWrite << ")";

    m_maintenanceTimer = new QTimer(this);
    connect(m_maintenanceTimer, &QTimer::timeout, this, &FileCacheManager::performMaintenance);
}

FileCacheManager::~FileCacheManager() {
    QMutexLocker lock(&m_dbMutex);
    m_dbs.clear();
}

QString FileCacheManager::getStatsIniPath() const {
    QString cacheDir = FileCacheManager::getCacheDirectory();
    return cacheDir + "/cache_stats.ini";
}

QString FileCacheManager::rootToDbFileName(const QString& root) const {
    if (root.isEmpty() || root == "/" || root == "__DEFAULT__") return QStringLiteral("FileCache.mmap");
    QString safe = root;
    safe.replace(":\\", "_");
    safe.replace(":/", "_");
    safe.replace(":", "_");
    safe.replace("\\\\", "");
    safe.replace("\\", "_");
    safe.replace("/", "_");
    safe.replace("*", "_");
    safe.replace("?", "_");
    safe.replace("\"", "_");
    safe.replace("<", "_");
    safe.replace(">", "_");
    safe.replace("|", "_");
    while (safe.startsWith("_")) safe = safe.mid(1);
    while (safe.endsWith("_")) safe.chop(1);
    return QString("FileCache_%1.mmap").arg(safe);
}

MmapCacheDatabase* FileCacheManager::getDatabaseForRoot(const QString& root, bool createIfMissing) {
    QString normRoot = root.isEmpty() ? QStringLiteral("__DEFAULT__") : root.toUpper();
    QMutexLocker lock(&m_dbMutex);

    auto it = m_dbs.find(normRoot);
    if (it != m_dbs.end()) {
        return it.value().get();
    }

    if (!createIfMissing) {
        return nullptr;
    }

    QString cacheDir = getCacheDirectory();
    QString fileName = rootToDbFileName(normRoot);
    QString dbPath = cacheDir + "/" + fileName;

    auto db = std::make_shared<MmapCacheDatabase>();
    db->load(dbPath);
    m_dbs.insert(normRoot, db);
    return db.get();
}

void FileCacheManager::initialize() {
    QString cacheDir = getCacheDirectory();
    QDir dir(cacheDir);

    // Load any existing per-drive FileCache_*.mmap and legacy FileCache.mmap
    QFileInfoList mmapFiles = dir.entryInfoList(QStringList() << "FileCache*.mmap", QDir::Files);
    for (const auto& fi : mmapFiles) {
        QString base = fi.fileName();
        QString root;
        if (base == "FileCache.mmap") {
            root = "__DEFAULT__";
        } else if (base.startsWith("FileCache_") && base.endsWith(".mmap")) {
            // e.g. FileCache_C.mmap -> C:
            QString name = base.mid(10, base.length() - 15);
            if (name.length() == 1) {
                root = name.toUpper() + ":";
            } else if (name.contains('_')) {
                // e.g. QUAKE2_I -> \\QUAKE2\I
                root = "\\\\" + QString(name).replace('_', '\\');
            } else {
                root = name.toUpper();
            }
        }
        if (!root.isEmpty()) {
            auto db = std::make_shared<MmapCacheDatabase>();
            db->load(fi.absoluteFilePath());
            QMutexLocker lock(&m_dbMutex);
            m_dbs.insert(root.toUpper(), db);
        }
    }

    rebuildKeyIndex();
    m_maintenanceTimer->start(60000);
}

void FileCacheManager::rebuildKeyIndex() {
    QMutexLocker lock(&m_keyMutex);
    m_knownKeys.clear();
    
    QMap<QString, RootStat> freshStats;
    bool hasMissingPaths = false;

    {
        QMutexLocker dbLock(&m_dbMutex);
        for (auto it = m_dbs.begin(); it != m_dbs.end(); ++it) {
            QString rootKey = it.key();
            auto db = it.value();
            QList<QString> keys = db->getAllKeys();
            for (const QString& k : keys) {
                m_knownKeys.insert(k);
                CacheEntry entry = db->get(k);
                QString root = !entry.originalPath.isEmpty() ? extractRoot(entry.originalPath) : rootKey;
                if (!root.isEmpty() && root != "__DEFAULT__") {
                    freshStats[root].count++;
                    freshStats[root].bytes += entry.fileSizeBytes;
                } else if (rootKey != "__DEFAULT__") {
                    freshStats[rootKey].count++;
                    freshStats[rootKey].bytes += entry.fileSizeBytes;
                } else {
                    hasMissingPaths = true;
                }
            }
        }
    }

    // Auto-migration & path reconciliation from folder_caches/*.bin
    QString folderCacheDir;
    QStringList args = QCoreApplication::arguments();
    int cacheDirIdx = args.indexOf("--cache-dir");
    if (cacheDirIdx != -1 && cacheDirIdx + 1 < args.size()) {
        folderCacheDir = args.at(cacheDirIdx + 1) + "/folder_caches";
    } else {
        folderCacheDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/folder_caches";
    }

    QDir folderDir(folderCacheDir);
    QFileInfoList binFiles = folderDir.entryInfoList(QStringList() << "*.bin", QDir::Files);
    int matched = 0;
    for (const auto& binInfo : binFiles) {
        QFile bf(binInfo.absoluteFilePath());
        if (bf.open(QIODevice::ReadOnly)) {
            QDataStream in(&bf);
            int count = 0;
            in >> count;
            for (int i = 0; i < count; ++i) {
                QString path;
                qint64 s;
                QDateTime d;
                in >> path >> s >> d;
                QString key = getCoalesceKey(path, QSize(200, 200));
                if (m_knownKeys.contains(key)) {
                    QString root = extractRoot(path);
                    if (!root.isEmpty()) {
                        MmapCacheDatabase* targetDb = getDatabaseForRoot(root, true);
                        if (targetDb) {
                            MmapCacheDatabase* legacyDb = getDatabaseForRoot("__DEFAULT__", false);
                            if (legacyDb && legacyDb->contains(key)) {
                                QByteArray rawData = legacyDb->getRawData(key);
                                if (!rawData.isEmpty()) {
                                    targetDb->insertRawData(key, path, "200x200", rawData);
                                    legacyDb->remove(key);
                                    freshStats[root].count++;
                                    freshStats[root].bytes += rawData.size();
                                    matched++;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (matched > 0) {
        qInfo() << "[FileCacheManager] Migrated and split" << matched << "existing cached items into dedicated per-drive databases.";
    }

    // Clean up empty legacy FileCache.mmap
    {
        QMutexLocker dbLock(&m_dbMutex);
        if (m_dbs.contains("__DEFAULT__")) {
            if (m_dbs["__DEFAULT__"]->getAllKeys().isEmpty()) {
                m_dbs.remove("__DEFAULT__");
                QString cacheDir = getCacheDirectory();
                QFile::remove(cacheDir + "/FileCache.mmap");
            }
        }
    }

    {
        QMutexLocker statLock(&m_rootStatsMutex);
        m_rootStats = std::move(freshStats);
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

    // Single unified thumbnail bucket ONLY for grid/overview/semantic thumbnail sizes (width & height <= 512px).
    if (size.isValid() && !size.isEmpty() && size.width() <= 512 && size.height() <= 512) {
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
    if (!m_canWrite) return QString();
    QString root = extractRoot(id);
    QString key = getCoalesceKey(id, requestedSize);
    
    {
        QMutexLocker lock(&m_keyMutex);
        if (!m_knownKeys.isEmpty() && !m_knownKeys.contains(key)) {
            return QString();
        }
    }

    MmapCacheDatabase* db = getDatabaseForRoot(root, false);
    if (db && db->contains(key)) {
        CacheEntry entry = db->get(key);
        if (QFile::exists(entry.thumbPath)) {
            return entry.thumbPath;
        } else {
            db->remove(key);
            {
                QMutexLocker lock(&m_keyMutex);
                m_knownKeys.remove(key);
            }
        }
    }
    return QString();
}

void FileCacheManager::registerCacheFile(const QString& id, const QSize& requestedSize, const QString& thumbPath, qint64 sizeBytes) {
    if (!m_canWrite) return;
    QString root = extractRoot(id);
    QString key = getCoalesceKey(id, requestedSize);
    
    MmapCacheDatabase* db = getDatabaseForRoot(root, true);
    if (db) {
        CacheEntry entry;
        entry.originalPath = id;
        entry.sizeKey = QString("%1x%2").arg(requestedSize.width()).arg(requestedSize.height());
        entry.thumbPath = thumbPath;
        entry.lastAccessed = QDateTime::currentMSecsSinceEpoch();
        entry.fileSizeBytes = sizeBytes;
        
        db->insert(key, entry);
        {
            QMutexLocker lock(&m_keyMutex);
            m_knownKeys.insert(key);
        }
        trackRootStat(id, sizeBytes);
        m_dirty = true;
    }
}

QByteArray FileCacheManager::getCachedData(const QString& id, const QSize& requestedSize) {
    if (!m_canWrite) return QByteArray();
    QString root = extractRoot(id);
    QString key = getCoalesceKey(id, requestedSize);

    MmapCacheDatabase* db = getDatabaseForRoot(root, false);
    if (db && db->contains(key)) {
        return db->getRawData(key);
    }

    // Fallback check in default/legacy db
    MmapCacheDatabase* defDb = getDatabaseForRoot("__DEFAULT__", false);
    if (defDb && defDb->contains(key)) {
        return defDb->getRawData(key);
    }

    return QByteArray();
}

bool FileCacheManager::isCached(const QString& id, const QSize& requestedSize) {
    if (!m_canWrite) return false;
    QString key = getCoalesceKey(id, requestedSize);
    QMutexLocker lock(&m_keyMutex);
    return m_knownKeys.contains(key);
}

void FileCacheManager::registerCachedData(const QString& id, const QSize& requestedSize, const QByteArray& data) {
    if (!m_canWrite) return;
    QString root = extractRoot(id);
    MmapCacheDatabase* db = getDatabaseForRoot(root, true);
    if (db) {
        QString key = getCoalesceKey(id, requestedSize);
        QString sizeKey = QString("%1x%2").arg(requestedSize.width()).arg(requestedSize.height());
        if (!db->contains(key)) {
            db->insertRawData(key, id, sizeKey, data);
            {
                QMutexLocker lock(&m_keyMutex);
                m_knownKeys.insert(key);
            }
            trackRootStat(id, data.size());
            m_dirty = true;
        }
    }
}

void FileCacheManager::performMaintenance() {
    // Maintenance is now managed per-drive naturally via individual .mmap databases
}

void FileCacheManager::clearCache() {
    nukeCache();
}

void FileCacheManager::nukeCache() {
    qInfo() << "[FileCacheManager] NUKE ALL CACHE requested by user. Wiping all per-drive databases, folder caches, and thumbnails...";
    
    QString cacheDir = getCacheDirectory();

    {
        QMutexLocker lock(&m_dbMutex);
        for (auto it = m_dbs.begin(); it != m_dbs.end(); ++it) {
            it.value()->clear();
        }
        m_dbs.clear();
    }

    // Delete all FileCache*.mmap files from disk
    QDir dir(cacheDir);
    QFileInfoList mmapFiles = dir.entryInfoList(QStringList() << "FileCache*.mmap", QDir::Files);
    for (const auto& fi : mmapFiles) {
        QFile::remove(fi.absoluteFilePath());
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

    // Wipe serialized folder metadata databases (.bin)
    QString folderCacheDir;
    QStringList args = QCoreApplication::arguments();
    int cacheDirIdx = args.indexOf("--cache-dir");
    if (cacheDirIdx != -1 && cacheDirIdx + 1 < args.size()) {
        folderCacheDir = args.at(cacheDirIdx + 1) + "/folder_caches";
    } else {
        folderCacheDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/folder_caches";
    }
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
}

QStringList FileCacheManager::getTrackedRootPaths() {
    QMutexLocker lock(&m_rootStatsMutex);
    return m_rootStats.keys();
}

QVariantMap FileCacheManager::getTrackedRootPathStats() {
    QVariantMap map;
    qint64 totalCount = 0;
    qint64 totalBytes = 0;

    QString cacheDir = getCacheDirectory();

    {
        QMutexLocker lock(&m_rootStatsMutex);
        for (auto it = m_rootStats.begin(); it != m_rootStats.end(); ++it) {
            if (it.value().count > 0 || it.key() != "__total__") {
                QVariantMap sub;
                sub["count"] = it.value().count;
                
                // Show the exact actual payload data size in MB for each drive
                qint64 driveBytes = it.value().bytes;
                if (driveBytes == 0) {
                    QString dbFile = cacheDir + "/" + rootToDbFileName(it.key());
                    driveBytes = QFileInfo(dbFile).size();
                }
                sub["bytes"] = driveBytes;

                map[it.key()] = sub;
                totalCount += it.value().count;
                totalBytes += driveBytes;
            }
        }
    }

    // Discover any other per-drive FileCache_*.mmap on disk
    QDir dir(cacheDir);
    QFileInfoList mmapFiles = dir.entryInfoList(QStringList() << "FileCache_*.mmap", QDir::Files);
    for (const auto& fi : mmapFiles) {
        QString base = fi.fileName();
        QString root;
        QString name = base.mid(10, base.length() - 15);
        if (name.length() == 1) {
            root = name.toUpper() + ":";
        } else if (name.contains('_')) {
            root = "\\\\" + QString(name).replace('_', '\\');
        } else {
            root = name.toUpper();
        }

        if (!root.isEmpty() && !map.contains(root)) {
            QVariantMap sub;
            sub["count"] = 0;
            sub["bytes"] = fi.size();
            map[root] = sub;
            totalBytes += fi.size();
        }
    }

    QVariantMap total;
    total["count"] = totalCount;
    total["bytes"] = totalBytes;
    map["__total__"] = total;
    return map;
}

void FileCacheManager::nukeCacheForPrefix(const QString& pathPrefix) {
    if (pathPrefix.isEmpty()) return;
    qInfo() << "[FileCacheManager] Nuking cache for path prefix:" << pathPrefix;

    QString root = extractRoot(pathPrefix);
    if (root.isEmpty()) return;

    QString cacheDir = getCacheDirectory();
    QString fileName = rootToDbFileName(root);
    QString filePath = cacheDir + "/" + fileName;

    // 1. Close and remove the database instance from memory
    {
        QMutexLocker lock(&m_dbMutex);
        if (m_dbs.contains(root)) {
            m_dbs[root]->clear();
            m_dbs.remove(root);
        }
    }

    // 2. Delete the physical .mmap file from disk to reclaim 100% disk space immediately
    if (QFile::exists(filePath)) {
        QFile::remove(filePath);
        qInfo() << "[FileCacheManager] Deleted per-drive cache file:" << filePath;
    }

    // 3. Clear keys and stats for this drive
    {
        QMutexLocker lock(&m_rootStatsMutex);
        m_rootStats.remove(root);
        QSettings settings(getStatsIniPath(), QSettings::IniFormat);
        settings.remove(root);
    }

    // 4. Wipe serialized folder metadata databases (.bin) that match this root prefix
    QString folderCacheDir;
    QStringList args = QCoreApplication::arguments();
    int cacheDirIdx = args.indexOf("--cache-dir");
    if (cacheDirIdx != -1 && cacheDirIdx + 1 < args.size()) {
        folderCacheDir = args.at(cacheDirIdx + 1) + "/folder_caches";
    } else {
        folderCacheDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/folder_caches";
    }
    QDir folderDir(folderCacheDir);
    if (folderDir.exists()) {
        QFileInfoList binFiles = folderDir.entryInfoList(QStringList() << "*.bin", QDir::Files);
        for (const auto& binInfo : binFiles) {
            QFile bf(binInfo.absoluteFilePath());
            if (bf.open(QIODevice::ReadOnly)) {
                QDataStream in(&bf);
                int count = 0;
                in >> count;
                if (count > 0) {
                    QString samplePath;
                    qint64 s;
                    QDateTime d;
                    in >> samplePath >> s >> d;
                    if (extractRoot(samplePath) == root) {
                        bf.close();
                        QFile::remove(binInfo.absoluteFilePath());
                        continue;
                    }
                }
                bf.close();
            }
        }
    }

    rebuildKeyIndex();
    emit cacheCleared();
    qInfo() << "[FileCacheManager] Nuke completed for drive:" << root;
}

int FileCacheManager::pruneStaleEntries(const QString& folderPrefix, const QSet<QString>& validFilePaths, const QSize& thumbSize) {
    if (!m_canWrite || folderPrefix.isEmpty()) return 0;

    QString cleanPrefix = QDir::cleanPath(folderPrefix);
    cleanPrefix = QDir::toNativeSeparators(cleanPrefix);
    if (!cleanPrefix.endsWith('\\')) cleanPrefix += "\\";

    QString root = extractRoot(folderPrefix);
    MmapCacheDatabase* db = getDatabaseForRoot(root, false);
    if (!db) return 0;

    // Build the set of valid keys from the current file list for fast lookup
    QSet<QString> validKeys;
    validKeys.reserve(validFilePaths.size());
    for (const QString& path : validFilePaths)
        validKeys.insert(getCoalesceKey(path, thumbSize));

    QList<QString> allKeys = db->getAllKeys();
    int pruned = 0;
    qint64 orphanedBytes = 0;

    for (const QString& key : allKeys) {
        CacheEntry entry = db->get(key);
        if (entry.originalPath.isEmpty()) continue;

        QString entryPath = QDir::toNativeSeparators(entry.originalPath);
        if (entryPath.startsWith(cleanPrefix, Qt::CaseInsensitive)) {
            if (!validKeys.contains(key) && !QFile::exists(entry.originalPath)) {
                orphanedBytes += entry.fileSizeBytes;
                db->remove(key);
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

        // Auto-Compaction on 30% deviation:
        // If orphaned/dead bytes exceed 30% of the active database size, compact immediately
        quint64 head = db->writeHead();
        quint64 validBytes = db->validPayloadBytes();
        if (head > 5LL * 1024LL * 1024LL && (head - validBytes) > (0.30 * head)) {
            QString cacheDir = getCacheDirectory();
            QString dbPath = cacheDir + "/" + rootToDbFileName(root);
            qInfo() << "[FileCacheManager] Dead space exceeds 30% deviation (" << (head - validBytes)/(1024*1024) << "MB dead /" << head/(1024*1024) << "MB total). Auto-compacting" << root << "...";
            db->compact(dbPath);
        }
    }
    return pruned;
}

void FileCacheManager::compact() {
    QString cacheDir = getCacheDirectory();
    QMutexLocker lock(&m_dbMutex);
    for (auto it = m_dbs.begin(); it != m_dbs.end(); ++it) {
        QString dbPath = cacheDir + "/" + rootToDbFileName(it.key());
        it.value()->compact(dbPath);
    }
    rebuildKeyIndex();
}

