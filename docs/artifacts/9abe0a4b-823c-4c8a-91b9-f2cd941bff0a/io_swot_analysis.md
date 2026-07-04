# I/O Architecture & Scaling Analysis (100k+ Images)

## Current Architecture Assessment
The current pipeline leverages:
1. **Background `QDirIterator`** for file scanning (recently optimized via batched insertion).
2. **`TaskScheduler`** (Prioritized queue: Immediate, Normal, Background).
3. **`AsyncImageProvider`** with `FrameBudgetScheduler` for thumbnail decoding.
4. **`VisibleRangeManager`** for viewport tracking (`visible ± 200` buffer zone).
5. **Memory Cache** (`QPixmapCache` / `FileCacheManager`).

### SWOT Analysis

**Strengths**
- Decoding is entirely decoupled from the UI thread (via `AsyncImageProvider`).
- Prioritized queueing prevents offscreen images from delaying visible ones.
- Frame budget scheduler naturally throttles decoded texture upload to the GPU.
- Batched UI model insertion ensures UI remains responsive during mass folder ingestion.

**Weaknesses**
- `QDirIterator` is disk-bound. Iterating 100k items across network/mechanical drives induces 500-2000ms latency just to get metadata.
- Generating and holding 100,000 `QVariantMap` or `ImageInfo` objects in the QML `ListModel` uses significant RAM (~50-100MB minimum).
- Individual thumbnail cache files pollute the disk with tens of thousands of small files (inefficient for NTFS).

**Opportunities**
- **Memory-Mapped Files (mmap)** for caching thumbnails in massive contiguous chunks.
- **SQLite Database** for metadata (dates, sizes), allowing instant O(1) lookups and pre-sorted queries without disk `stat()` calls.
- **Texture Atlasing**: Combining multiple small thumbnails into single GPU textures reduces draw calls exponentially.
- **Hierarchical Loading**: Pre-generating multi-resolution mipmaps for distant semantic zoom tiers.

**Threats**
- **GDI/User Object Exhaustion**: Windows limits the number of open handles (10,000 max by default). If we try to open too many files at once, the OS will crash the app.
- **VRAM Spikes**: Panning through thousands of items quickly could burst VRAM if old textures aren't aggressively evicted.
- **TDR (Timeout Detection and Recovery)**: Pushing too many textures to the GPU in a single frame can cause the driver to restart.

---

## 10 Ways to Improve (Path to 1,000,000 Items)

1. **SQLite Metadata Store (The "Source of Truth")**
   Instead of `QDirIterator` every launch, scan once and save to SQLite. On subsequent launches, instantly `SELECT * FROM metadata ORDER BY date DESC LIMIT 100000`. Only run a lightweight background directory watcher (`QFileSystemWatcher`) to detect changes.
   
2. **Virtual `ListModel` (Windowing)**
   Currently, the `ImageModel` stores all 100,000 items in memory. We should transition to a purely virtual model that only keeps `ImageInfo` structs for items within `visible ± 1000`. The rest are purged and paged from SQLite dynamically.

3. **Memory-Mapped (mmap) Thumbnail Cache**
   Instead of 100,000 individual `thumb_*.jpg` files, use a contiguous `.db` or `.bin` file mapped via `mmap`. Seeking and reading a 256x256 block of bytes from an `mmap` pointer is exponentially faster than opening a new NTFS file handle.

4. **Hierarchical Level of Detail (LOD) Pyramid**
   When the user zooms out so far that 10,000 images are on screen, don't load 10,000 individual thumbnails. Load pre-baked "macro-tiles" (e.g., one texture that represents 100 thumbnails). Google Earth uses this.

5. **Texture Atlasing**
   Pack thumbnails into 4096x4096 textures. A single 4K texture can hold 256 (256x256) thumbnails. This drops GPU draw calls by 256x.

6. **MFT Scanning (Master File Table)**
   Bypass Windows API (`FindNextFile`) and parse NTFS MFT directly. This can enumerate a 1,000,000 file hard drive in < 2 seconds, ignoring standard OS overhead. (Note: `FastVolumeScanner` is our early prototype of this, but it requires Admin rights).

7. **Aggressive VRAM Eviction Policies**
   Implement LRU (Least Recently Used) strict VRAM caps. If VRAM exceeds 70%, immediately delete textures outside the viewport buffer zone.

8. **Zero-Copy GPU Uploads**
   Currently, libjpeg decodes into a CPU buffer, which is then copied to a `QImage`, and then uploaded to OpenGL/D3D. We should use hardware decoders (NVDEC/QuickSync) to decode JPEGs *directly* into GPU VRAM.

9. **Predictive Pre-fetching**
   Analyze scroll velocity. If scrolling down at 500px/sec, shift the loading priority buffer heavily to the bottom and halt top-buffer loading.

10. **Thread Affinity & CPU Pinning**
    Pin high-priority I/O tasks to P-cores, and background pre-fetching tasks to E-cores to prevent OS context-switching overhead.

---

## Mathematical Model for I/O Budgeting

At **60 FPS**, a single frame is **16.67ms**.

**Disk I/O Costs (NVMe SSD)**:
- NTFS Open Handle: `~0.1ms`
- Read 50KB JPEG: `~0.05ms`
- Close Handle: `~0.05ms`
*Total I/O per file = `0.2ms`*

**CPU Decoding (libjpeg-turbo)**:
- Decode 50KB to 256x256 RGB: `~1.5ms`

**GPU Upload (PCIe Gen4)**:
- Upload 196KB (256x256x3) to VRAM: `~0.01ms`

**Total Time per Image** = `~1.7ms` (Assuming purely sequential, no parallelization).

To maintain 60 FPS, we can strictly process **no more than 9 images per frame** (9 * 1.7ms = 15.3ms) if done synchronously on a single thread.

**The Asynchronous Multiplier (Our Architecture):**
We have 4 I/O threads and 4 CPU threads.
- Maximum parallel decode throughput: `4 / 1.5ms = 2.66 decodes per ms`
- Theoretical cap: **44 decodes per frame** (`~2600 decodes per second`).

**The True Bottleneck (The Pager):**
If the user scrolls past 1,000 images in 1 second, they are demanding 1,000 decodes. Our theoretical cap is 2,600, BUT NTFS file handle creation (opening 1,000 files) will choke the I/O bus, raising the `0.1ms` cost to `5ms+` per file due to queue depth limits.

**Conclusion:** 
To handle 100k+ items dynamically, we must bypass individual NTFS file operations (`CreateFile`). The only mathematically viable solution for high-speed scrolling is **Texture Atlasing** combined with **Memory-Mapped Caches** (#3 and #5 above), pulling multiple thumbnails in a single contiguous read.
