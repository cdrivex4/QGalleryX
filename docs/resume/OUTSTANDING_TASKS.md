# Outstanding Tasks & Known Issues

**Last Updated:** February 10, 2026

## 🔴 High Priority

### DNG Proprietary Compression Support ⏸️ DEFERRED
**Status:** Paused for future optimization  
**Issue:** DNG files with `PhotometricInterpretation=32803` take 120+ seconds to load  
**Root Cause:** LibRaw CPU-only processing, no GPU acceleration for Bayer demosaicing  
**Current Workaround:** Users should use corresponding JPG files  
**Future Solutions:**
1. Show embedded JPEG preview immediately, background-load full RAW
2. Implement GPU-accelerated demosaicing (custom shaders)
3. Cache pre-processed DNG files to disk
4. Detect problematic DNGs and skip with user notification

**Files Involved:**
- `src/AsyncImageProvider.cpp` (lines 378-415) - LibRaw processing
- `test_scrollbench/qml/PhotoViewerScrollBench.qml` - RAW loading strategy

**Performance Impact:**
- Current: 120+ seconds load time, FPS drops to 27
- Target: <3 seconds (preview) + background processing

---

## 🟡 Medium Priority

### Video Playback Polish
**Status:** Functional but needs refinement  
**Items:**
- Implement hardware-accelerated decoding validation
- Add codec support detection (H.265, AV1, VP9)
- Test HEVC rotation metadata handling
- Validate audio sync on all video formats

### Global Text-Based Filtering
**Status:** Planned  
**Files:** `test_scrollbench/qml/MainScrollBench.qml`, `test_scrollbench/src/ScrollBenchImageModel.cpp`  
**Needs:**
- Add search bar to UI (Main View & Album View)
- Implement efficient substring filtering on backend (Filename/Folder name)
- Ensure filter works across both Standard Grid and Semantic Views

---

## 🟢 Low Priority / Future Enhancements

### Format Support Expansion
**Items:**
- Test all 170+ formats in compatibility matrix
- Validate HEIC support on Windows
- Test edge cases for exotic formats (MNG, TGA, etc.)
- Document any unsupported format edge cases

### Performance Optimization
**Items:**
- Profile LibRaw processing pipeline
- Investigate multi-threaded Bayer demosaicing
- Optimize memory usage during RAW processing
- Reduce VRAM spikes during rapid zooming

### UI Polish
**Items:**
- Loading spinner for slow RAW files
- Progress indicator for background processing
- Better error messages for unsupported formats
- "Processing..." overlay for DNG files

---

## ✅ Recently Completed

### Scan, Caching, & Background Pre-fetching (Aug 2026)
- ✅ **QImage Memory Stride UI Crash Fix:** Fixed a massive memory alignment bug in `VideoThumbnailer` where FFmpeg generated pixel rows that were not properly 32-bit aligned for `QImage`. A previous agent failed to commit this change, resulting in `Format_RGB888` (24-bit alignment) causing out-of-bounds OpenGL texture uploads and UI lockups (black boxes). Resolved by properly allocating `QImage::Format_RGB32`, mapping FFmpeg's scalar output to `AV_PIX_FMT_BGRA`, and updating luminance pixel calculations to `QRgb`.
- ✅ **PhotoViewer Video Playback TypeError:** Fixed a Javascript `TypeError` in `PhotoViewer.qml` where the UI attempted to call `pauseBackgroundTasks()` and `resumeBackgroundTasks()` on the `DesktopHelper` object instead of the `ImageModel`. This bug prevented the background thumbnailer from pausing during video playback, causing background tasks to continue consuming CPU cycles while watching videos. Fixed by routing the calls to `root.model`.
- ✅ **FFmpeg HEIC Infinite Loop Lockup:** Fixed a critical infinite loop in `VideoThumbnailer` where FFmpeg's `av_read_frame` endlessly read non-video data packets (like HEIC tiles) but the `maxPackets--` decrement was trapped inside an `if (is_video)` block. This caused background threads to permanently lock up and exhaust the `s_videoSemaphore`, resulting in all subsequent UI thumbnail requests being blocked indefinitely. Fixed by moving the decrement outside the check so any 150 packets will abort the loop and release the thread.
- ✅ **UI Priority Re-queue Demotion Bug:** Fixed a massive "chicken and egg" bug where `High` priority UI Grid tasks hit the concurrent video decoder limit and re-queued themselves at `TaskScheduler::Low` priority. Because the background snail already saturated the decoders, this demotion instantly buried active viewport requests behind thousands of off-screen background tasks. Fixed by threading `taskPriority` through `processImageTask` so failed semaphore acquisitions preserve the original `High` UI priority.
- ✅ **QImage Memory Stride & UI Crash Fix:** Fixed a massive memory alignment bug in `VideoThumbnailer` where FFmpeg generated pixel rows that were not properly 32-bit aligned for `QImage`. When Qt attempted to upload these misaligned textures to the GPU, it resulted in out-of-bounds memory reads, completely freezing the UI thread and rendering glitched textures across the entire UI. Resolved by allocating a perfectly aligned `QImage` first and passing its buffer directly to FFmpeg's `sws_scale`.
- ✅ **Infinite Loop & UI Crash Fix:** Prevented `QDirIterator` from endlessly recursing through Windows junction points (e.g., `Application Data` symlinks) by enforcing the `QDir::NoSymLinks` filter. This resolved the multi-hour uptime scans, terminal log spam, and eventual Out-Of-Memory (OOM) UI crashes on aging hardware.
- ✅ **TaskScheduler UI Freeze Fix:** Refactored `TaskScheduler::addTask` deduplication from an `O(N)` linear array search into an instantaneous `O(1)` hash lookup (`QHash<QString, Priority>`). This resolved massive UI stuttering when the `Aggressive` precacher flooded the low-priority queue and the user scrolled rapidly.
- ✅ **Idle Background Precacher:** Hooked up the natively integrated `QTimer` inside `ImageModel` which acts as the `IdlePrecacheWorker`. It silently ticks and feeds off-screen images to the `TaskScheduler` at `Low` priority when the UI is idle (`hasImmediateTasks() == false`).
- ✅ **RAM-Bypass Precache Logic:** Confirmed `AsyncImageProvider::precache()` processes images fully in the background and writes directly to the `FileCacheManager` disk DB, entirely bypassing `QQuickPixmapCache` so it doesn't artificially bloat RAM usage.
- ✅ **Precache UI Toggle:** Added a dropdown in `Main.qml` to allow users to switch the precacher between "Off", "Idle (Throttled)", and "Aggressive (Max CPU)".
- ✅ **QML Layout Optimization:** Switched `ImageModel` staggered loading to use `beginResetModel()` for massive chunks (>500 items) to prevent QML `GridView` engine lockups when rendering 80,000+ files from NVMe SSDs (O(1) layout calculation vs O(N)).
- ✅ **FFmpeg TypeScript Choke Fix:** Removed `.ts` from video extensions list in `DesktopHelper` and `ImageModel`, preventing `VideoThumbnailer` from fatally stalling the `CPU_BOUND` thread pool by trying to decode thousands of Node `node_modules/*.d.ts` text files as Transport Streams.
- ✅ **DB Cache Exposure:** Hooked up `SettingsHelper` C++ methods to expose the memory-mapped `FileCache.mmap` path and disk usage to the UI, allowing the user to view and wipe the central DB cache via a dedicated "Nuke" button right inside the `Main.qml` Menu tab.

### UI Telemetry, Texture Cache Bypass, & RAW Feedback (June 2026)
- ✅ **Bespoke `FastImageItem` (Garbage Collector Bypass):** Implemented a custom `QQuickItem` that manually instantiates `QSGTexture` directly from `AsyncImageProvider` cache hits to circumvent QML's `QQuickPixmapCache` scavenging logic, completely eradicating the "No Date" layout corruption when viewport culling is disabled.
- ✅ **FastImage Coordinate & UI Fixes:** Corrected `QSGSimpleTextureNode` crop math which previously sampled a single half-pixel stretch across the viewport (the "1 colour" bug). Fixed hardcoded integer role drift to restore Video & RAW icons in the Semantic view. Placed the FastImage toggle directly on the Main UI for testing, and explicitly routed HEIC/HEIF files through `AsyncImageProvider` in the PhotoViewer to prevent direct-loading failures.
- ✅ **Hardware Capabilities Matrix:** Rewrote `detectCPUFeatures()` using runtime `__cpuid()` (GCC/MinGW) instead of compile-time macros, and iteratively appended FFmpeg hardware enumerations to accurately list available instruction sets and GPU media engines (e.g., SSE3, AVX2, QSV, CUDA, D3D11VA).
- ✅ **True IO Latency Polling:** Stripped `Component.onCompleted` from QML delegates and wired `AsyncImageProvider::s_totalWorkDuration` atomic counters directly into the `TelemetryMonitor` to ensure UI metrics bypass staging queues and accurately report pure disk read/decode execution time.
- ✅ **RAW Specific UI Feedback:** Added a glowing "Decoding RAW..." badge bound to `FastImageItem.isLoading` to explicitly signal high-cost LibRaw decodes compared to standard IO waits.
- ✅ **Full-Resolution Caching Fix:** Explicitly bound `sourceSize` on delegates in the Normal grid view to strictly cache 3KB thumbnails instead of memory-crushing 48MB full-resolution JPEG matrices.

### Media Loading & Type Detection (Dec 25, 2024)
- ✅ Centralized file type detection (`FileTypeRouter`)
- ✅ Fixed video/RAW badges in grid and semantic views
- ✅ Corrected role ID mappings across all QML views
- ✅ Photo Viewer image loading for standard formats
- ✅ Comprehensive format compatibility documentation (170+ formats)

### Performance & Security Architecture (June 2026)
- ✅ **FFmpeg Core Security Update:** Updated embedded FFmpeg libraries to resolve CVEs.
- ✅ **Concurrency Leak & Stall Recovery:** Implemented `QueueGuardState` atomic flags to prevent double-increments and wired up `checkStalls()` to rescue IO locks.
- ✅ **Video Thumbnail Threading:** Re-routed heavy FFmpeg decodes to the `CPU_BOUND` dynamically scaling thread pool to prevent `IO_BOUND` pool starvation.
- ✅ **Dynamic Hardware Accel:** Scaled `s_videoSemaphore` dynamically based on system hardware_concurrency and active GPU pixel formats.
- ✅ **Video Smart Seek:** Replaced brutal 3x CPU-intensive black-frame retry loops with an intelligent 15% Smart Seek offset for representative video thumbnails.
- ✅ **QML Viewport Failsafe:** Clamped QML layout math to prevent -1 or massive visible range requests during initialization, preventing `AsyncImageProvider` thread exhaustion.
- ✅ **Album View Integration:** Wired up Album detail view with Semantic Zoom, Date Scrubber, and dynamically scaling `ScrollBenchImageModel` instances.

### Previous Sessions
See `SESSION_HISTORY.md` for details on earlier work.

---

## 🐛 Known Issues

### Minor Bugs
1. **Date Scrubber year markers** - May not display for very large collections (>10k items)
2. **Semantic view grouping** - Week grouping may show incorrect boundaries
3. **Thumbnail memory** - Occasional VRAM spike when scrolling rapidly

### Build Warnings
- Qt policy QTP0004 warnings (cosmetic, can be ignored)
- clangd lint errors on FileTypeRouter (IDE only, compiles fine)

---

## 📋 Testing Checklist

When revisiting DNG support:
- [ ] Test standard DNG (non-proprietary) - should load in 2-3 seconds
- [ ] Test Apple ProRAW DNG - verify preview extraction
- [ ] Test Android mobile DNG - validate color accuracy
- [ ] Test proprietary compression DNG - measure load time
- [ ] Profile CPU usage during demosaicing
- [ ] Test FPS impact on various hardware
- [ ] Validate memory leak during repeated DNG loads

---

## 🔗 Related Documentation

- **Format Compatibility:** `/docs/FORMAT_COMPATIBILITY.md`
- **Session History:** `/docs/resume/SESSION_2024-12-25.md`
- **Code Architecture:** `/docs/ARCHITECTURE.md`
- **Threading Model:** `/docs/THREAD_HIERARCHY.md`

---

**Note:** This document is living - update as tasks are completed or new issues discovered.
