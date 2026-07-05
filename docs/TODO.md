# QGalleryX / Antigravity — Milestone 2 Session Log

## Session Date: 2026-07-04 / 2026-07-05

## Status: Milestone 2 — Thumbnail Pipeline Hardening ✅

---

## Critical Warning (Carry Forward)
> **DO NOT** touch the current HEIC / video decode wiring. It should theoretically break in certain edge cases, but right now it is solid. Leave it alone unless explicitly directed otherwise.

---

## What Was Fixed This Session

### Fix 1 — LIFO Queue Abort Threshold (Dead Thumbnails on Old Hardware)
- **File**: `src/AsyncImageProvider.cpp`
- **Problem**: Hard abort at `activeTaskCount > 5000` killed the NEWEST (most visible) tasks first due to LIFO ordering.
- **Fix**: Removed hard threshold. `isRequestStillNeeded()` naturally culls offscreen tasks.

### Fix 2 — OOM/VRM Race Condition (Permanent Dead Thumbnails on Low RAM)
- **File**: `src/AsyncImageProvider.cpp` — `processStagedRequests()`
- **Problem**: When system RAM < 1GB, offscreen tasks were immediately delivered `QImage()` and permanently cached as placeholders. VRM 50ms timer lag caused visible items to also be misclassified as offscreen.
- **Fix**: Offscreen tasks under low-memory conditions are now re-queued into staging rather than permanently failed.

### Fix 3 — DriveConcurrencyGuard Double-Decrement
- **File**: `src/AsyncImageProvider.cpp` — `~DriveConcurrencyGuard()`
- **Problem**: Stall timer evicted a task from `s_activeTasksMap`, then the destructor decremented `activeWeight` again → drove the counter negative → cascading I/O flood on slow Intel hardware.
- **Fix**: Destructor checks `s_activeTasksMap.remove()` return count before decrementing weight.

### Fix 4 — VRM Pump Missing After Visible Range Update
- **File**: `src/VisibleRangeManager.h` — `setVisiblePaths()`
- **Problem**: When VRM updated its visible set, it never woke the AsyncImageProvider to re-evaluate tasks parked in staging. Tasks could sit in staging indefinitely if no active tasks were running to trigger a pump.
- **Fix**: `setVisiblePaths()` now calls `AsyncImageProvider::scheduleStagingProcessing()` after updating the set.

### Fix 5 — False Positive Stall Detection ("Logic Error?")
- **File**: `test_scrollbench/src/DiagnosticsMonitor.cpp`
- **Problem**: Stall timer fired on `stagedRequests > 0 && pendingRequests == 0` — this is an intentional state (low-memory parking), not a stall. The DiagnosticsMonitor was logging `CRITICAL: Logic Error?` unnecessarily.
- **Fix**: Stall only fires when `pendingRequests > 0` (active tasks in the thread pool). Staged-only items reset the stall timer.

### Fix 6 — Speculative FFmpeg/LibRaw Abort → Cached Placeholder (Dead Video/RAW Thumbs)
- **File**: `src/AsyncImageProvider.cpp` — `processImageTaskInternal()`
- **Problem**: Under high load, offscreen video/RAW tasks were speculatively aborted mid-decode returning `QImage()`. The deliver path then generated a placeholder and cached it permanently.
- **Fix**: Removed the speculative abort logic for video and RAW. Tasks already dispatched to a thread now always finish decoding.

### Fix 7 — FFmpeg Cancellation Token Cross-Delegate Poisoning
- **File**: `src/AsyncImageProvider.cpp` — video path in `processImageTaskInternal()`
- **Problem**: FFmpeg was passed the cancellation token from the *first* requesting delegate (`c.get()`). If that delegate was destroyed (scroll away), FFmpeg would abort mid-decode even if *other* delegates were still waiting for the coalesced result.
- **Fix**: Pass `nullptr` as the cancel token to FFmpeg. Cancellation is handled at the coalesce layer via `shouldAbort()` / `isRequestStillNeeded()`.

### Fix 8 — Placeholder Cache Poisoning on Transient Decode Failure
- **File**: `src/AsyncImageProvider.cpp` — `deliver:` label block
- **Problem**: ANY null result from decoding (OOM, disk hiccup, timeout, speculative abort) triggered placeholder generation and RAM+disk cache insertion, permanently blocking future valid decode attempts.
- **Fix**: Introduced `isGenuinelyBroken` flag. Placeholders are ONLY generated and cached when:
  - The file does not exist on disk, OR
  - `QImageReader::canRead()` returns false for a standard image format (corrupt header).
  - Media formats (video, RAW, HEIC) are NEVER marked as genuinely broken on null return — they return `QImage()` uncached so the next request retries.
- **Also fixed**: Corrupt-file placeholders are no longer written to disk cache (they have no TTL and waste disk space).

### Fix 9 — FastImageItem setSourceSize Renders Stale Stretched Image
- **File**: `test_scrollbench/src/FastImageItem.cpp` — `setSourceSize()`
- **Problem**: When `loadingResolution` changed (grid slider, ctrl+scroll), `setSourceSize()` cancelled and re-requested but left `m_image` populated with the old-size image. QML rendered the stale image stretched during the decode gap.
- **Fix**: `setSourceSize()` now clears `m_image` and calls `update()` before re-requesting, forcing a blank loading state.

### Fix 10 — Initial Video Thumbnails Dropped on Folder Selection
- **File**: `test_scrollbench/src/ScrollBenchImageModel.cpp` — `setVisibleStartIndex()`
- **Problem**: `cancelPendingRequests()` was triggered whenever `delta > 10` items appeared in the visible range. On initial folder selection, the grid jumps from 0 to ~27 visible items (delta > 10), which immediately cancelled all in-flight FFmpeg video decode tasks before they could complete.
- **Fix**: Cancel threshold raised from `delta > 10` to `delta > 80`. Normal scrolling and initial load never hit this. Only genuine scrubber jumps (100+ items) trigger a cancel.

### Fix 11 — Drag-Select Unresponsive (Flickable Stealing Input)
- **File**: `test_scrollbench/qml/GalleryViewScrollBench.qml` & `GalleryViewSemanticScrollBench.qml`
- **Problem**: Users could not draw a selection box. `DragHandler` defaults to `Qt.NoModifier`, meaning holding `Shift` or `Ctrl` ignored the drag, and without modifiers the `Flickable` list simply scrolled.
- **Fix**: Added `acceptedModifiers: Qt.NoModifier | Qt.ShiftModifier | Qt.ControlModifier` to explicitly allow drag-selection when holding a modifier.

### Fix 12 — Massive UI Hang / Crash on Select All / Invert / Drag Select
- **File**: `test_scrollbench/src/ScrollBenchImageModel.cpp` & `test_scrollbench/qml/SelectionActionBar.qml`
- **Problem**: Batch selection operations in QML (like "Invert") were running a Javascript loop that called `toggleSelection(i)` up to 20,000 times. Each call synchronously emitted `selectedCountChanged`, forcing all visible QML delegates to repeatedly re-evaluate their bindings, locking the UI thread.
- **Fix**: Replaced QML-side JS loops with a new native C++ `selectItems(indices)` method that processes bulk index updates in O(N) time without intermediate signals, and explicitly hooked `invertSelection` directly to the C++ method.

---

## Remaining Items (from pipeline_audit.md)

| Priority | Issue | Status |
|----------|-------|--------|
| 🟡 MEDIUM | Disk cache size key consistency verification | Open |
| 🟡 MEDIUM | Staging queue age-out / expiry for leaked parked items | Open |
| 🟡 MEDIUM | `isRequestStillNeeded` can drop tasks during 2ms coalesce window | Open |
| 🟢 LOW | RAM cache key normalization (duplicate decodes on path format mismatch) | Open |
| — | P1.3 Background Pipeline Refactor | Planned |
| — | P1.4 Pre-sort during MFT scan | Planned |
| — | P2.x Caching Strategies | Planned |

---

## Artifacts
- Full pipeline audit: `docs/artifacts/9abe0a4b.../pipeline_audit.md`
- Conversation ID: `9abe0a4b-823c-4c8a-91b9-f2cd941bff0a`
- Brain artifacts: `docs/artifacts/`
