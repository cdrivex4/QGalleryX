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
-   **Verified**: Run `Test-Path build/appSamsungGallery.exe` -> True.

### 🏗 Build System & Sync
-   **Integrated ScrollBench**: `appScrollBench.exe` is now built automatically by the main build script, outputting to `build/test_scrollbench/`.
-   **Dependency Verification**: `build.ps1` now explicitly checks for the presence of FFmpeg DLLs before starting compilation.
-   **Git LFS**: Enabled Large File Storage for `avcodec-62.dll` (>100MB) to ensure seamless cloning and building on new machines.
