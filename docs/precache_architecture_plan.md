# Pre-caching Architecture Plan & Impact Analysis (v2)

This document outlines the architecture, UI mapping, and fail-safes for the new Background Precaching engine and the user-facing "Snail Mode" control.

## 1. UI Integration: The Snail Button
We will introduce a 3-state toggle button (Snail Icon) in the QML Action Bar next to the "Scan Folder" button. It controls the `precacheMode` exposed by the C++ `ImageModel`.

1. 🐌 **White Snail (Strict Mode / Mode 0):** 
   - **Behavior:** The precacher timer is disabled.
   - **Result:** Ultimate battery saver. Zero off-screen processing. The system strictly waits for viewport visibility before decoding.
2. 🐌 **Yellow Snail (Idle Mode / Mode 1):**
   - **Behavior:** Dispatches tasks at `Low` priority.
   - **Result:** Yields to UI. Only processes background thumbnails when the user is completely idle and not scrolling.
3. 🐌 **Red Snail (Aggressive Mode / Mode 2):**
   - **Behavior:** Dispatches tasks at `Normal` priority.
   - **Result:** Saturates CPU cores. Ignores UI throttling and aggressively burns through the folder as fast as the hardware allows.

## 2. Files & Dependencies Touched

### `TaskScheduler.h` & `TaskScheduler.cpp`
**Changes:**
- Modify `addTask()` to accept a new optional parameter: `QString taskKey` (e.g., the file path).
- Add `hasImmediateTasks()` and `getQueueSize(Priority p)` so external classes can monitor saturation.
- Update internal enqueueing inside `m_cpuMutex`. Before pushing a task, scan existing queues for `taskKey` and promote it to the higher priority if it already exists in a lower queue.
- Inject OS-level thread priority commands (`SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL)`) on Windows to drop the background worker threads.

### `AsyncImageProvider.h` & `.cpp`
**Changes:**
- Add a static `precache(const QString &path, const QSize &requestedSize)` method.
- This method checks `FileCacheManager`. If a thumbnail exists, it aborts. If not, it decodes the image, resizes it, and writes the result strictly to the disk cache.
- Update existing `addTask` calls to pass the image `path` as the `taskKey`.

### `ImageModel.h` & `.cpp`
**Changes:**
- Introduce a `QTimer` based `IdlePrecacheWorker`.
- Expose `precacheMode` (int 0, 1, 2) to QML via `Q_PROPERTY`.
- Introduce `std::atomic<uint64_t> m_precacheGeneration`.

## 3. The Core Safety Nets (To Prevent Breakages)

| Risk | Mitigation Mechanism |
| :--- | :--- |
| **Priority Inversion / Duplicate Work** | **TaskKey Promotion:** If the UI suddenly demands a file currently sitting in the `Low` background queue, the `TaskScheduler` will rip the task out of the `Low` queue and instantly promote it to the `Immediate` queue. |
| **Hang on Mode Switch** | **Generation Tokens:** Clicking the Snail button increments `m_precacheGeneration`. When a queued background task executes, it compares its internal token to the master token. If they mismatch (user changed modes), the task instantly aborts in nanoseconds, safely flushing the queue without processing. |
| **RAM Bloat (Queue Saturation)** | **Submission Throttle:** When traversing a 50,000-image folder, the worker will check `TaskScheduler::getQueueSize()`. It will only maintain a maximum of 50 tasks in flight at any given time. This keeps RAM usage completely flat, no matter how large the folder is. |
| **UI Stuttering** | **Hardware Isolation:** The UI runs on the main thread, while the `TaskScheduler` guarantees 1 physical CPU core remains totally idle. `Idle Mode` explicitly pauses itself if `hasImmediateTasks()` is true. |

## 4. Expected Handoff Flow

1. You open a massive folder and click the **Yellow Snail**.
2. The UI renders the visible items (Immediate Priority).
3. The precacher looks at the queue, sees it is empty, and pushes the next 50 off-screen images into the `Low` priority queue.
4. You suddenly jump to index 500. QML demands index 500 at `Immediate` priority.
5. The precacher detects `Immediate` activity and goes to sleep.
6. The `TaskScheduler` intercepts index 500. If it was in the `Low` queue, it gets promoted.
7. The UI renders smoothly. When you stop scrolling, the precacher wakes back up.
