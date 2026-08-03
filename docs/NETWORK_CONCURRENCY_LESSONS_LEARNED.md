# Network, Concurrency & Memory Audit Guide (Lessons Learned)

**Project:** QGalleryX (High-Performance Image & Video Gallery)  
**Last Updated:** August 2026  
**Purpose:** Comprehensive reference of concurrency, network file I/O, memory management, and multimedia playback edge cases discovered and resolved in QGalleryX. Use this document when auditing or building similar high-throughput C++/Qt media applications.

---

## 1. 🧵 Worker Thread Scheduling & Queue Dequeue Safety

### ❌ The Anti-Pattern
Calling `dequeue()` on a `QQueue` or iterating `QMap` iterators inside worker loops while other threads enqueue, clear, or modify tasks:
```cpp
// DANGEROUS: QMap iterators can be invalidated during clear(), and dequeue() on empty queue crashes Qt6Core!
for (auto it = m_cpuQueue.begin(); it != m_cpuQueue.end(); ++it) {
    if (!it.value().isEmpty()) {
        task = it.value().dequeue(); // 💥 CRASH: 0xc0000005 if dequeued concurrently or empty!
    }
}
```

### ✅ The Battle-Tested Solution
1. **Pre-initialize Priority Keys**: Pre-allocate all priority keys (`{Immediate, High, Normal, Low}`) during initialization so queue structures never re-allocate or alter map layout.
2. **Never Erase Queue Map Keys**: In `clear()`, empty the `QQueue` objects inside the lock rather than deleting map keys.
3. **Re-Verify Direct Lookup Under Lock**: Access queues via fixed array iteration and check `!queue.isEmpty()` at the exact moment of `dequeue()` under mutex protection:
```cpp
const Priority priorities[] = {Immediate, High, Normal, Low};
for (Priority p : priorities) {
    if (!m_cpuQueue[p].isEmpty()) {
        if (m_backgroundPaused && p != Immediate) continue;
        task = m_cpuQueue[p].dequeue(); // 100% thread-safe
        found = true;
        break;
    }
}
```

---

## 2. 💽 Memory-Mapped File I/O (`mmap`) Over Network / SMB Shares

### ❌ The Anti-Pattern
Assuming `QFile::map()` will always succeed or behave identically on network drives, USB shares, or read-only directories:
- On Windows, file-backed `mmap` over SMB/network shares can fail kernel page allocation or raise unhandled OS hardware exceptions (`STATUS_IN_PAGE_ERROR` `0xc0000006` or `0xc0000005` Access Violation) if SMB packets drop.
- Dereferencing `m_mappedData` without `nullptr` checks crashes immediately if mapping fails.

### ✅ The Battle-Tested Solution
1. **Verify Mapping Success**: Check `isMapped()` (`m_mappedData != nullptr`) immediately after `map()`.
2. **Automatic Fallback to Native Disk/Memory Cache**: If `mmap` fails to allocate memory or is hosted on an incompatible filesystem, automatically fall back to standard file-based/in-memory data structures:
```cpp
void FileCacheManager::initialize() {
    m_db->load(m_dbPath);
    MmapCacheDatabase* mmapDb = dynamic_cast<MmapCacheDatabase*>(m_db.get());
    if (mmapDb && !mmapDb->isMapped()) {
        qWarning() << "FileCacheManager: Mmap failed on this filesystem. Falling back to QHashCacheDatabase.";
        m_db = std::make_unique<QHashCacheDatabase>();
        m_dbPath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/thumbnails/FileCache.db";
        m_db->load(m_dbPath);
    }
}
```

---

## 3. 🔄 Memory-Mapped Ring Buffer Capacity & Wrapping Logic (`advanceHead`)

### ❌ The Anti-Pattern
Using improper boundary conditions when wrapping ring buffer write pointers (`head` and `tail`):
```cpp
// DANGEROUS: When head == tail (empty buffer), header->tail >= header->head evaluates to TRUE!
while (header->tail >= header->head || header->tail == sizeof(RingHeader)) {
    header->tail = sizeof(RingHeader);
    continue; // 💥 INFINITE LOOP: Freezes thread when ring buffer fills after hours of use!
}
```

### ✅ The Battle-Tested Solution
1. **Handle Empty Index First**: If `m_index.isEmpty()`, immediately reset `head` and `tail` to `sizeof(RingHeader)`.
2. **Strictly Bound Overlap Eviction**: Evict old tail records only while `head <= tail` and `head + requiredSize > tail`:
```cpp
if (m_index.isEmpty()) {
    header->head = sizeof(RingHeader);
    header->tail = sizeof(RingHeader);
}

if (header->head + requiredSize > m_capacity) {
    if (header->head + sizeof(RecordHeader) <= m_capacity) {
        RecordHeader* wrapMarker = reinterpret_cast<RecordHeader*>(m_mappedData + header->head);
        wrapMarker->keyLen = 0xFFFFFFFF; // Wrap marker
        wrapMarker->dataLen = 0;
    }
    header->head = sizeof(RingHeader); // Wrap head
}

while (!m_index.isEmpty() && header->head <= header->tail && (header->head + requiredSize) > header->tail) {
    // Evict tail entry safely
}
```

---

## 4. 🎬 Qt 6 Multimedia Engine & System Codec Independence

### ❌ The Anti-Pattern
Relying on default Qt 6 `QtMultimedia` on Windows without specifying the backend:
- Qt 6 defaults to **Windows Media Foundation (WMF)** (`windowsmultimedia`).
- If the host Windows OS does not have Microsoft Store codecs installed (e.g. AV1 Video Extension), QML `MediaPlayer` fails to play AV1, VP9, WebM, or MKV files even if bundled FFmpeg DLLs exist in C++.

### ✅ The Battle-Tested Solution
Explicitly set the multimedia environment variable at application startup before creating `QApplication`:
```cpp
// Force Qt Multimedia to use bundled FFmpeg engine, cutting Windows Media Foundation system dependencies
qputenv("QT_MEDIA_BACKEND", "ffmpeg");
```

---

## 5. 🖼 Thread-Safe Image Cache Dereferencing

### ❌ The Anti-Pattern
Returning pointers or shallow shared references from RAM caches (`QCache<QString, QImage>`):
- When RAM monitors call `m_cache.clear()`, deleting `QImage*` inside the cache decrements `QImagePrivate` refcounts.
- If rendering or worker threads are copying or accessing those images concurrently, non-atomic refcount destruction causes Access Violations (`0xc0000005`).

### ✅ The Battle-Tested Solution
Return a deep copy (`img->copy()`) from cache lookups so cached pixel buffers are completely isolated from garbage collection and cache purges:
```cpp
QImage AsyncImageProvider::getCachedImage(const QString &id, const QSize &size) {
    QMutexLocker locker(&m_mutex);
    QString key = id + "_" + QString::number(size.width()) + "x" + QString::number(size.height());
    if (m_cache.contains(key)) {
        QImage *img = m_cache.object(key);
        if (img && !img->isNull()) {
            return img->copy(); // Deep copy isolates rendering from cache purges
        }
    }
    return QImage();
}
```

---

## 6. 📹 FFmpeg & Frame Decoding Null Pointer Guards

### ❌ The Anti-Pattern
Passing target scaling dimensions (`dstW`, `dstH`) or buffer pointers to `sws_scale()` without checking if video headers are corrupt or zero-sized:
- Corrupted video headers or tiny target sizes can yield `dstW <= 0` or `dstH <= 0`.
- `QImage tmp(0, 0)` returns `bits() == nullptr`.
- Passing `nullptr` to `sws_scale()` or scanline loops (`tmp.constScanLine(y)`) crashes immediately.

### ✅ The Battle-Tested Solution
1. Validate `dstW > 0 && dstH > 0` before allocation.
2. Check `!tmp.isNull() && tmp.bits() != nullptr` before passing buffers to `sws_scale()`.
3. Guard scanline sampling:
```cpp
if (dstW <= 0 || dstH <= 0) return QImage();

QImage tmp(dstW, dstH, QImage::Format_RGB32);
if (tmp.isNull() || !tmp.bits()) return QImage();

uint8_t *dst_data[4] = { tmp.bits(), nullptr, nullptr, nullptr };
int dst_linesize[4] = { (int)tmp.bytesPerLine(), 0, 0, 0 };

sws_scale(cleanup.swsCtx, finalFrame->data, finalFrame->linesize, 0, finalFrame->height, dst_data, dst_linesize);

for (int y = 0; y < dstH; y += step) {
    const uchar* scanLine = tmp.constScanLine(y);
    if (!scanLine) continue; // Safe scanline dereference
    const QRgb* line = reinterpret_cast<const QRgb*>(scanLine);
    // Process pixels safely
}
```

---

## 7. 🛡 Windows Shell API Threading (`QFileIconProvider`)

### ❌ The Anti-Pattern
Invoking OS Shell APIs (e.g. `QFileIconProvider` / `SHGetFileInfoW`) directly from background worker threads on network shares without exception handling:
- Windows Shell APIs execute COM calls.
- Calling Shell APIs from uninitialized COM threads or when network shares drop causes RPC timeouts and `0xc0000005` crashes inside `shell32.dll`.

### ✅ The Battle-Tested Solution
Wrap OS icon provider calls in `try / catch` blocks and avoid blocking Shell API calls on high-throughput thread pools:
```cpp
if (image.isNull()) {
    cacheable = false;
    try {
        QFileIconProvider provider;
        QIcon icon = provider.icon(QFileInfo(path));
        if (!icon.isNull()) {
            image = icon.pixmap(requestedSize).toImage();
        }
    } catch (...) {
        qWarning() << "[AsyncImageProvider] Icon provider failed for" << path;
    }
}
```

---

## 8. 📐 QML Component Property Scoping

### ❌ The Anti-Pattern
Using relative parent traversal chains (`parent.parent.parent.parent.property`) in QML:
- Re-ordering QML items or wrapping controls inside `Item` or `RowLayout` alters the parent depth, producing `Cannot assign to non-existent property` errors at runtime.

### ✅ The Battle-Tested Solution
Assign explicit IDs to container items (`id: videoContainer`) and reference properties directly (`videoContainer.currentRotation`).

---

## 📋 Audit Checklist for Future Applications

| Category | Audit Check | Required Pattern |
| :--- | :--- | :--- |
| **Concurrency** | Are worker queues dequeued safely under lock? | Iterate fixed priority array, re-verify `!queue.isEmpty()` under lock right before `dequeue()`. |
| **Memory Mapping** | Is `mmap` fallback enabled for network drives? | Verify `isMapped()`; fall back to standard file I/O / hash cache if mapping fails. |
| **Ring Buffers** | Does ring buffer eviction handle full capacity? | Evict old entries only while `head <= tail && (head + requiredSize) > tail`; handle empty index. |
| **Qt Multimedia** | Is Qt 6 using bundled FFmpeg? | Set `qputenv("QT_MEDIA_BACKEND", "ffmpeg")` before `QApplication` instantiation. |
| **RAM Cache** | Do cache lookups return deep copies? | Use `img->copy()` so purges on other threads never destroy active rendering memory. |
| **FFmpeg Decoding** | Are scaling dimensions and image buffers guarded? | Validate `dstW > 0 && dstH > 0` and `!tmp.isNull()` before calling `sws_scale`. |
| **OS Shell APIs** | Are `QFileIconProvider` calls thread-safe? | Wrap in `try / catch` blocks; avoid blocking background queues on Shell COM calls. |
| **QML Scoping** | Are properties accessed via explicit IDs? | Use `videoContainer.currentRotation` instead of relative `parent.parent.parent` chains. |
