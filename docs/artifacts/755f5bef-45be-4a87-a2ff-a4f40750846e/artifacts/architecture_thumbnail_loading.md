# Thumbnail Loading Architecture: Before vs. After

This document breaks down how the image and video loading pipelines functioned prior to our recent optimizations, how they function now, and identifies exactly where the new logic might be introducing starvation or delays.

---

## 1. Task Queuing & Thread Distribution

### The "Before" State: Blind LIFO
* **Mechanism:** All requested tasks (both Image and Video) were placed into the exact same `m_cpuQueue` based solely on their visibility priority (Immediate, Normal, etc). 
* **Insertion:** Tasks were inserted using `prepend()` (Last-In, First-Out). This ensured that the most recently requested item (what the user just scrolled to) was grabbed by the next available thread.
* **The Flaw:** Threads were completely agnostic to the *type* of work. If you scrolled to a viewport containing 12 videos, all 6 of your CPU worker threads would instantly grab a video. Because videos take 10x longer to process, all threads were gridlocked, completely **starving** any surrounding regular images from loading until the videos finished.

### The "After" State: Smart 5:1 Interleaving
* **Mechanism:** We split the internal queues into two physical arrays: `ImageTasks` and `VideoTasks`. We introduced a shared atomic counter (`turn = m_cpuRatioCounter % 6`). 
* **Execution:** When a thread wakes up, it pulls a ticket:
    * **Turns 0, 1, 2, 3, 4:** The thread attempts to pull an **Image**. (If empty, it instantly falls back to a Video).
    * **Turn 5:** The thread attempts to pull a **Video**. (If empty, it instantly falls back to an Image).
* **Potential Starvation / Delay Signals:**
    1. **Video Starvation during Rapid Scrolling:** If you are scrolling through a dense folder, you generate a constant, massive stream of new Image tasks. Because Images now get 5/6 of the CPU priority slots, Videos are strictly throttled to 1/6th of your CPU throughput. If you scroll fast, videos will deliberately be starved and will take noticeably longer to appear, as they are constantly yielding to the barrage of images.
    2. **The "Buried" Video LIFO Trap:** We are still using `prepend()` (LIFO) for insertions. Because videos are processing 5x slower now, the Video Queue can grow large during scrolling. If you scroll past 50 videos, then stop, the videos you are currently looking at are at the *front* of the queue, but the older videos are buried. If the ratio throttling is too aggressive, it can feel like videos have "stalled out" entirely.

---

## 2. Image Decoding & Scaling

### The "Before" State: Blind Upscaling
* **Mechanism:** We blindly called `QImageReader::setScaledSize(requestedSize)` on every single image.
* **The Flaw:** If an image was naturally small (e.g., a 64x64 icon), the CPU would mathematically stretch it to 256x256 before returning it. This wasted CPU cycles and inflated the memory footprint of tiny images by over 1600% (from ~16KB to ~262KB).

### The "After" State: Size-Aware Bypassing
* **Mechanism:** We now call `QSize originalSize = reader.size();` to peek at the image's true dimensions *before* decoding. If it's smaller than the thumbnail boundary, we bypass the CPU scaling completely, returning the raw small image and letting the GPU visually stretch it on the screen for free.
* **Potential Starvation / Delay Signals:**
    1. **Double-Reading I/O Penalty:** Calling `reader.size()` forces `QImageReader` to open the file and parse the image headers *before* it actually begins the heavy pixel decoding phase (`reader.read()`). On lightning-fast local NVMe SSDs, this is virtually instantaneous. However, on extremely slow mechanical drives or network shares (where I/O roundtrips are expensive), this two-step process (Read Header -> Read Pixels) effectively doubles the I/O latency for every single image. If thousands of images are requesting their headers simultaneously over a network, the I/O pool will bottleneck, causing the exact stalling behavior you might be experiencing.

---

## Summary of Suspects
If you are seeing thumbs failing to load or severely lagging right now, the most likely culprits are:
1. **Network/Disk I/O Choke:** The new `reader.size()` call is causing an I/O bottleneck by forcing a preliminary header read on every file.
2. **Video Queue Throttling:** The 5:1 ratio is successfully keeping images fast, but it is causing the Video queue to grow so large during scrolling that videos appear completely stuck.
