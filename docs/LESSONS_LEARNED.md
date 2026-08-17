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

## 🌐 6. Persistent Memory-Mapped (`mmap`) Database & Zero-Copy Retrieval

### 💡 Core Principle
Disk caching for thumbnail media must be permanent, non-destructive, and scalable across multi-drive photo libraries. Never treat persistent disk caches like ephemeral circular FIFO queues.

### 🔍 Lessons Learned & Applied
- **Append-Only Auto-Growing Mmap Architecture (v3)**:
  - *The Problem*: An earlier iteration implemented `FileCache.mmap` as a fixed 1.0 GB circular ring buffer. Once 1GB was reached across large libraries (e.g. 45,000 files on D: + 40,000 files on C: = ~1.7 GB), writing Drive C silently wrapped around and erased Drive D's thumbnails, causing mysterious cache misses on scroll.
  - *The Fix*: Removed circular wrapping and eviction entirely. `FileCache.mmap` is now an append-only binary database that starts at 512 MB and automatically expands in 512 MB chunks as new media is indexed. All crawled thumbnails are permanent and preserved across restarts.
- **Zero-Copy Kernel Slicing**:
  - *The Principle*: Pointer dereferences directly into memory-mapped OS pages (`m_mappedData + offset`) provide instant binary slice retrieval with zero syscall overhead. The OS kernel transparently manages physical RAM page residency.
- **Filesystem Reconciliation & Compaction (`pruneStaleEntries`, `compact`)**:
  - *The Principle*: When files are deleted or moved on disk outside the app, the DB must mirror the filesystem. A background pass prunes stale index keys, and if orphaned space exceeds 30%, `compact()` rewrites the mmap file in-place to reclaim disk space.

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

---

## 📋 Comprehensive Audit Checklist

| Principle | Architectural Target | Implementation Rule |
| :--- | :--- | :--- |
| **SSOT** | Data Pipeline & UI Components | Use `ImageModel` as sole in-memory source of truth (`AlbumModel` consumes `ImageModel`). Standardize UI buttons in `StyledButton.qml`. |
| **RAII** | C-Libraries (FFmpeg, LibRaw, COM) | Encapsulate contexts in stack-allocated cleanup structs (`FFmpegCleanup`, `RawCleanup`). |
| **Modular Design** | Decoupled Infrastructure | Keep `TaskScheduler`, `AsyncImageProvider`, and `FileCacheManager` free of UI/model dependencies. |
| **Library Re-Use** | 3rd-Party API Wrappers | Centralize FFmpeg in `VideoThumbnailer` and OS functions in `DesktopHelper`. |
| **Thread Safety** | Concurrency & Worker Queues | Rate-limit heavy tasks via Semaphores; iterate fixed priority arrays under mutex lock when dequeuing. |
| **Mmap DB & Caching** | Persistent Storage & Zero-Copy | Auto-expanding append-only mmap log (v3). Zero ring-buffer evictions. Background filesystem reconciliation and compaction. |
| **Codec Independence**| Qt 6 Multimedia | Set `qputenv("QT_MEDIA_BACKEND", "ffmpeg")` at startup. |
| **Defensive Memory** | RAM Caching & Decoders | Return `img->copy()` from cache lookups. Validate `dstW/dstH` and wrap OS Shell calls in `try/catch`. |
