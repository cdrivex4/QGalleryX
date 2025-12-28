# AI Development Context & Project Scope

This document provides a compressed context for any AI assistant joining the project. It outlines the current state, recent breakthroughs, and the remaining roadmap.

## 🚀 Current Project Status (Dec 21, 2025)

The project is currently in the **Integration Phase**. We have successfully built, debugged, and verified a performance-optimized prototype called `ScrollBench` (located in `test_scrollbench/`). 

The primary objective is to migrate the high-performance patterns from `ScrollBench` back into the main `SamsungGallery` application.

## ✅ Completed Milestones (ScrollBench)

*   **Crash Resolution**: Identified and fixed a deep runtime crash in `AsyncImageProvider` caused by race conditions and improper Qt object lifecycle management during model resets.
*   **Frame Budgeting**: Implemented `FrameBudgetScheduler`. This throttles heavy operations (like marking 1,000 thumbnails as "loaded" simultaneously) to fit within the 16ms frame window, preventing UI stutters.
*   **Viewport Culling**: Rewrote the QML `updateCulling` logic to be robust against window resizing and rapid scrolling. It now correctly manages `visibleStartIndex` and `visibleEndIndex` with a configurable buffer.
*   **Telemetry HUD**: Developed a real-time Performance Overlay that displays FPS, Cache Hit Rate, Memory Usage, and Frame Budget completions.
*   **Stable Build**: `ScrollBench` successfully compiles and deploys to `test_scrollbench/deploy/`.

## 🛠️ Remaining High-Priority Work

### 1. Main App Integration (Critical)
*   **Backport Providers**: Replace the old `AsyncImageProvider.cpp` in `src/` with the stabilized version from `test_scrollbench`.
*   **Inject Scheduler**: Instantiate `FrameBudgetScheduler` in `main.cpp` and inject it into the main `GalleryModel`.
*   **Gallery Logic**: Update `GalleryViewSemantic.qml` to use the `updateCulling` pattern to reduce delegate instantiation overhead.

### 2. Feature Completion
*   **Timeline Scrubber**: Implement the interactive year/month scrubber for the gallery.
*   **Resize Editor**: Finalize `ResizeEditor.qml` for the Share Dialog (image compression, scaling previews).
*   **Media Viewer**: Polish the video playback transitions and ensure background audio is muted during thumbnail generation.

### 3. Technical Debt & Stabilization
*   **Lint Cleanup**: There are currently ~100+ C++ lint errors in `src/` related to missing includes and forward declarations. While the code compiles, the LSP is noisy and needs fixing.
*   **Cross-Link Fixes**: Ensure `main_scrollbench.cpp` signals and slots are fully public and correctly connected (recent fixes in progress).

## 🏗️ Architecture Stack
*   **Framework**: Qt 6.9.3 (MinGW 64-bit).
*   **Graphics**: Vulkan/D3D11 backend (configured via `SettingsHelper`).
*   **Thread Model**:
    *   `MAIN`: QML UI + Model Management.
    *   `WORKER`: `TaskScheduler` (IO-bound and CPU-bound pools).
    *   `ASYNC`: `AsyncImageProvider` (Dedicated thread for image decoding).

## 📝 Ongoing Notes for AI
*   **Aesthetics**: The user demands "Premium" looks. Use glassmorphism, smooth animations, and curated color palettes.
*   **Reliability**: Never let the UI thread block. Always use `TaskScheduler` for disk/network I/O.
*   **Binary Management**: Large binaries are handled via Git LFS or direct placement in `3rdparty/`.

---
*Last Updated: 2025-12-21 by Antigravity*
