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

## 🌐 6. Network File I/O & Memory-Mapped (`mmap`) Fallbacks

### 💡 Core Principle
Never assume local filesystem speeds or memory-mapping support when accessing files. Always handle network share latencies, URL encoding, and filesystem restrictions gracefully.

### 🔍 Lessons Learned & Applied
- **Automatic `mmap` Fallback to Native Cache**:
  - *The Problem*: Memory-mapping a 1GB ring buffer file (`FileCache.mmap`) over SMB/network shares or restricted permissions raises OS page-fault exceptions (`STATUS_IN_PAGE_ERROR` `0xc0000006`).
  - *The Fix*: `FileCacheManager` verifies `isMapped()` after loading. If memory mapping fails, it automatically falls back to `QHashCacheDatabase` (Native in-memory index with standard file I/O).
- **Ring Buffer Capacity Eviction (`advanceHead`)**:
  - *The Problem*: When the 1GB ring buffer reached capacity after long-term use, an improper `head == tail` check caused an infinite eviction loop.
  - *The Fix*: Handled empty index states first and bounded eviction loops strictly to active overlapping regions (`!m_index.isEmpty() && head <= tail && (head + requiredSize) > tail`).
- **URL & Path Normalization**:
  - *The Problem*: Paths passed as `file:///I:/` or `file://192.168.1.10/` failed `QFileInfo::exists()` and corrupt cache keys.
  - *The Fix*: Sanitized `QUrl` inputs to native local/UNC file paths (`QUrl::toLocalFile`) and validated `fi.lastModified().isValid()` before converting timestamps.

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

## 📋 Comprehensive Audit Checklist

| Principle | Architectural Target | Implementation Rule |
| :--- | :--- | :--- |
| **SSOT** | Data Pipeline & UI Components | Use `ImageModel` as sole in-memory source of truth (`AlbumModel` consumes `ImageModel`). Standardize UI buttons in `StyledButton.qml`. |
| **RAII** | C-Libraries (FFmpeg, LibRaw, COM) | Encapsulate contexts in stack-allocated cleanup structs (`FFmpegCleanup`, `RawCleanup`). |
| **Modular Design** | Decoupled Infrastructure | Keep `TaskScheduler`, `AsyncImageProvider`, and `FileCacheManager` free of UI/model dependencies. |
| **Library Re-Use** | 3rd-Party API Wrappers | Centralize FFmpeg in `VideoThumbnailer` and OS functions in `DesktopHelper`. |
| **Thread Safety** | Concurrency & Worker Queues | Rate-limit heavy tasks via Semaphores; iterate fixed priority arrays under mutex lock when dequeuing. |
| **Network & Mmap** | File I/O & Ring Buffers | Verify `isMapped()`; fall back to `QHashCacheDatabase` on network shares. Bound `advanceHead` ring eviction loops. |
| **Codec Independence**| Qt 6 Multimedia | Set `qputenv("QT_MEDIA_BACKEND", "ffmpeg")` at startup. |
| **Defensive Memory** | RAM Caching & Decoders | Return `img->copy()` from cache lookups. Validate `dstW/dstH` and wrap OS Shell calls in `try/catch`. |
