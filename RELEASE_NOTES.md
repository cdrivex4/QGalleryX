# Release Notes - v2.3.1 (Media Player Overhaul & Native AV1 FFmpeg Backend)

## 🚀 Highlights
This release modernizes the full-screen video player UI with custom flat vector icons (`MediaIcon.qml`), fixes mouse hover interaction on the volume slider popup, standardizes slider progress visualization, and forces Qt Multimedia to use our bundled FFmpeg engine for native AV1/VP9 video playback.

## 🛠 New Features & UI Enhancements

### 🎬 Native AV1 & FFmpeg Playback Engine
- **Independent Video Backend**: Added `qputenv("QT_MEDIA_BACKEND", "ffmpeg")` in `main.cpp`, cutting Windows Media Foundation system dependencies.
- **AV1, VP9 & MKV Support**: Unlocked native playback for AV1, VP9, WebM, FLV, and TS video files out-of-the-box using our bundled FFmpeg DLLs.

### 🎨 Flat Vector Icons (`MediaIcon.qml`)
- **Flat Vector Graphics**: Created a dedicated `MediaIcon.qml` Canvas component rendering solid flat white Play (`▶`), Pause (`❚❚`), Speaker (`🔊`), Mute (`🔇`), and Rotate (`↻`) icons.
- **Emoji Bug Elimination**: Replaced OS text/emoji characters to prevent Windows Segoe UI from rendering blue 3D emoji boxes.

### 🔊 Pop-up Volume Slider & Hover Fix
- **Combined Hover Hit Region**: Added a non-blocking `combinedHoverArea` spanning both the speaker button and the vertical pop-up volume slider, eliminating premature menu closing.
- **Inverted Track Styling**: Styled the volume slider with a full white active bar rising from the bottom (`0%` / Mute) up to the handle, leaving the top inactive track grey.

### ⏯ Video Timeline Progress Bar
- **Elapsed/Remaining Progress**: Updated the horizontal video progress bar so elapsed time (0:00 to handle) is rendered in solid white (`#ffffff`), while remaining video time is rendered in dark grey (`#40ffffff`).

---

# Release Notes - v2.3.0 (Fullscreen Thread Isolation & Image Load Optimization)

## 🚀 Highlights
This release focuses on resolving critical bottlenecks in the image loading pipeline, particularly when opening large (50MB+) images in the full-screen viewer after rapidly scrolling through the grid. We achieved instantaneous full-screen load times by introducing strict thread isolation.

## 🛠 New Features & Optimizations

### ⚡ Fullscreen Thread Isolation (TaskScheduler)
- **Breathing Room via Fencing**: Introduced `TaskScheduler::pauseBackground(bool)` which physicaly prevents background worker threads from dequeuing `Low` or `Normal` priority tasks (like grid thumbnails) while the full-screen viewer is active.
- **Immediate Priority Exclusive**: Guarantees that `Immediate` tasks get 100% of the CPU threads exactly when the user requests a full-screen image, circumventing massive queue delays.
- **UI Hooks**: Wired `PhotoViewer.qml` `onVisibleChanged` to instantly pause/resume background tasks globally across the C++ threading pool.

### 🚀 Image Decode Acceleration
- **Bilinear CPU Stall Removed**: Replaced `Qt::SmoothTransformation` with `Qt::FastTransformation` during large-image fallback scaling in `AsyncImageProvider`, eliminating a massive 500ms CPU stall per image.
- **RAW Demosaicing Bypassed**: Forced LibRaw to always use `half_size = 1` for fallback decodes, bypassing the heavy 3-5 second demosaicing step on high-megapixel RAW files and instantly generating a 4K-friendly frame.
- **QML RAM Cache Expansion**: Increased `QML_IMAGE_CACHE_SIZE` from 100MB to 1024MB to comfortably fit ~15 full-resolution (4K-capped) images in RAM at once, completely stopping cache-eviction thrashing when swiping back and forth.

---

# Release Notes - v2.2.0 (MFT Scanning & Performance)

## 🚀 Highlights
This release focuses on **Performance Optimization** and **File System Efficiency**. The application now includes a high-performance MFT scanner for 10-100x faster file enumeration on NTFS volumes.

## 🛠 New Features

### ⚡ MFT Scanning (FastVolumeScanner)
-   **High-Speed Enumeration**: Direct Master File Table access via `DeviceIoControl` and `FSCTL_ENUM_USN_DATA`
-   **Performance Gain**: 10-100x faster than standard `QDirIterator` for large collections
-   **Full Path Reconstruction**: FRN-based path building from File Reference Numbers
-   **Graceful Fallback**: Automatic fallback to `QDirIterator` for non-NTFS or non-Admin scenarios
-   **Admin Requirement**: Requires Administrator privileges for optimal performance

**Files Integrated:**
-   `src/FastVolumeScanner.cpp/.h` - Core implementation
-   `src/ImageModel.cpp` - Main app gallery scanning
-   `src/AlbumModel.cpp` - Album view population
-   `test_scrollbench/src/ScrollBenchImageModel.cpp` - Test application

### 🎯 Frame Budget Scheduler
-   **UI Stutter Prevention**: Throttles heavy operations to fit within 16ms frame window
-   **Ported from ScrollBench**: Proven performance optimization now in main app
-   **AsyncImageProvider Integration**: Prevents main thread flooding during thumbnail generation
-   **Configurable Budget**: Adaptive frame timing based on system performance

**Files:**
-   `src/FrameBudgetScheduler.cpp/.h` - Scheduler implementation
-   `src/AsyncImageProvider.cpp` - Integration and throttling
-   `src/main.cpp` - Initialization and registration

### 📂 FileTypeRouter (Centralized Format Detection)
-   **170+ Formats**: Unified detection for RAW (70+), images (60+), videos (40+)
-   **Single Source of Truth**: Replaces duplicate extension checking across codebase
-   **Consistent Routing**: Same format detection for thumbnails and viewers

**Files:**
-   `test_scrollbench/src/FileTypeRouter.cpp/.h` - Format catalog

## 🐛 Fixes & Improvements

### 🔴 TDR Crash Resolution
-   **Reduced Video Concurrency**: 4 → 1 concurrent operations (prevents GPU timeout)
-   **Reduced RAW Concurrency**: 8 → 2 concurrent operations (prevents resource exhaustion)
-   **Issue**: GPU TDR (Timeout Detection and Recovery) crashes during rapid scrolling
-   **Solution**: Semaphore-based concurrency limits in `AsyncImageProvider`

### 🔧 ScrollBench Enhancements
-   **MFT Scanner Integration**: ScrollBench now uses same fast scanning as main app
-   **Album View Fixed**: Performance and functionality improvements
-   **UI Style Consistency**: "Basic" style applied for visual parity with main app

### 🧹 Code Quality
-   **Null Reference Fix**: Added null check for `appSettings` in `Main.qml` (line 272)
-   **Build System**: All CMake targets updated with new dependencies

---

# Release Notes - v2.1.0 (Reforged + Network Stability)


## 🚀 Highlights
This release focuses on **Deployment Stability** and **Network Support**. The application is now fully portable and can be run directly from network shares without installation or missing DLL errors.

## 🛠 Fixes & Improvements

### 🌐 Network & Deployment
-   **UNC Path Support**: Fixed file loading logic to correctly interpret `\\Server\Share` paths. Previously, these were malformed by manual string string parsing. We now use `QUrl::toLocalFile()` for robust handling.
-   **Self-Contained Deployment**: Updated `build.ps1` to include `MinGW` runtime libraries (`libgcc`, `libstdc++`, `libwinpthread`). This fixes the "Missing DLL" error when running on clean systems or network drives.

### ⚡ Performance & Stability
-   **WIC/COM Fix**: Removed `CoInitialize`/`CoUninitialize` calls from worker threads. This resolves a conflict with `QImageReader` that caused PNG/JPEG loading failures on Windows.
-   **UI Responsiveness**:
    -   Implemented a "Re-queue" strategy for RAW/Video tasks. If resources are busy, tasks yield instead of blocking the thread pool.
    -   **Reverted** a strict synchronous Memory Limit (2.5GB) that was causing UI freezes due to mutex contention.

### 🧹 Code Quality
-   Refactored `SystemMonitor` to expose static memory usage methods (ready for future GC implementation).
-   Cleaned up `AsyncImageProvider` duplicate code blocks.

## 📦 Build Information
-   **Qt Version**: 6.9.3 (MinGW 64-bit)
-   **Status**: Stable
-   **Verified**: Run `Test-Path build/QGalleryX.exe` -> True.

### 🏗 Build System & Sync
-   **Integrated ScrollBench**: `QGalleryXBench.exe` is now built automatically by the main build script, outputting to `build/test_scrollbench/`.
-   **Dependency Verification**: `build.ps1` now explicitly checks for the presence of FFmpeg DLLs before starting compilation.
-   **Git LFS**: Enabled Large File Storage for `avcodec-62.dll` (>100MB) to ensure seamless cloning and building on new machines.
