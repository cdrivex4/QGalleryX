# Action Plan: UI Responsiveness & 10k Architecture

This document combines the UI Responsiveness Plan and the 10k Architecture Strategy into a single actionable todo list. It details exactly how we will restore the legacy application, wire the new Multi-Tiered Hierarchical Eviction Pipeline in `test_scrollbench`, and decouple the codebases without breaking the automated build pipeline.

---

## 1. Legacy Application Restoration & Decoupling Strategy

The legacy application (`appSamsungGallery.exe`) was functionally frozen at `v2.1.0` (Commit `33419c0`), but recent development on `test_scrollbench` inadvertently overwrote shared files in the common `src/` directory, breaking the legacy pipeline. 

We must decouple the code so that experimental changes in ScrollBench do not affect the legacy reference app, while ensuring `build.ps1` and `tst_linkage.exe` continue to pass.

### Safe Decoupling & Build Pipeline Protection
1. **Physical Separation (No Deletions):** We will not delete the current `src/` folder. Instead, we will:
   - Extract the `v2.1.0` legacy files into a new `src_legacy/` directory.
   - Retain the current experimental files inside `test_scrollbench/src/` (or a dedicated `src_next/`).
2. **CMake Target Isolation:** 
   - Modify the root `CMakeLists.txt` so that the `appSamsungGallery` target explicitly links against `src_legacy/`.
   - Modify `test_scrollbench/CMakeLists.txt` so that `appScrollBench` links exclusively against the experimental backend.
3. **Linkage Verification:** The `tst_linkage.exe` post-build step guarantees backend validity. By cleanly separating the CMake targets before modifying any C++ code, we guarantee that the build pipeline (`build.ps1`) will not break during the migration.
4. **Knowledge Extraction:** With the `v2.1.0` pipeline isolated and running, we can study its original queueing and garbage collection to safely port its mathematical limits to the new ScrollBench eviction pipeline.

---

## 2. The 10k Image Target: A Reality Check

*Correction:* The target is **10,000 images**, not 100,000. 
This is a massive game-changer. At 10k images, QML's `GridView` with highly aggressive viewport culling is **100% viable**. We do not need to reinvent the wheel with monolithic C++ OpenGL renderers or complex Spatial QuadTrees. The current `FastImageItem` and `ScrollBenchImageModel` approach is correct, it just lacks a proper memory eviction pipeline.

### 10k Architecture SWOT Analysis

| Strengths | Weaknesses |
| :--- | :--- |
| **QML Native:** 10k is within the upper bounds of what QML can handle *if* instantiated objects are strictly managed. <br> **UI Consistency:** We keep our robust `Ctrl+Click` selection, animations, and custom styling. | **Garbage Collection Spikes:** If the user scrolls rapidly from item 0 to 10,000, QML will thrash allocating and destroying 10,000 `FastImageItem` delegates. |

| Opportunities | Threats |
| :--- | :--- |
| **Multi-Tiered Eviction:** Implementing strict RAM/VRAM eviction tiers will guarantee a flat memory curve, regardless of how fast the user scrolls. | **Disk I/O Bottlenecks:** 10k thumbnails pulled from an HDD will stall the pipeline if the Disk Cache tier is not pre-warmed or prioritized correctly. |

---

## 3. The Multi-Tiered Hierarchical Eviction Pipeline

To break the performance plateau in `test_scrollbench`, we must implement a deterministic, multi-tiered eviction pipeline. The legacy application touched on this, but we will explicitly wire the modules as follows:

### The Architecture (Module Wiring & Data Flow)

The pipeline consists of 4 distinct tiers. Data flows *up* towards the GPU when requested, and is actively *evicted down* when the user scrolls away.

#### Tier 0: The QML Viewport (VRAM & Scenegraph)
*   **Module:** `GalleryViewScrollBench.qml` & `FastImageItem.qml`
*   **Operation:** QML handles exactly what is visible on the screen plus a small pre-fetch buffer.
*   **Eviction Trigger:** As a delegate leaves the viewport + buffer zone, it is destroyed. `FastImageItem` explicitly nullifies its internal `QSGTexture`, forcing an immediate VRAM release rather than waiting for QML's lazy garbage collector.

#### Tier 1: Pending & Active Decode Queue (CPU / Thread Pool)
*   **Module:** `AsyncImageProvider` & `TaskScheduler` (`s_stagedRequests` and `m_pendingResponses`)
*   **Operation:** Holds requests that are currently being decoded from disk.
*   **Eviction Trigger:** When QML destroys a delegate, the `AsyncImageResponse` destructor fires. The `ResponseTracker` drops its atomic reference.
*   **Module Wiring:** `TaskScheduler` explicitly checks `isRequestStillNeeded()` before admitting a task to a worker thread. If the reference is gone, the task is **evicted (aborted)** immediately, saving CPU cycles.

#### Tier 2: The Main RAM Cache (Uncompressed `QImage` Data)
*   **Module:** `AsyncImageProvider::m_cache` (`QCache<QString, QImage>`)
*   **Operation:** Holds fully decoded matrices ready for instant QML texture generation.
*   **Eviction Trigger:** The cache has a strict `setMaxCost()` limit (e.g., 200MB). 
*   **Hierarchical Logic:** When new images from Tier 1 complete, they are inserted here. If the cost exceeds 200MB, the Least Recently Used (LRU) images are evicted from RAM. *Crucially, if the user scrolls backwards, we want to hit Tier 3, not Tier 1.*

#### Tier 3: The Persistent Disk Cache (Compressed Thumbnail Data)
*   **Module:** `FileCacheManager` (Proposed Module based on legacy architecture)
*   **Operation:** To prevent re-running heavy `LibRaw` or `FFmpeg` decodes for 10k items, the compressed thumbnails (e.g., small JPEGs) are saved to `%LOCALAPPDATA%`.
*   **Eviction Trigger:** Managed by a low-priority background thread (`TaskScheduler::queueIoTask`). When the disk cache folder exceeds a set gigabyte limit, the oldest files are physically deleted.

### Step-by-Step Function Execution Flow

1.  **Scroll Event:** User scrolls down. `ScrollBenchImageModel` calculates `visibleStartIndex = 500`, `visibleEndIndex = 600`.
2.  **Cull Signal:** Items 400-499 exit the buffer zone. QML destroys their delegates.
3.  **Tier 0 Eviction:** `~AsyncImageResponse()` fires. VRAM is freed.
4.  **Tier 1 Eviction Check:** `TaskScheduler` pops item 450 from the priority queue. `tracker->response.load()` returns `nullptr`. The task is skipped.
5.  **Tier 2 Insertion:** Item 550 finishes decoding. `AsyncImageProvider::insertCachedImage()` is called. The RAM cache exceeds 200MB, so item 200 is evicted from RAM.
6.  **Fast Return:** User scrolls *up* to item 540. It is pulled instantly from Tier 2 (RAM).
7.  **Deep Return:** User scrolls back to item 200. It was evicted from Tier 2, but `AsyncImageProvider` intercepts the request and pulls the pre-rendered thumbnail directly from Tier 3 (Disk Cache) in ~2ms instead of running LibRaw (120+ seconds).
