# QGalleryX - Threading & Caching Architecture Handoff

This document is a hand-off file intended for an AI agent performing a detailed SWOT analysis and architectural review of the `QGalleryX` project. The core problem this document addresses is the conflict between **UI responsiveness (unblocking the UI)** and **thread safety / state machine integrity** during heavy directory scans.

## 1. Project Structure & Core Components

- **`ImageModel` (C++):** The core `QAbstractListModel` that feeds the QML `GridView`. It maintains two primary vectors: `m_allItems` (the raw list of discovered files) and `m_items` (the filtered/sorted list presented to the UI). 
- **`TaskScheduler` (C++):** A custom global thread pool that handles background CPU/IO tasks (primarily generating thumbnails via FFmpeg or `libraw`). Tasks are assigned priorities (`High`, `Normal`, `Low`, `Background`).
- **`AsyncImageProvider` (C++):** Serves `image://async/` requests from the QML delegates. It submits decoding tasks to the `TaskScheduler`.
- **`GalleryView` / `Main.qml` (QML):** The frontend UI. The `GridView` relies heavily on `cacheBuffer` to preload off-screen images. 
- **`DesktopHelper` (C++):** Provides utility functions, including checking if a drive is a high-latency network path or removable media (`staticIsNetworkPath`).

## 2. The Loading Lifecycle (The Two-Pass System)

When a user opens a directory (e.g., an SD card with 32,000 files), `ImageModel` spawns a background `QtConcurrent` worker to perform a Two-Pass scan:

### Pass 1: The Fast Walk (`QDirIterator`)
- Iterates sequentially through the physical directory table.
- Parses the filename via Regex to guess the creation date (e.g., `20260805_120000.jpg`).
- Appends the file to a temporary `fastItems` list.
- **Current Fix applied:** Sends an "Early View" (partial list) to the UI at 200ms, and pushes the final full 32,000 skeleton list to the UI the moment Pass 1 finishes.

### Pass 2: The Metadata Fill (`QFileInfo`)
- Loops over `fastItems`.
- Calls `QFileInfo(path).size()` and `birthTime()` to fill in missing metadata.
- **Current Fix applied:** Pass 2 now uses an **Interrupt / Spin Lock**. It continuously polls `TaskScheduler::hasUrgentTasks()`. If the user scrolls (generating Urgent UI tasks), Pass 2 instantly pauses itself (spin locks), yielding 100% of the Disk I/O pipeline so thumbnails load instantly. Once the UI stops demanding thumbnails, Pass 2 resumes crawling.

## 3. The Precache State Machine (Snail/Rocket Icons)

The UI has a button to toggle `precacheMode`:
- `0` (Battery Saver): Strict mode, no background generation.
- `1` (Idle/Yellow Snail): Slow background generation (Low priority).
- `2` (Aggressive/Red Rocket): Fast background generation (Normal priority).

When enabled, a `QTimer` (`m_precacheTimer`) fires every 50ms, walks through `m_allItems` using an index counter (`m_precacheIndex`), and submits thumbnail generation tasks to the `TaskScheduler`. 
- **Current Fix applied:** We introduced `m_precacheGeneration` to instantly kill zombie tasks in the `TaskScheduler` when the user switches back to Battery Saver.

## 4. The Core Architectural Conflict

The application faces a paradox between performance and thread safety.

**The Desire:** We want to enable the global metadata `.bin` cache for ALL drives, meaning the brutal `QFileInfo` Pass 2 only ever runs *once* per folder. **(Note: We recently achieved this! We removed the `isNetworkPath` restriction and bound the cache hash to the hardware Volume Serial ID so swapping SD cards doesn't cross-contaminate data).** However, during that very first run, we want to push the 32,000 skeletons to the UI instantly (unblocking the UI), while Pass 2 quietly fetches sizes/dates in the background.

**The Danger:** If we unblock the UI and let Pass 2 run in the background, we mangle the state machine. 
1. Pass 2 eventually finishes, updates the dates, and calls `applyFilter()`. This re-sorts the entire `m_allItems` array.
2. If the user had toggled the Precache Snail on, `m_precacheIndex` was happily walking through the array. When the array is suddenly re-sorted out from under it, the precache logic breaks (skipping files, processing files twice).
3. Simultaneously, QML delegates are requesting thumbnails, but the underlying data model is shifting, causing UI jitter and race conditions.

## 5. Active Testing Phase (The "Thread Governors")

We are currently testing four different "Thread Governors" dynamically exposed to the QML UI to handle how `TaskScheduler` queues background work:
- **Sequential (FIFO)**
- **Viewport First (LIFO)**
- **Adaptive (Hybrid LIFO/FIFO)**
- **Round Robin (1:1)**

While the new **Spin Lock** in Pass 2 prevents the disk from being starved during thumbnail generation, the core state machine danger (Point #4) still remains unsolved.

## 6. Known Edge Cases / Bugs to Fix

**Drag-and-Drop Race Condition:** 
When dragging a file into the window, `Main.qml` sets a `pendingFileToOpen` variable and switches the `currentPath`. The goal is to automatically open the image full-screen once it's found. 
However, because of the "Early View" batching in Pass 1, `itemsPopulated()` is emitted prematurely (at 200ms). The UI checks if the dropped image is in this partial 200ms batch. If it's not (e.g. it's the 5,000th file), the UI logic sometimes unconditionally clears the `pendingFileToOpen` variable. When the scan fully finishes seconds later, the UI has forgotten what to open. 
*Current Workaround Attempt:* We added a check `if (!window.activeModel.isLoading)` before clearing it, but it appears `itemsPopulated` from a *previous* directory scan (or a thread race) might still be falsely triggering the clear. A more robust ID or path-matching verification is needed before abandoning the pending request.

## 7. Refactoring Blueprint (The Mission)

You must perform a detailed **SWOT Analysis** and implement a strict refactor of the threading architecture based on the following exact blueprint:

### 1. Queue-Based Event Loop for Pass 2 (No more destructive `applyFilter()`)
Pass 2 should **never** touch `m_allItems` directly, and it should **never** trigger `beginResetModel()`. Instead:
- As the `QFileInfo` worker loops through the `fastItems`, it pushes its metadata findings (file path, precise size, birth time) into a thread-safe producer-consumer queue (e.g. `QConcurrent::Queue` or a mutex-protected `std::queue`).
- The main UI thread utilizes a low-interval `QTimer` to consume this queue in batches, updating the specific items in the model and emitting `dataChanged(index, index, {Roles})`.
- The final sort must use `layoutAboutToBeChanged()` and `layoutChanged()` rather than `beginResetModel()`. This shuffles existing delegate instances rather than destroying them, fixing the UI jitter without deferring the sort.

### 2. O(1) Decoupled Precache Indexing
- Stop relying on `m_precacheIndex`. 
- When Pass 1 finishes, populate a `std::deque<quint64>` representing the precache backlog. Use a **quint64 hash of the absolute path** as the standard identifier across the entire application (precache queue, completion queue, drag-drop handler).
- Maintain a `QHash<quint64, int> m_idToRow` in parallel with the sorted items, keeping it in sync during every sort.
- The 50ms `m_precacheTimer` pops a quint64 ID off the deque, looks up its model index in O(1) time via `m_idToRow`, requests the thumbnail, and moves on. This is immune to array sorting. 
- Pair this with `m_precacheGeneration` to instantly kill zombie tasks when dropping to Battery Saver mode.

### 3. Drag-and-Drop Race Condition Fix
- Abandon `itemsPopulated()` for clearing pending files. 
- The race isn't just about finding the file; it's about stale signals from previous directory scans triggering clears.
- Introduce a `std::atomic<quint32> m_scanId` that increments on every new scan. Gate every signal handler (`passOneCompleted(quint32 scanId)`) to ensure the signal matches the current scan.
- Store the absolute path of the dropped file. Do not clear `pendingFileToOpen` unless the exact path is confirmed loaded, or the final directory walk explicitly reports the file does not exist.

### 4. Condition Variable for the Spin Lock
- The current spin lock uses a manual `QThread::msleep()` loop. This burns CPU on battery and gets dramatically worse on network paths. 
- Replace the `QThread::msleep()` spin lock with a proper `std::condition_variable` in `TaskScheduler` that Pass 2 can block on cleanly.

## 8. Zero-Crash Architecture & Mmap Hardening (Completed v2.3.3)

1. **Deterministic Lifecycle RAII (`ImageModel::~ImageModel()`)**:
   - Background tasks and UI dispatches are guarded by `std::shared_ptr<std::atomic<bool>> m_aliveToken` and `QPointer<ImageModel> safeThis`.
   - On model destruction, `m_aliveToken` is set to `false`, `m_scanGeneration++` and `m_precacheGeneration++` are incremented, timers are stopped, and `m_folderWatcher` is unhooked.
2. **Context-Bound Singleton Signals**:
   - `connect(&FileCacheManager::instance(), &FileCacheManager::cacheCleared, this, ...)` passes `this` as the context receiver, ensuring Qt auto-unregisters the connection on model destruction.
3. **Multi-Window Lifecycle (`DesktopHelper::openNewWindow()`)**:
   - Stack-allocates `QQmlComponent`, assigns `QQmlEngine::CppOwnership`, and binds `visibleChanged` to `win->deleteLater()` for leak-free secondary windows.
4. **16MB Mmap Incremental Allocation & 30% Compaction**:
   - `MMAP_GROW_CHUNK` resized from 512MB to 16MB.
   - `MmapCacheDatabase::compact()` automatically defragments the binary cache when dead records exceed 30%.
5. **NTFS MFT Recursion Safety**:
   - Guarded `FastVolumeScanner::resolvePath()` with `depth > 64` ceiling and string pool boundary validation.
6. **Forensic Black Box Recorder**:
   - Installed `SetUnhandledExceptionFilter` and `std::set_terminate` in `main.cpp` generating minidumps (`crash_dump.dmp`) and diagnostic logs (`application_crash.log`).

## 9. Zero-Latency GUI Thread & Directional Lookahead (Completed v2.3.4)

1. **Asynchronous L2 MMAP Decompression**:
   - Removed synchronous `diskImg.loadFromData(mmapData)` from `AsyncImageProvider::requestImageResponse()`.
   - All L2 disk cache hits now decompress in parallel on `TaskScheduler` background worker threads. The GUI thread has $0\text{ms}$ decompression latency.
2. **Trajectory-Biased Directional Lookahead**:
   - `ViewportGovernor::updateViewport()` shifts lookahead windows ($+2\times \text{count}$ forward during downward scrolls, $-2\times \text{count}$ backward during upward scrolls).
   - Unified `DateScrubber.qml` to calculate continuous delta movements and feed into the exact same lookahead prefetching pipeline as touch flings.
3. **QML Delegate Pooling & Precomputed Roles**:
   - Enabled `reuseItems: true` on `GridView` in `GalleryView.qml` and `GalleryViewTiles.qml`.
   - Added precomputed `IsVideoRole` in `ImageModel`, eliminating thousands of JavaScript string splitting operations per second.
4. **Dynamic Low-Core Worker Scaling**:
   - Tuned `TaskScheduler` on $\le 4$ thread CPUs to dedicate 2 threads to the UI event loop, Scene Graph, and OS compositor.
