# Architectural Principles & Lessons Learned

**Project:** QGalleryX (High-Performance Image & Video Gallery)  
**Last Updated:** August 2026  
**Purpose:** Master architectural documentation of core engineering principles, design patterns, and squashed technical debt in QGalleryX. Serves as the definitive guide for future feature development, codebase maintenance, and auditing similar high-throughput media applications.

---

## 🏛 1. Single Source of Truth (SSOT) Architecture

### 💡 Core Principle
Never duplicate data pipelines, background scanners, or UI styling across components. Every state, data pipeline, and UI element must have one clear, authoritative owner.

### 🔍 Lessons Learned & Applied
- **Data Pipeline Unification (`ImageModel` vs `AlbumModel`)**:
  - *The Problem*: Originally, both `ImageModel` (main grid) and `AlbumModel` (album grid) performed independent filesystem walks. This caused double disk I/O, duplicate thread scheduling, and out-of-sync state when navigating folders.
  - *The Fix*: `ImageModel` was established as the **Single Source of Truth** for all scanned files in memory. `AlbumModel` now directly consumes `ImageModel` (`sourceModel: imageModel`), eliminating duplicate disk walks and thread contention.
- **UI Design System (`StyledButton.qml` & `MediaIcon.qml`)**:
  - *The Problem*: Buttons and icons were implemented ad-hoc with inline styles and OS text/emojis. Windows `Segoe UI` rendered emojis as blue 3D boxes, breaking visual consistency.
  - *The Fix*: Created `StyledButton.qml` (single source of truth for gallery controls) and `MediaIcon.qml` (flat 2D vector Canvas rendering for Play, Pause, Speaker, Mute, and Rotate icons), enforcing style guide consistency across the app.
- **Explicit Property Scoping**:
  - *The Problem*: Controls used relative parent traversal chains (`parent.parent.parent.parent.currentRotation`), which broke whenever QML layout structures were re-arranged.
  - *The Fix*: Bound controls directly to explicit component IDs (`videoContainer.currentRotation`).

---

## ⚙️ 2. RAII (Resource Acquisition Is Initialization)

### 💡 Core Principle
Encapsulate low-level C-style pointers, handles, and library contexts inside C++ RAII cleanup wrappers. Guarantees zero memory or handle leaks regardless of early returns, timeout interruptions, or unexpected exceptions.

### 🔍 Lessons Learned & Applied
- **FFmpeg Context Cleanup (`VideoThumbnailer::FFmpegCleanup`)**:
  - *The Problem*: Decoding video frames requires opening C contexts (`AVFormatContext`, `AVCodecContext`, `SwsContext`, `AVFrame`, `AVPacket`). Manual cleanup scattered across early returns led to dangling handles.
  - *The Fix*: Wrapped all FFmpeg pointers inside a stack-allocated RAII cleanup struct:
    ```cpp
    struct FFmpegCleanup {
        AVFormatContext *fmtCtx = nullptr;
        AVCodecContext *codecCtx = nullptr;
        AVFrame *frame = nullptr;
        SwsContext *swsCtx = nullptr;
        AVPacket *packet = nullptr;

        ~FFmpegCleanup() {
            if (swsCtx) sws_freeContext(swsCtx);
            if (frame) av_frame_free(&frame);
            if (packet) av_packet_free(&packet);
            if (codecCtx) avcodec_free_context(&codecCtx);
            if (fmtCtx) avformat_close_input(&fmtCtx);
        }
    } cleanup;
    ```
- **LibRaw Struct Cleanup (`RawCleanup`)**:
  - *The Problem*: RAW image decoding uses `LibRaw` structs and memory buffer allocations (`dcraw_make_mem_thumb`).
  - *The Fix*: Encapsulated `LibRaw::recycle()` and `LibRaw::dcraw_clear_mem()` inside RAII destructors.

---

## 🧩 3. Modular & Loosely Coupled Design

### 💡 Core Principle
Keep system components isolated with clean boundaries. Infrastructure components should operate independently of business domain models and UI layers.

### 🔍 Lessons Learned & Applied
- **Decoupled Task Infrastructure**:
  - `TaskScheduler` manages priority thread queues (`Immediate`, `High`, `Normal`, `Low`) without any knowledge of images, videos, or UI models.
  - `AsyncImageProvider` manages image decoding and caching without knowing about QML view states or grid layouts.
  - `FileCacheManager` handles disk-level caching transparently without binding to scanner logic.
- **Safer Testing & Refactoring**: Because components do not share internal state or tight dependencies, refactoring `TaskScheduler` or replacing `FileCacheManager` backends does not break UI or model logic.

---

## 📦 4. Library Wrapping & Re-Use

### 💡 Core Principle
Wrap standard third-party libraries (FFmpeg, LibRaw, DirectX/DXGI, PDH, Windows Shell) into dedicated helper classes rather than scattering raw API calls throughout the codebase.

### 🔍 Lessons Learned & Applied
- **Dedicated Hardware & System Helpers**:
  - `VideoThumbnailer`: Encapsulates all FFmpeg C calls for frame extraction, time scaling, pixel conversion, and black-frame detection.
  - `DesktopHelper`: Centralizes OS file-type identification, network drive detection, and shell integrations.
  - `SystemMonitor`: Wraps Windows PDH (Performance Data Helper), DXGI video memory queries, and process memory calls into clean Qt signals.

---

## 🔒 5. Thread Safety & Concurrency Control

### 💡 Core Principle
Protect shared resources with explicit synchronization primitives (mutexes, atomic flags, semaphores) to prevent thread contention, CPU starvation, or GPU VRAM overload.

### 🔍 Lessons Learned & Applied
- **Semaphore Rate-Limiting (`s_videoSemaphore`, `s_rawSemaphore`)**:
  - *The Problem*: Rapid scrolling threw dozens of concurrent RAW/Video decoding tasks to worker threads, exhausting system RAM and GPU decoding pipelines.
  - *The Fix*: Limited concurrent video frame extractions (`s_videoSemaphore(4)`) and RAW decodes (`s_rawSemaphore(2)`).
- **Thread-Safe Queue Worker Loops (`TaskScheduler`)**:
  - *The Problem*: Iterating raw `QMap` iterators or calling `dequeue()` on an empty `QQueue` under multi-threaded execution caused `0xc0000005` Access Violations in `Qt6Core.dll`.
  - *The Fix*: Pre-allocate fixed priority arrays (`{Immediate, High, Normal, Low}`) and re-verify `!queue.isEmpty()` under mutex lock at the exact moment of `dequeue()`.

---

## 🌐 6. Network File I/O, Persistent Mmap Database & Zero-Copy Retrieval

### 💡 Core Principle
Never assume local filesystem speeds or memory-mapping support when accessing files. Always handle network share latencies, URL encoding, and filesystem restrictions gracefully, while ensuring persistent disk storage is permanent, non-destructive, and scalable across multi-drive photo libraries.

### 🔍 Lessons Learned & Applied
- **Evolution from Ring Buffer (v2) to Append-Only Expanding Mmap Database (v3)**:
  - *The Initial Battle (v1/v2 Ring Buffer)*: When `FileCache.mmap` was initially built as a 1.0 GB circular ring buffer, hitting capacity after long-term use caused an infinite eviction loop due to an improper `head == tail` check. We fixed the infinite loop by bounding eviction strictly to active overlapping regions (`!m_index.isEmpty() && head <= tail && (head + requiredSize) > tail`).
  - *The Deeper Architectural Battle (v2 Ring Buffer Flaw)*: Even with the eviction loop fixed, a circular ring buffer inherently capped total library thumbnail storage at 1GB. Once a user crawled Drive D (45,000 files, ~900MB) and then crawled Drive C (40,000 files, ~800MB), writing Drive C silently wrapped around and erased Drive D's thumbnails, producing unexpected cache misses when scrolling back through Drive D.
  - *The Definitive Resolution (v3 Append-Only Mmap)*: Removed circular wrapping and eviction entirely. `FileCache.mmap` is now an append-only binary database that starts at 512 MB and automatically expands in 512 MB chunks as new media is indexed. All crawled thumbnails are permanent, indexed $O(1)$, and preserved across restarts.
- **Zero-Copy Kernel Slicing**:
  - *The Principle*: Pointer dereferences directly into memory-mapped OS pages (`m_mappedData + offset`) provide instant binary slice retrieval with zero syscall overhead. The OS kernel transparently manages physical RAM page residency.
- **Filesystem Reconciliation & Compaction (`pruneStaleEntries`, `compact`)**:
  - *The Principle*: When files are deleted or moved on disk outside the app, the DB must mirror the filesystem. A background pass prunes stale index keys, and if orphaned space exceeds 30%, `compact()` rewrites the mmap file in-place to reclaim disk space.
- **Automatic `mmap` Fallback to Native Cache**:
  - *The Problem*: Memory-mapping a large file (`FileCache.mmap`) over SMB/network shares or restricted permissions raises OS page-fault exceptions (`STATUS_IN_PAGE_ERROR` `0xc0000006`).
  - *The Fix*: `FileCacheManager` verifies `isMapped()` after loading. If memory mapping fails, it automatically falls back to `QHashCacheDatabase` (Native in-memory index with standard file I/O).
- **URL & Path Normalization**:
  - *The Problem*: Paths passed as `file:///I:/` or `file://192.168.1.10/` failed `QFileInfo::exists()` and corrupted cache keys.
  - *The Fix*: Sanitized `QUrl` inputs to native local/UNC file paths (`QUrl::toLocalFile`), collapsed double slashes with `QDir::cleanPath()`, and converted all paths to native lowercase separators (`QDir::toNativeSeparators().toLower()`) to ensure 100% hash key consistency across all engines (MFT, QDirIterator, QML).

---

## 🎬 7. Qt 6 Multimedia & System Codec Independence

### 💡 Core Principle
Never rely on host OS media codecs for critical application features.

### 🔍 Lessons Learned & Applied
- **Forced FFmpeg Backend (`QT_MEDIA_BACKEND`)**:
  - *The Problem*: Qt 6 `QtMultimedia` defaults to Windows Media Foundation (WMF). On Windows systems without installed Microsoft Store extension codecs (e.g. AV1 Video Extension), QML `MediaPlayer` failed on AV1, VP9, WebM, and MKV files.
  - *The Fix*: Set `qputenv("QT_MEDIA_BACKEND", "ffmpeg")` during startup, cutting WMF dependencies and using bundled FFmpeg DLLs end-to-end for 100% self-contained playback.

---

## 🛡️ 8. Defensive Coding & Deep-Copy Protection

### 💡 Core Principle
Validate all external inputs, guard against null pointers, and isolate memory shared across threads.

### 🔍 Lessons Learned & Applied
- **Deep-Copy Cache Lookups**:
  - *The Problem*: Returning shallow shared `QImage` references from RAM cache allowed `m_cache.clear()` on RAM monitor threads to delete `QImagePrivate` refcounts while rendering threads were reading them.
  - *The Fix*: `AsyncImageProvider::getCachedImage` returns `img->copy()` (deep copy), isolating rendering memory from cache purges.
- **Corrupt Media & Shell COM Guards**:
  - *The Problem*: Corrupt videos with `dstW <= 0` or zero-byte buffers caused `sws_scale` to dereference `nullptr`. Calling `QFileIconProvider` on network files from worker threads without exception handling triggered Shell COM RPC crashes.
  - *The Fix*: Validated `dstW > 0 && dstH > 0` and `!tmp.isNull()` before `sws_scale`, guarded `constScanLine` pixel loops, and wrapped `QFileIconProvider` in `try / catch` blocks.

---

## 🧠 9. User-Caught Architectural Misconceptions & AI Bug Log

A critical historical log of instances where the User identified system flaws, misconceptions, and architectural bugs that AI coding assistants missed or introduced:

1. **The Circular Ring Buffer Flaw (User-Caught):**
   - *AI Misconception*: AI assumed a fixed 1.0 GB circular ring buffer was acceptable for a thumbnail cache.
   - *User Discovery*: User noticed that after crawling 80,000+ files across C: and D: drives, scrolling previously crawled folders still produced cache misses. User pointed out that disk cache is meant to house ALL parsed data permanently across all drives, not overwrite old drives in a circular loop.
   - *Resolution*: AI eliminated the ring buffer wrapping and replaced it with auto-growing append-only mmap storage.

2. **Silent Cache Hits & Logging Asymmetry (User-Caught):**
   - *AI Misconception*: AI left L1 RAM hits and L2 Disk hits completely silent while logging every cache miss, creating a false impression in the terminal that the cache was completely failing.
   - *User Discovery*: User asked why the console only printed misses despite the crawler finishing.
   - *Resolution*: Pointed out the 2,052 silent hits in telemetry and added visible, throttled L1/L2 hit logging.

3. **In-Memory Index Startup Desync (User-Caught):**
   - *AI Misconception*: AI checked `m_knownKeys` in `isCached()` before `rebuildKeyIndex()` had synchronized it with `MmapCacheDatabase::m_index`.
   - *User Discovery*: User noticed that restarting the app caused the crawler to misjudge whether files were already cached.
   - *Resolution*: Unified `isCached()` to query `m_db->contains(key)` directly as the authoritative single source of truth.

4. **OSD vs Menu Telemetry Desynchronization (User-Caught):**
   - *AI Misconception*: AI had different code paths, formats, and missing Rebuild/Nuke controls between the On-Screen Display (OSD) and the Settings Menu.
   - *User Discovery*: User spotted that the OSD was displaying mismatched data and lacked the re-crawl/rebuild cache triggers present in the Menu.
   - *Resolution*: Connected both the Menu and OSD to the exact same `appSettings.getTrackedRootPathStats()` backend and added a unified `Rebuild Cache / Re-Crawl` button to the OSD.

5. **Cross-Drive Global Cache Wiping & Mid-Run Compaction Crash (User-Caught):**
   - *AI Misconception*: AI implemented `pruneStaleEntries` by checking the entire database against only the active folder's file list, assuming any key not present belonged to deleted files. This caused scanning drive `I:\` to immediately wipe all 200,000+ cached entries for `C:\` and `D:\`, followed by a compaction that unmapped the active `FileCache.mmap` under running worker threads, causing a `0xc0000005` crash.
   - *User Discovery*: User reported instant cache misses on previously parsed drives and a crash when switching directories.
   - *Resolution*: Scoped `pruneStaleEntries(folderPrefix, ...)` strictly to the scanned path prefix, added `QFile::exists` validation, and eliminated unsafe mid-run mmap unmapping.

6. **File Type Descrepancy & TypeScript `.ts` vs MPEG-TS Collision (User-Caught):**
   - *AI Misconception*: Scanner, crawler, filmstrip, and decoder used hardcoded, divergent extension lists (e.g. missing `heif`, `tif`, `cr3`), and blindly treated `.ts` files as MPEG Transport Stream videos.
   - *User Discovery*: User spotted log misses: `[Cache] MISS - decoding Video/HEIC "use-history.ts"`.
   - *Resolution*: Established `DesktopHelper` as the authoritative Single Source of Truth (`supportedExtensions()`, `supportedNameFilters()`), and added `0x47` header sync-byte verification to reject TypeScript/text files at zero CPU cost.

---

## 🛠️ 10. AI-Discovered Bugs & Technical Deep-Dives

Key systemic bugs and low-level concurrency failures diagnosed and resolved by AI analysis during codebase audits:

1. **Task Deduplication Key Poisoning ("The Black Box Bug"):**
   - *Discovery*: Traced a pervasive bug where grid cells intermittently rendered as empty black boxes. When QML retried a failed or slow thumbnail request with `?retry=1`, the un-stripped query string formed a mismatched deduplication key in `m_pendingTasks`. If the initial decode failed, attached child responses were dropped without executing their callbacks, permanently leaving the UI item in an un-rendered state.
   - *Fix*: Standardized URL parameter stripping across `AsyncImageProvider` prior to task deduplication mapping and ensured fallback error signals always emit `handleDone(QImage())`.

2. **QImageReader Corrupted EXIF / Non-Standard JPEG Silent Failures:**
   - *Discovery*: Certain JPEG files with non-standard EXIF markers or malformed ICC profiles consistently failed in Qt's `QImageReader` with generic read errors, despite being completely playable in FFmpeg.
   - *Fix*: Implemented an automatic transparent fallback in `AsyncImageProvider`: if `QImageReader` fails, the task automatically routes the file through `VideoThumbnailer` (FFmpeg software decode), recovering the image seamlessly.

3. **TaskScheduler Queue Dequeue Iterator Invalidation (`0xc0000005` Access Violation):**
   - *Discovery*: Multi-threaded stress testing revealed that calling `dequeue()` on an empty `QQueue` or iterating `QMap` structures while worker threads were modifying tasks caused pointer corruption in `Qt6Core.dll`.
   - *Fix*: Replaced dynamic map lookups with a fixed, pre-allocated array of priority queues (`{Immediate, High, Normal, Low}`) and enclosed the dequeue operation in strict atomic emptiness re-checks under mutex lock.

4. **Shallow `QImage` Reference Purge Race (Use-After-Free):**
   - *Discovery*: `QImage` utilizes implicit sharing (copy-on-write). When the background RAM monitor thread called `m_cache.clear()`, the refcount of the internal image buffer dropped to zero while the QML render thread was concurrently reading the pixel scanlines.
   - *Fix*: Mandated explicit `img->copy()` deep-copy returns across all cache lookup boundaries.

5. **Multi-Window Teardown & Dangling Singleton Signal Crash (`Qt6Core.dll!+0x244f2c`):**
   - *Discovery*: When secondary windows or models were opened and subsequently closed, global singletons (`FileCacheManager::instance()`) retained lambda slot pointers connected without a context `QObject`. On the next cache clear event, the singleton dispatched signals into freed `ImageModel` instances, resulting in access violation `0xc0000005`.
   - *Fix*: Mandated passing `this` to all singleton signal connections (`connect(&singleton, &Singleton::signal, this, [this]() { ... })`) ensuring automatic disconnection upon object destruction, and implemented `ImageModel::~ImageModel()` with active cancellation tokens (`m_aliveToken`).

6. **Mmap Database Incremental Allocation (16MB) & 30% Compaction:**
   - *Discovery*: Preallocating 512MB memory-mapped chunks caused small directories to bloat disk usage immediately, while accumulating dead entries from deleted files slowly bloated the file size.
   - *Fix*: Reduced `MMAP_GROW_CHUNK` to 16MB and added automated defragmentation/compaction (`compact()`) whenever stale/pruned records exceed 30% of total payload.

7. **Corrupt Media FFmpeg Demuxer Assertions (`oggdec.c:964` abort):**
   - *Discovery*: Recovered audio files with invalid stream headers triggered FFmpeg internal demuxer assertions and process aborts when parsed by container probes.
   - *Fix*: Restricted file enumeration strictly to visual media, added `err_detect=ignore_err` flags, and added procedural fallback rendering in `VideoThumbnailer`.

8. **Windows SEH Forensic Black Box vs Lazy Error Handling:**
   - *Discovery*: Using inline `__try / __except` blocks in normal code is lazy programming that masks root causes and breaks C++ RAII object destruction.
   - *Fix*: Installed top-level `SetUnhandledExceptionFilter` and `std::set_terminate` purely as a forensic recorder at the OS level, capturing minidumps (`crash_dump.dmp`) and diagnostic logs without compromising C++ architectural guarantees.

9. **Synchronous GUI Thread Decompression Stalls (L2 MMAP Hits):**
   - *Discovery*: `AsyncImageProvider::requestImageResponse()` was synchronously executing `diskImg.loadFromData(mmapData)` inside the main event loop. While mmap pointer reads are instant ($0.02\text{ms}$), decompressing 30 JPEGs in a row during a fling froze the GUI thread for $300\text{ms}+$.
   - *Fix*: Strict separation of concerns: only uncompressed L1 RAM hits return synchronously on the GUI thread; all L2 MMAP disk hits are dispatched to `TaskScheduler` worker threads for parallel decompression, guaranteeing $0\text{ms}$ main-thread latency.

10. **Directional Trajectory Lookahead vs Discontinuous Scrubber Dragging:**
    - *Discovery*: Touch flings followed continuous momentum allowing symmetric lookahead to pre-decode approaching tiles, whereas dragging the timeline scrubber jumped across years, flooding the task queue with abandoned requests for intermediary positions.
    - *Fix*: Enhanced `ViewportGovernor` with trajectory-biased lookahead ($+2\times \text{count}$ forward during downward scrolls, $-2\times \text{count}$ backward during upward scrolls) and unified `DateScrubber.qml` to feed continuous delta updates into the exact same pipeline.

11. **Direct3D 11 Zero-Copy Video Swapchain vs CPU-Bound Frame Transfers:**
    - *Discovery*: A custom FFmpeg media player decoded frames on GPU but used `av_hwframe_transfer_data()` to copy decoded 4K/1080p frames back to CPU RAM, ran software `sws_scale` pixel conversion, and re-uploaded them to GPU for `QVideoSink`. This GPU $\rightarrow$ CPU RAM $\rightarrow$ Software Scaler $\rightarrow$ GPU roundtrip pegged CPU usage at 100%, starved the audio clock, and caused micro-stuttering. Furthermore, setting `QT_FFMPEG_DECODING_HW_DEVICE_TYPES = "none"` in `main.cpp` crippled hardware video decoding on iGPUs like Intel HD 630 (QuickSync).
    - *Fix*: Upgraded to Qt 6's native `MediaPlayer` + `AudioOutput` + `VideoOutput` with `Direct3D11` / WMF hardware acceleration. Decoded video frames remain in GPU VRAM and blit directly into the Direct3D 11 swapchain with zero CPU copies and hardware-synced audio clocks.

12. **Deterministic Single-Pass Metadata Sorting vs Date Migration Snapping:**
    - *Discovery*: When scanning large folders/drives, files without embedded filename dates were initially assigned fallback timestamps, causing them to migrate to their real dates during Pass 2 EXIF extraction. This date shift forced a second `std::sort` and multiple `beginResetModel()` calls that scrambled the visible grid, destroyed delegate layouts, and reset scroll positions.
    - *Fix*: Captured exact `lastModified()` timestamps directly from `QDirIterator`'s native `WIN32_FIND_DATA` during the initial discovery pass (Pass 1). Suppressed all `modelReset` signals in Pass 1 and Pass 2 for cached directories, ensuring the grid layout is 100% stable on Frame 1.

13. **QML Off-Screen Delegate Allocation Limits (`displayMargin` Overload):**
    - *Discovery*: Adding additive `displayMarginBeginning: 400` + `displayMarginEnd: 800` + `cacheBuffer: 1200` to `ListView` forced Qt Quick to instantiate over 800 active off-screen QML row items and timers. Frame layout times jumped from 8ms to 35ms, causing heavy UI sluggishness.
    - *Fix*: Reduced `cacheBuffer` to a bounded `400px` and removed `displayMargin` overrides, dropping active QML delegate count by 70% and restoring a solid 120 FPS frame rate.

14. **TypeScript (`.ts`) Extension Filter Trap:**
    - *Discovery*: Media file filters included `"*.ts"` for MPEG Transport Stream video containers. On development machines, this caused the scanner on `C:\` to index over 50,000 `.ts` TypeScript source code files across `node_modules` and `.git`, which failed image decode and stalled the pipeline.
    - *Fix*: Removed `"ts"` from default media extension filters, added `DesktopHelper::isSupportedFile()` validation, and added directory skip filters for `.git`, `node_modules`, `AppData/Local/Packages`, and `Windows/WinSxS`.

15. **Integrated GPU (iGPU) Shared Memory Bandwidth Conservation:**
    - *Discovery*: Integrated GPUs (e.g. Intel HD Graphics 630 on i3-7100) have no dedicated VRAM and share system DDR4 bandwidth ($\sim 19\text{ GB/s}$). Setting `QML_IMAGE_CACHE_SIZE = "1024"` (1 GB) and running scalar software BC1 compression on CPU worker threads saturated memory bus bandwidth, starving the iGPU display controller.
    - *Fix*: Set `QML_IMAGE_CACHE_SIZE = "256"`, used fast SIMD JPEG writes ($<10\%$ CPU) for disk caching, and assigned `THREAD_PRIORITY_BELOW_NORMAL` to CPU worker threads.

16. **Dynamic Drop Model Promotion & Explicit Window Scope:**
    - *Discovery*: Drag-and-drop opened a 15-neighbor file slice for 0ms initial navigation, but a missing `pendingFileToOpen` property on `ApplicationWindow` threw a JavaScript TypeError that prevented scanning the parent folder.
    - *Fix*: Declared `pendingFileToOpen` explicitly on `ApplicationWindow` and enabled dynamic model promotion so `PhotoViewer` seamlessly switches to the full folder dataset once background scanning completes.

17. **Never Use `static` Local Variables for Per-Instance State (C1):**
    - *Discovery*: `onDirectoryChanged()` used `static QTimer *debounceTimer = nullptr` — a function-level static shared across ALL `ImageModel` instances. In multi-window mode, the second window reused the first window's heap-allocated timer. After the first window closed, the timer callback fired into a destroyed `ImageModel`, causing a use-after-free crash.
    - *Fix*: Replaced the `static QTimer*` with a value-type member `QTimer m_debounceTimer` in the header, initialized with `setSingleShot(true)` in the constructor. Rule: **never use static local variables for state that belongs to an instance.**

18. **Qt Signals Must Always Be Emitted from the GUI Thread (R1):**
    - *Discovery*: `emit this->crawlerProgressChanged()` was called directly from inside a `TaskScheduler` CPU worker thread lambda. Qt QML property bindings connected to this signal were then evaluated on the wrong thread, causing undefined behaviour and potential UI corruption.
    - *Fix*: Wrapped the emit in `QMetaObject::invokeMethod(this, [...]() { emit crawlerProgressChanged(); }, Qt::QueuedConnection)`. Rule: **any signal connected to QML properties must be emitted from the GUI thread.**

19. **Keep All Parallel Lists in Sync on Mutation (C4):**
    - *Discovery*: `ImageModel` maintains two parallel lists: `m_images` (the filtered/displayed list) and `m_allItems` (the unfiltered master). `deleteSelected()` only removed items from `m_images` but not `m_allItems`. After the next filter query change, `applyFilter()` re-populated `m_images` from `m_allItems`, causing deleted images to "reappear".
    - *Fix*: `deleteSelected()` now removes from both lists using a `QSet<QString>` of deleted paths. Rule: **any mutation of a derived/filtered list must also update the source-of-truth list.**

20. **All Async Lambdas Capturing `this` Must Have a Lifetime Guard (C2):**
    - *Discovery*: `setFilterQuery()` dispatched a `QThreadPool` task that captured `this` directly. If the `ImageModel` was destroyed (e.g. closing a folder window while filtering), the background lambda or its inner `QueuedConnection` callback would dereference a dangling pointer.
    - *Fix*: Applied the established `alive = m_aliveToken` + `QPointer<ImageModel> safeThis(this)` pattern consistently. Rule: **every async lambda that touches `this` must guard with both the alive token and a `QPointer`.**

21. **Normalize Cache Keys Before Insert (L8):**
    - *Discovery*: On L2 disk cache hits, the decoded image was inserted into L1 RAM cache using the original `id` parameter (which may still contain `file:///` prefix or forward slashes). The key inserted by `insertCachedImage(id, ...)` differed from the key `normalizeRamKey()` would produce on lookup — creating a second, never-matchable entry in `QCache`, causing every L2 hit to also become an L1 miss.
    - *Fix*: Changed to `insertCachedImage(path, ...)` where `path` is the fully-normalized native-separator form. Rule: **always use the fully-normalized path when writing to a cache; raw QML-side paths must never be used as cache keys.**

22. **QML Type Detection Must Delegate to C++ (Q1/D3):**
    - *Discovery*: `PhotoViewer.qml` had a `isVideoFile()` function with a hardcoded extension list that diverged from `DesktopHelper::staticGetFileType()`. It was missing `.vob`, `.wmv`, `.ogg`, `.mp3`, and other audio/video formats. Files with these extensions showed the image viewer instead of the media player.
    - *Fix*: Changed `isVideoFile()` to call `desktopHelper.getFileType(path) === 2` (the authoritative C++ source), with the extension list as a fallback only when `desktopHelper` is unavailable. Rule: **never duplicate file-type or format logic in QML — always delegate to the C++ helper.**

---

## 📋 Comprehensive Audit Checklist

| Principle | Architectural Target | Implementation Rule |
| :--- | :--- | :--- |
| **SSOT** | Data Pipeline & UI Components | Use `ImageModel` as sole in-memory source of truth (`AlbumModel` consumes `ImageModel`). Standardize UI buttons in `StyledButton.qml`. |
| **RAII** | C-Libraries (FFmpeg, LibRaw, COM) | Encapsulate contexts in stack-allocated cleanup structs (`FFmpegCleanup`, `RawCleanup`). |
| **Modular Design** | Decoupled Infrastructure | Keep `TaskScheduler`, `AsyncImageProvider`, and `FileCacheManager` free of UI/model dependencies. |
| **Library Re-Use** | 3rd-Party API Wrappers | Centralize FFmpeg in `VideoThumbnailer` and OS functions in `DesktopHelper`. |
| **Thread Safety** | Concurrency & Worker Queues | Rate-limit heavy tasks via Semaphores; iterate fixed priority arrays under mutex lock when dequeuing. Signals to QML always via `QueuedConnection`. |
| **Mmap DB & Caching** | Persistent Storage & Zero-Copy | Auto-expanding append-only mmap log (v3). Zero ring-buffer evictions. Background filesystem reconciliation and compaction. |
| **Codec Independence**| Qt 6 Multimedia | Set `qputenv("QT_MEDIA_BACKEND", "ffmpeg")` at startup. |
| **Defensive Memory** | RAM Caching & Decoders | Return `img->copy()` from cache lookups. Validate `dstW/dstH` and wrap OS Shell calls in `try/catch`. |
| **No Static Instance State** | Multi-Window Safety | Never use `static` local variables for per-instance data. Use member variables or `QPointer` guards. |
| **Parallel List Sync** | Model Mutations | Any delete/add to a filtered list must also mutate the master (`m_allItems`) list. |
| **Async Lifetime Guards** | Thread Safety | All `QThreadPool`/`TaskScheduler` lambdas that touch `this` must capture `alive` + `QPointer<T> safeThis`. |
| **QML Delegation** | Type Detection | QML must never duplicate C++ logic. Use `desktopHelper.getFileType()` not hardcoded extension lists. |
