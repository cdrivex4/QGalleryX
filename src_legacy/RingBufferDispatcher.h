#ifndef RINGBUFFERDISPATCHER_H
#define RINGBUFFERDISPATCHER_H

#include <QString>
#include <atomic>
#include <functional>
#include <vector>
#include <memory>

/**
 * @brief Lock-Free Ring-Buffer Task Dispatcher for QGalleryX.
 * Replaces mutex-based TaskScheduler queues with atomic lock-free ring buffers
 * for 3 priority rings: Ring 0 (Immediate Viewport), Ring 1 (2x Lookahead), Ring 2 (Precache).
 */
class RingBufferDispatcher {
public:
    using Task = std::function<void()>;

    enum RingPriority {
        Ring0_Immediate = 0, // Viewport Visible Tiles
        Ring1_Lookahead = 1, // 2x Viewport Bounds
        Ring2_Precache  = 2  // Background Crawler
    };

    struct DispatchEntry {
        Task task;
        QString key;
        RingPriority priority;
        uint64_t sequence;
    };

    explicit RingBufferDispatcher(size_t capacity = 100000);
    ~RingBufferDispatcher() = default;

    // Lock-free push into target ring
    bool push(Task task, RingPriority priority, const QString& key = "");

    // Lock-free pop from highest priority non-empty ring
    bool pop(DispatchEntry& outEntry, int governorMode = 0);

    // Maintenance
    void clear();
    size_t size(RingPriority priority) const;
    bool isEmpty() const;

private:
    struct RingSlot {
        std::atomic<bool> isWritten{false};
        DispatchEntry entry;
    };

    struct LockFreeRing {
        std::vector<RingSlot> buffer;
        std::atomic<size_t> head{0};
        std::atomic<size_t> tail{0};
        size_t capacity;
        std::atomic_flag spinlock = ATOMIC_FLAG_INIT;

        explicit LockFreeRing(size_t cap);
        bool push(const DispatchEntry& item);
        bool pop(DispatchEntry& item);
        bool popLifo(DispatchEntry& item);
        void clear();
        size_t size() const;
    };

    LockFreeRing m_ring0;
    LockFreeRing m_ring1;
    LockFreeRing m_ring2;
    std::atomic<uint64_t> m_sequence{0};
};

#endif // RINGBUFFERDISPATCHER_H
