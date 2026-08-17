#ifndef FILECACHEMANAGER_H
#define FILECACHEMANAGER_H

#include <QObject>
#include <QString>
#include <QSize>
#include <QDateTime>
#include <QHash>
#include <QMutex>
#include <QTimer>
#include <QDir>
#include <memory>

struct CacheEntry {
    QString originalPath;
    QString sizeKey;
    QString thumbPath;
    qint64 lastAccessed; // Unix timestamp
    qint64 fileSizeBytes;

    // Serialization support for QDataStream
    friend QDataStream& operator<<(QDataStream& out, const CacheEntry& entry) {
        out << entry.originalPath << entry.sizeKey << entry.thumbPath 
            << entry.lastAccessed << entry.fileSizeBytes;
        return out;
    }
    friend QDataStream& operator>>(QDataStream& in, CacheEntry& entry) {
        in >> entry.originalPath >> entry.sizeKey >> entry.thumbPath 
           >> entry.lastAccessed >> entry.fileSizeBytes;
        return in;
    }
};

/**
 * @brief Abstract interface for the Disk Cache Database.
 * Allows swapping between QHash, LMDB, SQLite, etc.
 */
class ICacheDatabase {
public:
    virtual ~ICacheDatabase() = default;
    
    // Core operations
    virtual void load(const QString& dbPath) = 0;
    virtual void save(const QString& dbPath) = 0;
    virtual bool contains(const QString& key) = 0;
    virtual CacheEntry get(const QString& key) = 0;
    virtual void insert(const QString& key, const CacheEntry& entry) = 0;
    virtual void remove(const QString& key) = 0;
    
    // Maintenance
    virtual qint64 totalSizeBytes() = 0;
    virtual QList<QString> getOldestKeys(int limit) = 0;
    virtual QList<QString> getAllKeys() = 0;
    virtual void clear() = 0;
};

/**
 * @brief Option 1: Native QDataStream + QHash implementation.
 * Zero external plumbing, blazing fast memory lookups.
 */
class QHashCacheDatabase : public ICacheDatabase {
public:
    void load(const QString& dbPath) override;
    void save(const QString& dbPath) override;
    bool contains(const QString& key) override;
    CacheEntry get(const QString& key) override;
    void insert(const QString& key, const CacheEntry& entry) override;
    void remove(const QString& key) override;
    
    qint64 totalSizeBytes() override;
    QList<QString> getOldestKeys(int limit) override;
    QList<QString> getAllKeys() override;
    void clear() override;

private:
    QHash<QString, CacheEntry> m_db;
    qint64 m_totalBytes = 0;
    QMutex m_mutex;
};

/**
 * @brief Option 2: Memory-Mapped Ring Buffer implementation.
 * Bypasses filesystem overhead by using a single mmap'd binary log.
 */
class MmapCacheDatabase : public ICacheDatabase {
public:
    MmapCacheDatabase();
    ~MmapCacheDatabase() override;

    void load(const QString& dbPath) override;
    void save(const QString& dbPath) override;
    bool contains(const QString& key) override;
    CacheEntry get(const QString& key) override;
    void insert(const QString& key, const CacheEntry& entry) override;
    void remove(const QString& key) override;
    
    qint64 totalSizeBytes() override;
    QList<QString> getOldestKeys(int limit) override;
    QList<QString> getAllKeys() override;
    void clear() override;

    // Direct byte access for the ring buffer
    bool isMapped() const { return m_mappedData != nullptr; }
    QByteArray getRawData(const QString& key);
    void insertRawData(const QString& key, const QString& originalPath, const QString& sizeKey, const QByteArray& data);

private:
    // Version 3: Append-only log. head = next write offset. No tail, no eviction.
    struct RingHeader {
        quint32 magic;      // 0x4D4D4150 ('MMAP')
        quint32 version;    // 3
        quint64 head;       // Next write position
        quint64 capacity;   // Current mapped size
        quint64 entryCount; // Total records written
        quint64 _reserved;  // Padding (was 'tail' in v2)
    };

    struct RecordHeader {
        quint32 keyLen;
        quint32 dataLen;
        // followed by key bytes
        // followed by data bytes
    };

    QHash<QString, CacheEntry> m_index; // In-memory index
    QHash<QString, quint64> m_offsets;  // Key -> Offset in mmap
    
    uchar* m_mappedData = nullptr;
    QFile m_file;
    quint64 m_capacity;
    QMutex m_mutex;
    
    bool growFile(quint64 newCapacity); // Expand the mmap file
    void clearInternal();
};

/**
 * @brief Manages the thumbnail disk cache and handles pruning.
 */
class FileCacheManager : public QObject {
    Q_OBJECT
public:
    static FileCacheManager& instance();
    
    void initialize();
    
    // Called by AsyncImageProvider (Legacy / File-based)
    QString getCachedPath(const QString& id, const QSize& requestedSize);
    void registerCacheFile(const QString& id, const QSize& requestedSize, const QString& thumbPath, qint64 sizeBytes);
    
    // Direct Byte Access for Mmap Cache (Zero-Copy)
    QByteArray getCachedData(const QString& id, const QSize& requestedSize);
    void registerCachedData(const QString& id, const QSize& requestedSize, const QByteArray& data);
    
    // O(1) membership check — no data read, just index lookup
    bool isCached(const QString& id, const QSize& requestedSize);

    // Filesystem Reconciliation:
    // Call after a directory scan with the complete set of valid file paths for THAT folder.
    // Only prunes files strictly within folderPrefix whose source file is confirmed missing on disk.
    int pruneStaleEntries(const QString& folderPrefix, const QSet<QString>& validFilePaths, const QSize& thumbSize);

    // Rewrite the mmap to contain only the currently-indexed (valid) entries.
    // Reclaims space from deleted files. Run in a background thread.
    void compact();

    // Limit management
    Q_INVOKABLE void setMaxDiskCacheSizeMB(int megabytes);
    Q_INVOKABLE void clearCache();
    Q_INVOKABLE void nukeCache();
    Q_INVOKABLE void nukeCacheForPrefix(const QString& pathPrefix);
    Q_INVOKABLE QStringList getTrackedRootPaths();

    // Returns per-drive stats: { "D:\\" -> {"count": N, "bytes": B}, ... }
    // plus "__total__" key for the global DB stats
    Q_INVOKABLE QVariantMap getTrackedRootPathStats();
    
    Q_INVOKABLE QString getDbPath() const { return m_dbPath; }
    static QString getCacheDirectory();

signals:
    void cacheCleared();

private slots:
    void performMaintenance();

private:
    FileCacheManager();
    ~FileCacheManager();
    
    QString getCoalesceKey(const QString &id, const QSize &size);
    void rebuildKeyIndex();

    std::unique_ptr<ICacheDatabase> m_db;
    QString m_dbPath;
    qint64 m_maxBytes;
    QTimer* m_maintenanceTimer;
    bool m_dirty = false;
    bool m_canWrite = true;
    
    QSet<QString> m_knownKeys;
    QMutex m_keyMutex;

    // Per-drive root stats: root path (upper) -> {count, totalBytes}
    struct RootStat { qint64 count = 0; qint64 bytes = 0; };
    QMap<QString, RootStat> m_rootStats;
    QMutex m_rootStatsMutex;

    QString extractRoot(const QString& originalPath) const;
    void trackRootStat(const QString& originalPath, qint64 bytes);
    QString getStatsIniPath() const;
};

#endif // FILECACHEMANAGER_H
