# Known Issues & Workarounds

**Last Updated:** 2026-07-05

---

## 🔴 Open Issues

### 1. Selection/Share Features Only in ScrollBench
- **Issue**: Multi-select and share/resize dialogs are fully implemented in ScrollBench but not yet ported to main application
- **Impact**: Users must use ScrollBench to select multiple images or resize for sharing
- **Workaround**: Use `appScrollBench.exe` for these features
- **Status**: Planned for next release (v2.3.0)

### 2. Case-Sensitive File Extension Matching
- **Issue**: Files with uppercase extensions (`.JPG`, `.PNG`, `.MP4`) may not be detected during directory scanning
- **Root Cause**: Some QDirIterator filter paths use lowercase patterns only
- **Workaround**: Manually add uppercase variants to filter lists
- **Status**: Documented fix available — `docs/FOLDER_SCANNING_DIAGNOSTIC.md`

### 3. Staging Queue Leak on Abandoned Items
- **Issue**: Items re-queued to staging under low-memory+offscreen conditions with no subsequent VRM update will accumulate in `m_stagedRequests` indefinitely
- **Impact**: Memory bloat on very large (100k+) folder scrolls after rapid directional change
- **Status**: Identified, fix planned (age-out expiry for staged items)
- **File**: `src/AsyncImageProvider.cpp` — `processStagedRequests()`

### 4. `isRequestStillNeeded` 2ms Coalesce Window Drop
- **Issue**: New delegate created for a file within the 2ms `scheduleStagingProcessing` delay window may not yet be registered in `m_pendingResponses`, causing the task to be silently dropped
- **Impact**: Occasional blank tile on very fast initial renders
- **Status**: Identified, low frequency, fix planned

### 5. RAM Cache Key Normalization Mismatch
- **Issue**: RAM cache key is `id + "_" + WxH`. If requests arrive with different path forms (raw vs `file://` prefixed), they miss the cache and trigger duplicate decodes
- **Impact**: Wasted CPU/RAM on path format inconsistency
- **Status**: Identified, low priority

---

## ✅ Fixed This Session (Milestone 2 — 2026-07-05)

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
