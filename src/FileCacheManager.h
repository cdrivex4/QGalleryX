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
    void clear() override;

private:
    QHash<QString, CacheEntry> m_db;
    qint64 m_totalBytes = 0;
    QMutex m_mutex;
};

/**
 * @brief Option 2: LMDB implementation (Stub).
 */
class LmdbCacheDatabase : public ICacheDatabase {
public:
    void load(const QString& dbPath) override;
    void save(const QString& dbPath) override;
    bool contains(const QString& key) override;
    CacheEntry get(const QString& key) override;
    void insert(const QString& key, const CacheEntry& entry) override;
    void remove(const QString& key) override;
    
    qint64 totalSizeBytes() override;
    QList<QString> getOldestKeys(int limit) override;
    void clear() override;
};

/**
 * @brief Manages the thumbnail disk cache and handles pruning.
 */
class FileCacheManager : public QObject {
    Q_OBJECT
public:
    static FileCacheManager& instance();
    
    void initialize();
    
    // Called by AsyncImageProvider
    QString getCachedPath(const QString& id, const QSize& requestedSize);
    void registerCacheFile(const QString& id, const QSize& requestedSize, const QString& thumbPath, qint64 sizeBytes);
    
    // Limit management
    void setMaxDiskCacheSizeMB(int megabytes);
    void clearCache();
    
    QString getDbPath() const { return m_dbPath; }

private slots:
    void performMaintenance();

private:
    FileCacheManager();
    ~FileCacheManager();
    
    QString getCoalesceKey(const QString &id, const QSize &size);

    std::unique_ptr<ICacheDatabase> m_db;
    QString m_dbPath;
    qint64 m_maxBytes;
    QTimer* m_maintenanceTimer;
    bool m_dirty = false;
    bool m_canWrite = true;
};

#endif // FILECACHEMANAGER_H
