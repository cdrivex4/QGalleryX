# Updated Priority Plan — 100k+ Image Scale

> Updated: 2026-07-04, incorporating all user feedback

---

## Corrections From Review

> [!NOTE]
> **Stall recovery timer EXISTS.** The `AsyncImageProvider` constructor (line 401) already creates a 5-second `QTimer` that calls `checkStalls()`. The `OUTSTANDING_TASKS.md` was stale — this was implemented at some point and the doc was never updated.
>
> **`activeWeight` double-increment is NOT confirmed.** The code actually has TWO guard mechanisms (`QueueGuardState` for cleared tasks, `DriveConcurrencyGuard` for completed tasks) that both decrement. The increment happens once at admission (line 708). The current accounting appears correct — the issue documented in `OUTSTANDING_TASKS.md` may have already been fixed. We'll audit this during implementation to be sure but it's not a P0.
>
> **Placeholder thumbnails already exist.** Leaving alone.
>
> **Per-format budget weights — skipped.** Each format (RAW, DNG, HEIC) has different processing costs but trying to implement per-format weights is impractical. We continue with the current uniform approach.
>
> **Main app and ScrollBench are INTENTIONALLY different.** The two apps share the engine layer (`src/`) but have separate UIs and processing pipelines by design. The issue is that the main app's pipeline has been accidentally replaced with ScrollBench internals at some point. We need to check git history and restore the original main app pipeline.

---

## Priority Order

### 🔴 P0 — Do Immediately

#### P0.1: Rename `gridSize` → `gridResolution` everywhere
**Rationale:** "Grid size" has been ambiguous for too long. `gridSize` actually controls the **resolution** of the grid cells (how many px each thumbnail renders at). The term "grid size" should be reserved for the actual grid dimensions (number of columns, overall layout). This rename makes the codebase honest.

**Scope (files to change):**

| File | Changes |
|------|---------|
| [SettingsHelper.h](file:///d:/Dev/antigravity/src/SettingsHelper.h) | `Q_PROPERTY gridSize` → `gridResolution`, getter/setter/signal rename |
| [SettingsHelper.cpp](file:///d:/Dev/antigravity/src/SettingsHelper.cpp) | `gridSize()` → `gridResolution()`, `setGridSize()` → `setGridResolution()`, settings key |
| [SettingsHelper.h (legacy)](file:///d:/Dev/antigravity/src_legacy/SettingsHelper.h) | Same renames |
| [SettingsHelper.cpp (legacy)](file:///d:/Dev/antigravity/src_legacy/SettingsHelper.cpp) | Same renames |
| [PerformanceOverlay.qml](file:///d:/Dev/antigravity/test_scrollbench/qml/PerformanceOverlay.qml) | `settings.gridSize` → `settings.gridResolution` (lines 242, 245, 246) |
| [GalleryViewScrollBench.qml](file:///d:/Dev/antigravity/test_scrollbench/qml/GalleryViewScrollBench.qml) | All `settings.gridSize` refs (~8 locations) |
| [GalleryViewSemanticScrollBench.qml](file:///d:/Dev/antigravity/test_scrollbench/qml/GalleryViewSemanticScrollBench.qml) | All `settings.gridSize` refs (~15 locations) |
| Legacy QML (qml_legacy/) | `appSettings.gridSize` refs |

**Important:** The QSettings storage key should stay as `"gridSize"` for backwards compatibility (so existing user settings don't reset), but add a migration comment.

---

#### P0.2: Fix the slider bug (`onMoved` → `onPressedChanged`)
The Grid Zoom slider (now "Grid Resolution" slider) fires `onMoved` on every pixel of drag, causing hundreds of relayouts/second. Change to `onPressedChanged` to only apply when the user releases.

Also fix the same pattern on Cache Size and Thread Count sliders.

---

#### P0.3: Replace `beginResetModel` with batched `beginInsertRows`
In `ImageModel::scanDirectory()`, replace the current pattern:
```cpp
// CURRENT (kills QML at 100k)
beginResetModel();
m_images = batch;
std::sort(...);
endResetModel();
```
With incremental batched insertion:
```cpp
// NEW (smooth at any scale)
// Insert 100 items per frame tick
```

---

### 🟡 P1 — Do Next

#### P1.1: Wire FrameBudgetScheduler into main app
The main app creates a `FrameBudgetScheduler` but never connects it to the decode pipeline. Wire it up identically to how ScrollBench does it.

#### P1.2: Defer `QFileInfo` stat calls (lazy metadata)
During scan, only collect filenames. Insert placeholder `ImageInfo` records with empty dates. Lazy-load date/size metadata when items enter the viewport + buffer zone.

#### P1.3: Viewport buffer with background processing
Add 2-screen buffer zones (`visible ± 200`) for priority loading, BUT:
- **Offscreen items still process** — especially when disk cache is on and the user has a large set
- The buffer just controls **priority**, not whether items get processed at all
- Visible = Immediate priority, Buffer = Normal priority, Offscreen = Background priority (current `TaskScheduler::Background`)

#### P1.4: Pre-sort files during MFT scan
Modify `FastVolumeScanner` to extract `CreationTime` from MFT records during the scan itself, then return results already sorted. Eliminates the O(n log n) sort for 100k items on the main thread.

#### P1.5: Audit & fix `activeWeight` accounting
The code has two guard mechanisms (`QueueGuardState` + `DriveConcurrencyGuard`) that both decrement `activeWeight`. Audit all paths to confirm there's no leak or double-decrement. Update `OUTSTANDING_TASKS.md` to reflect current state.

---

### 🟢 P2 — Architecture Improvements

#### P2.1: 1GB GPU Texture Atlas for Thumbnails
Allocate a 1GB VRAM texture atlas for thumbnail storage. Similar to how the old application worked:
- Thumbnails (16-128px, capped resolution) are packed into the atlas
- Application UI icons/overlays are optimised to consume minimal atlas space
- Custom `QSGNode` renders from atlas coordinates — zero per-item texture upload
- Atlas uses LRU eviction when full

#### P2.2: Memory-Mapped Thumbnail Cache with Tiered Fallback
Implement a 4-tier cache hierarchy:

```
┌─────────────────────┐
│  1. GPU VRAM Atlas   │  ← 1GB, fastest, per-session
│     (1GB)            │
├─────────────────────┤
│  2. System RAM Cache │  ← User-configurable via slider
│     (slider-tuned)   │
├─────────────────────┤
│  3. Disk Cache       │  ← Memory-mapped binary file
│     (mmap'd)         │  ← Pointer dereference, no malloc
├─────────────────────┤
│  4. Source Disk       │  ← Full decode from original file
│     (actual I/O)     │
└─────────────────────┘
```

Cache miss flow: VRAM atlas → check RAM → check disk cache (mmap) → decode from source

#### P2.3: LRU Mathematical Model for Dynamic IO Scaling
Develop a model that integrates with existing streaming protocols:

- **Inputs:** viewport velocity, drive latency (from `DriveStats.avgLoadTimeMs`), active decode count, cache hit ratio
- **Outputs:** optimal concurrency limit per drive, LRU eviction threshold, prefetch distance
- **Equation basis:** `optimalConcurrency = f(driveLatency, hitRate, scrollVelocity)` where:
  - Fast scroll → reduce concurrency (items will pass by before decode completes)
  - High cache hit → increase prefetch distance
  - High drive latency → reduce concurrency, increase RAM cache weight

#### P2.4: Multi-Resolution Thumbnail Pyramid
Pre-generate thumbnails at fixed tiers during first scan:
- **16px** — fast scroll / year view
- **64px** — month view
- **128px** — day view (ceiling)

Show smallest tier during fast scroll, upgrade when velocity drops. Thumbnail resolution slider range capped to **16-128px**.

---

### 🔵 P3 — Restore & Reconcile

#### P3.1: Restore Main App Pipeline from Git History
The main app (`appSamsungGallery`) has had its internals accidentally cross-contaminated with ScrollBench code at some point. Need to:
1. Check git log for commits that modified `src/main.cpp` or `resources/qml/Main.qml` unexpectedly
2. Identify which commits swapped the pipeline
3. Restore the original main app processing pipeline
4. Verify the main app uses `src/` (its own engine) and NOT `test_scrollbench/src/` components directly

#### P3.2: Update `OUTSTANDING_TASKS.md`
Remove stale entries:
- ~~Stall recovery timer missing~~ — it exists (5s timer in `AsyncImageProvider` constructor)
- ~~`activeWeight` double-increment~~ — guards appear correct, needs audit only
- Add new entries from this plan

---

### ⚪ P4 — Future / When Ready

#### P4.1: SQLite-backed model
Replace `QList<ImageInfo>` with a SQLite database for persistent indexed storage.

#### P4.2: Hierarchical date clustering
"June 2026 — 3,412 photos" clusters instead of showing 100k individual items.

#### P4.3: GPU compute shader decode pipeline
Experimental: WebGPU compute shaders for JPEG/PNG decoding directly on GPU.

---

## Summary Table

| # | Task | Type | Est. Effort | Status |
|---|------|------|-------------|--------|
| P0.1 | Rename `gridSize` → `gridResolution` | Rename | 1-2 hours | ⬜ TODO |
| P0.2 | Fix slider `onMoved` → `onPressedChanged` | Bug fix | 15 min | ⬜ TODO |
| P0.3 | Batched `beginInsertRows` | Architecture | 2-4 hours | ⬜ TODO |
| P1.1 | Wire FrameBudgetScheduler to main app | Integration | 1 hour | ⬜ TODO |
| P1.2 | Lazy metadata (defer stat calls) | Performance | 3-4 hours | ⬜ TODO |
| P1.3 | Viewport buffer + background processing | Performance | 2-3 hours | ⬜ TODO |
| P1.4 | MFT pre-sort by CreationTime | Performance | 3-4 hours | ⬜ TODO |
| P1.5 | Audit activeWeight accounting | Bug audit | 1 hour | ⬜ TODO |
| P2.1 | 1GB GPU texture atlas | Architecture | 3-5 days | ⬜ TODO |
| P2.2 | Memory-mapped cache + tiered fallback | Architecture | 2-3 days | ⬜ TODO |
| P2.3 | LRU mathematical model | Design | 1-2 days | ⬜ TODO |
| P2.4 | Multi-res thumbnail pyramid (16-128px) | Feature | 1-2 days | ⬜ TODO |
| P3.1 | Restore main app pipeline from git | Investigation | 2-4 hours | ⬜ TODO |
| P3.2 | Update OUTSTANDING_TASKS.md | Documentation | 30 min | ⬜ TODO |

> [!TIP]
> **Recommended execution order:** P0.1 + P0.2 first (quick wins, unblock terminology), then P0.3 (biggest perf impact), then P1.x in parallel, then P2.x as a cohesive architecture block, with P3.1 investigation running asynchronously whenever you want to check git history.
