# Pre-caching Architecture Plan & Impact Analysis

Before we write a single line of code, here is exactly what this change entails, what systems it will touch, what the risks are, and what the expected outcomes will be.

## 1. Files & Dependencies Touched

### `TaskScheduler.h` & `TaskScheduler.cpp`
**What we are changing:**
- We will modify `addTask()` to accept a new optional parameter: `QString taskKey` (e.g., the file path).
- We will add a method `hasImmediateTasks()` so external classes can check if the UI is currently demanding resources.
- We will modify the internal enqueueing logic inside the `m_cpuMutex` lock. Before pushing a task, it will scan existing queues for the `taskKey`.
- We will inject OS-level thread priority commands (`SetThreadPriority` on Windows) to drop the background worker threads to `THREAD_PRIORITY_BELOW_NORMAL`.

**Dependencies/Risks:**
- This is the core multi-threading engine for the entire app. Any bugs introduced here (deadlocks or race conditions) will cause the app to freeze.
- We must ensure that scanning the queues for `taskKey` is O(N) but extremely fast, as holding `m_cpuMutex` blocks all thread dispatches. 

### `AsyncImageProvider.h` & `.cpp`
**What we are changing:**
- We will add a static `precache(const QString &path, const QSize &requestedSize)` method.
- This method will check `FileCacheManager`. If the thumbnail exists, it safely aborts. If not, it decodes the image (using the existing LibRaw/QImageReader logic) and writes the result strictly to the disk cache.
- We will update the existing `addTask` calls in this file to pass the image `id` (file path) as the `taskKey`.

**Dependencies/Risks:**
- We must ensure the `precache` method doesn't inadvertently trigger UI signals or allocate massive `QImage` blocks into the RAM cache needlessly. It should only write to disk and clear RAM immediately to avoid memory leaks.

### `ImageModel.h` & `.cpp`
**What we are changing:**
- We will introduce an `IdlePrecacheWorker` (likely driven by a `QTimer` running on the main thread).
- After `scanDirectory` finishes loading all files into `m_allItems`, this timer activates.
- Every ~50ms, it checks `TaskScheduler::hasImmediateTasks()`.
  - If `true` (user is scrolling), it skips a beat and waits.
  - If `false` (user is idle), it pops the next 3-5 images from `m_allItems` that are currently off-screen and dispatches them to `TaskScheduler::Low` priority.

**Dependencies/Risks:**
- `ImageModel` is heavily tied to QML. The timer must be extremely lightweight so it doesn't cause main-thread micro-stutters. By only checking a boolean and pushing a lambda, overhead is virtually zero.

---

## 2. The Handoff Flow (Expected Behavior)

1. **Idle state:** You open a folder of 1,000 RAW images. The first 20 show up immediately. The precacher begins walking from index 21 to 1,000, adding them to the `Low` priority queue.
2. **The User Scrolls:** You suddenly jump to index 500. 
3. **The Interception:** QML demands index 500 at `Immediate` priority.
4. **The Promotion:** `TaskScheduler` receives the request for index 500. It sees that index 500 is currently sitting in the `Low` queue. It deletes it from the `Low` queue and inserts it into the `Immediate` queue.
5. **The Throttle:** The precacher realizes `Immediate` tasks are now pending. It goes to sleep.
6. **The Result:** The UI renders index 500 instantly. When you stop scrolling, the precacher wakes back up and continues filling the disk cache.

## 3. Potential Breakages & Mitigations

| Risk | Mitigation Strategy |
| :--- | :--- |
| **Deadlocking the Scheduler** | We will strictly limit `taskKey` deduplication checks to happen inside the existing mutex locks, preventing race conditions where a thread grabs a task right as we promote it. |
| **Memory Bloat** | Background precaching will only write to the **Disk Cache** (`FileCacheManager`), NOT the RAM cache. This ensures the app doesn't consume gigabytes of RAM while idling in the background. |
| **UI Stuttering** | We will leave 1 CPU core totally free and lower the OS thread priority. The OS scheduler will mathematically guarantee the UI thread takes precedence. |
