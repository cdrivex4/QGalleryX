# UI Responsiveness Optimization Plan

This document outlines a roadmap to push the UI responsiveness of the Antigravity application beyond its current limits, targeting absolute zero-jank 120fps+ scrolling through hundreds of thousands of media items.

## Part 1: 10 General Ways to Improve UI Responsiveness (Top-Level & Low-Level)

1. **Hardware Scenegraph Materials (Low-Level):** Move aspect-ratio cropping and image scaling to custom `QSGMaterial` shaders (GLSL/HLSL). This shifts math off the CPU and uses the GPU's native texture mapping.
2. **Background Texture Uploads (Low-Level):** Use a shared background `QOpenGLContext` (or Vulkan equivalent) to upload `QImage` data to VRAM asynchronously. Currently, `window()->createTextureFromImage` blocks the main GUI thread.
3. **Texture Atlasing (Low-Level):** Instead of individual `QSGTexture` objects for every thumbnail, pack hundreds of thumbnails into a single 4K Texture Atlas. This reduces GPU draw calls exponentially.
4. **Lock-Free Thread Queues (Low-Level):** Replace mutex-backed thread pools in `TaskScheduler` with Single-Producer/Multiple-Consumer (SPMC) lock-free ring buffers to eliminate contention stalls when queuing 10,000 thumbnails.
5. **Scroll Velocity Heuristics (Top-Level):** Implement a precise kinematic scroll tracker. When scroll velocity exceeds a threshold (e.g., >3000px/s), aggressively pause all IO/decoding. Only resume when the viewport begins to settle.
6. **Pre-Compiled QML & Basic Controls (Top-Level):** Strip all `QtQuick.Controls` down to `Basic` style and ensure 100% of the QML compiles to native C++ using `qmlsc` (avoiding any V4 JS engine fallback during scrolling).
7. **Eliminate Alpha Blending (Top-Level):** Remove semi-transparent overlays (like the RAW/Video badges or selection state). Instead, pre-bake these badges directly into the `QImage` on the background thread before it hits the GPU.
8. **Mmap Database for Thumbnails (Low-Level):** Instead of saving 10,000 individual `.cache` files to disk, use a Memory-Mapped (mmap) key-value store (like LMDB or a flat blob). This avoids NTFS filesystem overhead for tiny files.
9. **Granular Model Updates (Top-Level):** When filtering or sorting, use `beginMoveRows` and `beginInsertRows` exclusively instead of triggering a full model reset, preventing the QML scenegraph from tearing down and rebuilding.
10. **Pre-warmed Object Pools (Top-Level):** Instead of dynamically creating/destroying delegates inside a `Repeater` as the layout shifts, allocate a fixed, absolute pool of QML Items that simply teleport and re-bind data.

---

## Part 2: 10 Better Ways Than Our Current Implementation

| Area | Current Implementation | Better Alternative |
| :--- | :--- | :--- |
| **1. Delegate Node Count** | `FastImageItem` per thumbnail inside a `Row` -> `Repeater`. Creates ~10 QSGNodes per image (image, overlays, text). | **`GridRendererItem`**: A single monolithic C++ custom `QQuickItem` that paints the *entire* grid row (or even viewport) using one `QSGGeometryNode` and a texture array. |
| **2. Grouping Architecture** | `GroupedProxyModel` rebuilds a linear internal array when sorting/filtering, blocking the main thread. | **Asynchronous B-Tree Model**: Move grouping logic to a background thread that calculates the tree hierarchy and applies updates via granular diffs. |
| **3. Texture Instantiation** | `FastImageItem::updatePaintNode` calls `createTextureFromImage` on the main GUI thread. | **Offscreen RHI Context**: Background threads create `QRhiTexture` objects directly and hand the GPU pointers to the GUI thread instantly. |
| **4. FFmpeg Video Decoding** | `VideoThumbnailer` opens and closes `avformat_open_input` for every single video file individually. | **Persistent FFmpeg Daemon**: Maintain a pool of warm `AVFormatContext` workers that stream sequentially, saving massive file-header parsing latency. |
| **5. Viewport Culling** | Culling works by returning `QVariant()` for out-of-bounds indices, causing QML to blank data but keep objects. | **Virtual QML Viewport**: Write a custom C++ `QQuickItem` that acts as the `ListView`, rendering *only* what is visible and completely bypassing QML's internal `QQuickItemView`. |
| **6. Image Scaling** | Images are scaled via `QImageReader::setScaledSize` (CPU scaling) or LibRaw bounds. | **GPU-Only Mipmapping**: Read raw file chunks into memory and let the GPU do bilinear downsampling. |
| **7. Date Scrubber** | Iterates over the QML/Proxy model sequentially to find the first index matching a date. | **Binary Search Index**: Maintain a secondary `std::map<QDateTime, int>` in C++ to jump to exact scroll positions in `O(log n)` time. |
| **8. Memory Management** | Relies on QML GC bypass, but still allocates C++ `AsyncImageResponse` wrappers and lambda captures dynamically. | **Zero-Allocation Pipeline**: Pre-allocate an arena of 10,000 `AsyncImageResponse` objects at startup and reuse them to eliminate all heap allocation during scrolling. |
| **9. I/O Reads** | Uses `QFile` to read image files synchronously in the worker threads. | **Windows IO Completion Ports (IOCP) / `io_uring`**: Use OS-level asynchronous overlapped I/O to queue hundreds of file reads simultaneously without blocking thread execution. |
| **10. UI Telemetry** | Timer-based QML updates poll `Atomic` counters, causing micro-stalls on the main thread loop. | **Shared Memory Overlay**: Use a completely separate hardware overlay (or a separate process window) to render debug stats, leaving the main thread 100% dedicated to images. |

---

## Part 3: SWOT Analysis of the Current UI Rendering Architecture

### Strengths
- **Custom `FastImageItem`:** Bypassing the native `QQuickPixmapCache` successfully sidesteps the chaotic Garbage Collection spikes that were corrupting layouts.
- **Aggressive Multithreading:** The IO and CPU-bound thread separation prevents heavy FFmpeg/LibRaw tasks from stalling standard JPEG reads.
- **Extremely decoupled:** The pipeline (Scanner -> Model -> Proxy -> AsyncProvider -> QML) is logically isolated, making it highly testable.

### Weaknesses
- **Main Thread Texture Uploads:** The system still suffers from a fundamental Qt bottleneck: `createTextureFromImage` runs on the main thread, meaning a burst of 50 cached images loading simultaneously can still cause a minor frame drop.
- **Node Bloat:** The QML delegate is "heavy". Every thumbnail has multiple overlay items (Shadows, Video Badges, RAW badges, borders), multiplying the Scenegraph node count exponentially.
- **Disk Cache I/O:** Saving thousands of 10KB `.cache` files crushes the NTFS filesystem and creates artificial IO bottlenecks.

### Opportunities
- **Vulkan / RHI Migration:** Transitioning fully to Qt 6's RHI (Rendering Hardware Interface) allows us to bypass legacy OpenGL limitations and use modern features like Texture Arrays and Compute Shaders for decoding.
- **Unified Single-Node Rendering:** Scrapping the `ListView` entirely in favor of a single C++ `ScrollBenchGrid` item could push performance from "great" to "industry leading".
- **Machine Learning Caching:** Predict user scroll direction to pre-fetch and pre-decode images before they even enter the viewport buffer.

### Threats
- **VRAM Exhaustion:** Even at 128x128, loading 50,000 thumbnails into GPU memory will eventually crash the VRAM buffer. Strict GPU eviction policies are critical.
- **Windows File Lock Contention:** High-velocity scrolling triggers thousands of rapid file opens/closes. If antivirus software (e.g., Windows Defender) intercepts these, the IO thread pool will stall.
- **QML Engine Unpredictability:** No matter how optimized our C++ is, relying on QML for layout math (`GroupedProxyModel` + `ListView` + `Repeater`) always carries the risk of hidden JS evaluation stalls in edge cases.
