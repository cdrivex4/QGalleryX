# Known Issues & Workarounds

**Last Updated:** 2026-08-18

---

## 🔴 Open Issues & Active Investigations

- 🟡 **Outstanding Cache Misses on Specific Edge-Case Assets**: Investigating intermittent cache misses occurring after crawler passes (e.g. non-standard color spaces, deeply nested filenames, or transient file locks during background crawls).

## ✅ Fixed This Session (Milestone 8 — v2.3.1 Milestone — 2026-08-18)

- ✅ **Prefix-Scoped Cache Reconciliation & Compaction Crash**: Fixed global cross-drive cache purging in `FileCacheManager::pruneStaleEntries()` by strictly scoping deletions to `folderPrefix`, preventing other drives (`C:`, `D:`, `I:`) from being wiped, and eliminated unsafe mid-run mmap unmapping.
- ✅ **Instant Folder DB Metadata Loading**: Added $< 5\text{ms}$ instant UI rendering on drive/folder selection from existing `.bin` caches with parallel non-blocking MFT verification.
- ✅ **Single Source of Truth (SSOT) File Type Pipeline**: Unified all supported extensions and wildcard filters across `DesktopHelper`, `ImageModel`, and UI delegates via `DesktopHelper::supportedExtensions()` and `supportedNameFilters()`.
- ✅ **TypeScript `.ts` vs MPEG Transport Stream Disambiguation**: Added `0x47` header sync-byte verification in `DesktopHelper::staticGetFileType()` to reject TypeScript source code files (`use-history.ts`, `index.ts`) from being processed as videos.
- ✅ **Dynamic DXGI Multi-Adapter Enumeration**: Implemented dynamic adapter iteration across all DXGI devices to support remote desktop / AnyDesk sessions with software rasterizer filtering.

## ✅ Previously Fixed (Milestone 7 — v2.3.0 Milestone — 2026-08-18)

- ✅ **Fixed 1GB Circular Ring Buffer Eviction**: Replaced the flawed 1GB circular ring buffer with an auto-expanding (512MB-chunk) append-only binary mmap database (`FileCache.mmap` v3), permanently eliminating the issue where crawling new drives silently erased previously crawled drives.
- ✅ **Black Box Task Key Deduplication Bug**: Fixed key mismatch in `AsyncImageProvider` that was causing tasks to stall and drop image responses.
- ✅ **Mmap Index Ground-Truth Crawler Work Queue**: Replaced naive sequential cursor walking with an $O(N)$ index-checked missing work queue (`m_crawlWorkQueue`), guaranteeing zero gaps and persistent state across restarts.
- ✅ **Filesystem Reconciliation & Compaction Protocol**: Added `FileCacheManager::pruneStaleEntries()` and `compact()` to automatically purge deleted files from the database and reclaim disk space.
- ✅ **OSD & Menu Cache Telemetry & Controls Synchronization**: Connected both the On-Screen Display (OSD) and the Settings Menu to the exact same backend (`appSettings.getTrackedRootPathStats()`), unified display formats, and added a `Rebuild Cache / Re-Crawl` button to the OSD.
- ✅ **Transparent FFmpeg Fallback for Corrupt EXIF/JPEG Streams**: Integrated `VideoThumbnailer` fallback directly into `AsyncImageProvider` when `QImageReader` fails on non-standard JPEGs.
- ✅ **L1 RAM / L2 Disk Hit Visibility & Logging**: Added visible, throttled console logging for L1 RAM and L2 Disk hits to reflect real-time cache throughput.

## ✅ Fixed This Session (Milestone 6 — 2026-08-03)

- ✅ **TaskScheduler Queue Dequeue Race Condition**: Fixed `0xc0000005` crash in `Qt6Core.dll` caused by iterating `QMap` iterators or calling `dequeue()` on an empty `QQueue` under concurrent worker execution. Replaced map iteration with fixed priority arrays (`{Immediate, High, Normal, Low}`).
- ✅ **Mmap Cache Network Fallback & Ring Buffer Eviction Freeze**: Fixed infinite loop during ring buffer wrapping (`advanceHead()`) and added automatic fallback to `QHashCacheDatabase` if memory-mapping fails over network/SMB shares.
- ✅ **Qt 6 Native AV1 & FFmpeg Multimedia Backend**: Added `qputenv("QT_MEDIA_BACKEND", "ffmpeg")` to bypass Windows Media Foundation (WMF) system codec dependencies, enabling out-of-the-box AV1, VP9, WebM, and MKV video playback.
- ✅ **Deep Copy Cache Retrieval**: Updated `AsyncImageProvider::getCachedImage` to return `img->copy()`, preventing thread race conditions during `m_cache.clear()` RAM purges.
- ✅ **Corrupt Video & Shell Icon Null Guards**: Added `dstW/dstH` and `tmp.bits()` guards in `VideoThumbnailer.cpp` and wrapped `QFileIconProvider` in `try / catch` to prevent network Shell COM crashes.
- ✅ **QML Video Rotation Property Scoping**: Replaced brittle `parent.parent.parent.parent` traversal with explicit `videoContainer.currentRotation` ID binding in `PhotoViewer.qml`.

## ✅ Previously Fixed (Milestone 5 — 2026-08-02)

- ✅ **Unified Album & Grid Data Pipeline**: `AlbumModel` now consumes `ImageModel` in-memory (`sourceModel: imageModel`), eliminating duplicate disk walks and thread contention.
- ✅ **Network Drive Detection Centralized**: Fixed false positive network classification for local NTFS drives (e.g. `I:\`) by probing filesystem type (`DesktopHelper::staticIsNetworkPath`) rather than device path prefix.
- ✅ **Passive IO Latency Guard & Audit Logger**: Added `PassiveReadLatencyGuard` to measure local file read duration, append anomalies to `%LOCALAPPDATA%/.../disk_latency_audit.log`, and emit non-intrusive QML Toast notifications.
- ✅ **Stats Overlay Counter Scoping**: Fixed `Loaded: 0 / ?` counter binding scope in `Main.qml`.
- ✅ **RAW/HEIC Fallback & Edit Overlay**: Added QImageReader fallback for HEIC files without `moov` atoms and routed DNG edit overlay through `image://async/`.

## ✅ Previously Fixed (Milestone 4 — 2026-07-13)

- ✅ **Selection/Share Ported to Main App** → Multi-select, share dialog, and resize editor are now fully integrated into the main gallery view.
- ✅ **Resize Dialog Nested Sizing Bug** → Fixed `ResizeEditor` breaking its layout when instantiated inside `ShareDialog` by binding it to `Overlay.overlay`.
- ✅ **Date Scrubber "Abyss" Scrolling** → Replaced explicit `contentY` math with `positionViewAtIndex` to prevent scrolling into uninstantiated lazy-load regions.
- ✅ **Semantic View Selection Overlay Missing** → Added a QML `Connections` block to dynamically update selection visuals in the Repeater when `selectedCountChanged` fires.
- ✅ **Album View Inner Search Filter Sync** → Added `onActiveModelChanged` in `Main.qml` to ensure the global search bar text is injected into the local Album directory's model upon entering a folder.
- ✅ **Corrupted Text/Icons (??? Back)** → Replaced corrupted unicode characters with proper arrow and media icons.

## ✅ Previously Fixed (Milestone 3 — 2026-07-11)

- ✅ **Case-Sensitive File Extension Matching** → Fixed missing uppercase extension files in scanners
- ✅ **Staging Queue Leak on Abandoned Items** → Implemented 5s age-out expiry for stalled staged requests
- ✅ **isRequestStillNeeded 2ms Coalesce Window Drop** → Replaced non-atomic check with atomic abortIfNotNeeded
- ✅ **RAM Cache Key Normalization Mismatch** → Lowercased Windows path cache keys inside normalizeId
- ✅ **Disk Cache Size Key Consistency** → Verified existing keys are consistent across implementations

## ✅ Previously Fixed (Milestone 2 — 2026-07-05)

- ✅ **LIFO Queue Abort Threshold** → Removed hard 5000-task abort that killed newest visible tasks first
- ✅ **OOM/VRM Race → Permanent Placeholder** → Offscreen tasks re-queued instead of permanently failed
- ✅ **DriveConcurrencyGuard Double-Decrement** → I/O flood on Intel/older hardware fixed
- ✅ **VRM Pump Missing** → VRM now wakes AsyncImageProvider after visible set update
- ✅ **False Positive Stall "Logic Error?"** → DiagnosticsMonitor no longer flags intentional parking
- ✅ **Speculative FFmpeg/LibRaw Abort** → Removed; dispatched tasks always complete their decode
- ✅ **FFmpeg Cancel Token Cross-Delegate Poisoning** → Pass nullptr to FFmpeg, coalesce layer handles cancellation
- ✅ **Placeholder Cache Poisoning on Transient Failure** → `isGenuinelyBroken` flag gates placeholder generation
- ✅ **setSourceSize Renders Stale Stretched Image** → Clears m_image immediately before re-requesting
- ✅ **First ~27 Video Thumbs Dropped on Folder Open** → Cancel threshold raised from 10 to 80
- ✅ **QML Loop Hang on Invert/Drag Select** → Implemented `selectItems` bulk C++ method to prevent n² binding re-evaluations
- ✅ **Drag-Select Unresponsive** → Added `acceptedModifiers` to `DragHandler` to prevent `Flickable` from stealing input

## ✅ Previously Fixed (Milestone 1 — 2026-07-04)

- ✅ **QFileInfo Deferred Loading** → Folder scan no longer stats files during enumeration
- ✅ **FileCacheManager mmap Wrap-Around Crash** → Fixed 0xC0000005 on rapid semantic zoom
- ✅ **Resolution Slider Crash** → Debounced thumbnailSize propagation
- ✅ **Use-After-Free on Rapid Resize** → Fixed thread race in FastImageItem

## ✅ Previously Fixed (v2.2.1 — Feb 2026)

- ✅ **1 FPS UI Slowdown** → Replaced O(N) loops with O(1) counters
- ✅ **Viewport Culling Broken** → Fixed path normalization in VisibleRangeManager
- ✅ **CPU Overload** → Task weighting + CPU-aware backoff throttling

---

## 🔗 Related Documentation

- **Network, Concurrency & Memory Audit Guide**: `docs/NETWORK_CONCURRENCY_LESSONS_LEARNED.md`
- **Full Pipeline Audit**: `docs/artifacts/9abe0a4b-823c-4c8a-91b9-f2cd941bff0a/pipeline_audit.md`
- **Session Log / TODO**: `docs/TODO.md`
- **Feature Status**: `docs/FEATURES.md`
- **Architecture**: `docs/ARCHITECTURE.md`
- **Format Compatibility**: `docs/FORMAT_COMPATIBILITY.md`
- **Artifact History**: `docs/artifacts/`
