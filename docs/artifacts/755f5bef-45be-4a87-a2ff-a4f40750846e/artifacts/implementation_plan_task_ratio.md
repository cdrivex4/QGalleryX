# Implementation Plan: Smart Round-Robin Worker Loop (5:1 Image-to-Video Ratio)

## 1. Overview
The goal is to strictly interleave image and video processing tasks at a 5:1 ratio within the core worker threads. This ensures video thumbnails (which are heavy and slow) do not saturate the thread pool and stall the loading of fast image thumbnails. 

Crucially, this will **not** increase the overall CPU limit. The existing safeguard—which limits the total number of CPU worker threads to 80% of your available hardware cores—will remain completely intact. The 5:1 ratio simply dictates *what* those existing threads choose to work on next.

## 2. Files to Modify

### A. `src/TaskScheduler.h`
**What we will change:**
1. **New Enum:** Add a `TaskCategory` enum (`ImageTask`, `VideoTask`) to categorize workloads.
2. **Update `addTask` Signature:** Modify `addTask` to accept the new `TaskCategory` parameter.
3. **Split Queues:** Currently, `m_cpuQueue` and `m_ioQueue` are singular maps keyed by `Priority`. We will split them into arrays of maps (e.g., `QMap<Priority, QList<Task>> m_cpuQueues[2]`), allowing us to cleanly separate Image tasks from Video tasks.
4. **Shared Counters:** Add atomic counters (e.g., `std::atomic<int> m_cpuRatioCounter{0}`) to track the exact mathematical turn for the round-robin logic.

### B. `src/TaskScheduler.cpp`
**What we will change:**
1. **Queue Insertion (`addTask`):** Route incoming tasks into either the Image or Video queue bucket based on the requested `TaskCategory`.
2. **Smart Dequeue Logic (`cpuWorkerLoop` & `ioWorkerLoop`):**
   - When a thread wakes up to find work, it will increment the shared ratio counter.
   - `turn = counter % 6`. 
   - If `turn < 5` (Turns 0, 1, 2, 3, 4), the thread's primary target is the **Image Queue**.
   - If `turn == 5` (Turn 5), the thread's primary target is the **Video Queue**.
   - **Fallback Logic (100% CPU Utilization):** If a thread's primary target queue is empty, it will instantly fallback to the *other* queue. This guarantees threads never sit idle if there's work to be done, satisfying the requirement to process 100% videos if no images are left.
3. **Wait Conditions:** Update the condition variables (`m_cpuCondition.wait()`) so threads only sleep if *both* the Image and Video queues are completely empty.

### C. `src/AsyncImageProvider.cpp`
**What we will change:**
1. **Task Dispatching:** Locate all instances where `TaskScheduler::instance().addTask(...)` is called to dispatch image generation or I/O reads.
2. **Category Injection:** Check if the incoming request is a video (`req.isVideo`). Pass `TaskScheduler::VideoTask` if it is a video, and `TaskScheduler::ImageTask` otherwise.

## 3. How It All Wires Up
1. When the QML Grid requests 50 images and 50 videos, `AsyncImageProvider` stages them.
2. `AsyncImageProvider` packages them into C++ `std::function` tasks and pushes them to `TaskScheduler`, explicitly tagging their category.
3. Inside `TaskScheduler`, the 50 images sit in the `ImageQueue` and 50 videos sit in the `VideoQueue`.
4. The worker threads (capped at 80% of your CPU) wake up.
5. Thread 1 gets turn 0 -> Pops Image.
6. Thread 2 gets turn 1 -> Pops Image.
7. ... Thread 6 gets turn 5 -> Pops Video.
8. As threads finish, they grab the next turn. This mathematically guarantees exactly 5 images pop for every 1 video, keeping the UI feeling blazing fast.

## 4. Impact Analysis
* **CPU Usage:** Remains capped at 80%. We are changing *task selection*, not *thread count*.
* **UI Responsiveness:** Will drastically improve for mixed-media folders.
* **Idle Threads:** Prevented. If you enter a folder of 100 videos and 0 images, threads will prefer the empty Image queue, instantly fallback to the Video queue, and chew through the videos at maximum speed.
