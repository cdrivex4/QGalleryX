// Update audit_notes.md with deep-dive findings from AsyncImageProvider
// ... (keeping previous content)

### 🛠️ Technical Overview: `AsyncImageProvider`
*   **Role:** The "Pixel Engine." It is a high-performance, asynchronous bridge that converts file paths into `QImage` objects for the QML UI.
*   **Key Responsibilities:** Managing an in-memory `QCache`, handling threading/prioritization of image loads, and integrating specialized decoders (LibRaw for RAW files, VideoThumbnailer for video).

### 🔍 Audit Findings: `AsyncImageProvider`

#### **Strengths (The "Good")**
*   **Request Deduplication:** If multiple QML components request the same image simultaneously, it detects the pending task via `m_pendingTasks` and attaches the new response to the existing work. This prevents redundant disk/CPU usage.
*   **Intelligent Prioritization:** It uses a custom `TaskScheduler::Priority`. "Viewer" requests (large images) are treated as `Immediate`, while "Grid" or "Precache" requests are `Low` priority, ensuring smooth scrolling even during background loading.

*   **Advanced Resource Control:** Uses `QSemaphore` (`s_videoSemaphore`, `s_rawSemaphore`) to limit the number of concurrent high-cost RAW and Video decodes, preventing system-wide resource exhaustion.

#### **Potential Risks & Technical Debt (The "Bad/Ugly")**
*   **Memory Management Risk (High):** The cache key is a string concatenation: `path + "_" + width + "x" + height`. While functional, frequent string allocations for every single request could lead to high memory fragmentation during rapid scrolling.
*   **The "Heavy-Load" Fallback:** For RAW files, the code attempts an "Embedded Preview" first (Fast), but falls back to a full decode (Slow) if that fails. This fallback is a huge performance hit and is handled by `TaskScheduler`, which could lead to massive task queues if many large RAWs are requested at once.
*   **Thread Safety Complexity:** The use of `QMutexLocker` around the entire cache access and pending task management is safe but could become a contention bottleneck on high-core-count CPUs during rapid scrolling.
*   **Unpredictable Cache Eviction:** Relys on `QCache::setMaxCost`. Since cost is calculated in KB, a very large image can wipe out hundreds of smaller thumbnails instantly, causing "flicker" as the UI re-requests them.

#### **Wiring Connections (Updated)**
*   `AsyncImageProvider::requestImageResponse` $\leftarrow$ Called by QML `Image` component.
*   `AsyncImageProvider::processImageTask` $\to$ Dispatched to `TaskScheduler`.
*   `AsyncImageProvider::precache` $\to$ Triggers a "Dummy" task in `TaskScheduler` to warm up the cache.
*   **The Decoding Pipeline:** 
    1.  `FileCacheManager` (Check Disk Cache) $\to$ 2. `LibRaw`/`VideoThumbnailer` (Decode) $\to$ 3. `AsyncImageProvider::insertCachedImage` (Update RAM Cache) $\to$ 4. `AsyncImageResponse::handleDone` (Deliver to QML).
*   **Latency Tracking:** `PassiveReadLatencyGuard` wraps the entire loading process to monitor how long disk reads are stalling the UI thread.
