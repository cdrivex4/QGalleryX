# Known Issues & Workarounds

**Last Updated:** 2026-07-13

---

## 🔴 Open Issues

- None identified currently.

## ✅ Fixed This Session (Milestone 4 — 2026-07-13)

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

- **Full Pipeline Audit**: `docs/artifacts/9abe0a4b-823c-4c8a-91b9-f2cd941bff0a/pipeline_audit.md`
- **Session Log / TODO**: `docs/TODO.md`
- **Feature Status**: `docs/FEATURES.md`
- **Architecture**: `docs/ARCHITECTURE.md`
- **Format Compatibility**: `docs/FORMAT_COMPATIBILITY.md`
- **Artifact History**: `docs/artifacts/`
