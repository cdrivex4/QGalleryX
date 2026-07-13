# Full Architecture Comparison Report
## `appSamsungGallery` (Legacy Engine) vs `test_scrollbench` (Modern Engine)

---

## Part 1: Memory & Cache Architecture

### 1A. RAM Cache

| Layer | Legacy (`appSamsungGallery`) | Modern (`test_scrollbench`) |
|---|---|---|
| **Cache type** | `QCache<QString, QImage>` with mutex lock — in `AsyncImageProvider` | Same `AsyncImageProvider`, shared via `../../src/` include |
| **Cache key** | `filePath + "_" + width + "x" + height` | Same scheme |
| **Cache size** | Configurable via `SettingsHelper::cacheSizeMB`, default 512 MB | Same setting, same `AsyncImageProvider` |
| **Cache stats** | `appSettings.getCacheStats()` → `AsyncImageProvider::getCacheStats()` | Not exposed to QML stats panel directly |
| **Cache invalidation** | Never invalidated unless manually cleared via `clearCache()` | Same |
| **Problem** | `StatsOverlay.qml` reads `appSettings.getCacheStats()` once per second, but `getCacheStats()` only returns `totalCost` in **KB** (not MB). The QML displays `cacheCost / 1024` treating it as KB → MB, which is **correct**, BUT `insertCachedImage` calculates cost as `image.sizeInBytes() / 1024` (= KB), which means the cache fills up much faster than the MB ceiling suggests. No memory pressure eviction exists beyond `QCache` auto-eviction. | Same issue |

> **Root cause of RAM display**: Cache stats are wired correctly but the displayed number is real. A 512 MB "limit" actually fills in wall-clock time based on how many images pass through — and if images are large, the QCache fills faster than you'd expect.

---

### 1B. GPU VRAM

| Layer | Legacy | Modern |
|---|---|---|
| **VRAM tracking** | `SystemMonitor` → PDH counter `GPU Engine\*engtype_3D*` via Windows PDH API | Same `SystemMonitor` (shared from `src_legacy`) |
| **Displayed in stats** | ✅ `systemMonitor.gpuVramUsedMB` / `gpuVramTotalMB` — bound correctly in `StatsOverlay.qml` line 143 | Not displayed in ScrollBench stats panel |
| **VRAM used by app** | Qt Scenegraph + texture atlas. Each thumbnail uploaded to GPU VRAM as `QSGTexture`. Size = `(thumbnailPx)² × 4 bytes` per image. At 150px thumbnails: ~90KB/image × 8948 items = **~750 MB peak** before QCache kicks in. | Same — but the `FastImageItem` (`QQuickItem` subclass) gives finer control over when textures are destroyed |
| **VRAM eviction** | None. Qt Scenegraph owns texture lifetime. When scrolling fast, old textures may not be freed immediately. | Slightly better: `FastImageItem` explicitly invalidates textures on culled items |

---

### 1C. Disk Load & Caching

| Layer | Legacy | Modern |
|---|---|---|
| **Thumbnail source** | `image://async/` → `AsyncImageProvider::requestImageResponse()` → `TaskScheduler` → disk read | ScrollBench uses same `AsyncImageProvider` via `../../src/` |
| **Disk caching** | **None.** Every app restart re-reads all thumbnails from disk | **None** — same problem |
| **RAW handling** | `LibRaw::unpack_thumb()` first (fast embedded preview), fallback to half-size full decode | Same (`AsyncImageProvider` is shared code) |
| **Network drive detection** | ❌ Not implemented — treats I:\ (your SD card) as a local drive, no special throttling | ✅ `QStorageInfo` checks if drive letter maps to UNC path; uses non-incremental scan strategy for network paths |
| **Scan strategy** | Single batch: scans all files, then `beginResetModel`/`endResetModel` once. | ✅ **Incremental**: updates UI at 10, 50, 200, 1000 items, then every 2000 — you see images appear progressively |
| **MFT Fast Scan** | ❌ Not implemented | ✅ `FastVolumeScanner` uses Windows MFT (Master File Table) direct access for local NTFS drives — dramatically faster scan on local drives |

---

## Part 2: Stats Overlay — Wiring Audit & 1 FPS Bug

### The FPS Counter

The `FrameAnimation` in `StatsOverlay.qml` (lines 343–354) counts render frames:

```qml
FrameAnimation {
    running: root.visible
    onTriggered: {
        root.frameCount++
        var now = new Date().getTime()
        if (now - root.lastTime >= 1000) {
            root.fps = root.frameCount
            root.frameCount = 0
            root.lastTime = now  // ← BUG: never initialized!
        }
    }
}
```

**Root cause of 1 FPS display:**
- `root.lastTime` is declared as `property int lastTime: 0`
- On first trigger, `now - 0 = current Unix timestamp in ms ≈ 1,750,000,000,000` which is `>> 1000`
- This passes the `>= 1000` check immediately, so `fps = 1` on the very first frame
- `lastTime` is then set to `now`, and from that point it works correctly — BUT the display reads "1" for the first second and then jumps to the real FPS
- **Additionally**: if the stats overlay is hidden and re-shown, `lastTime` resets to 0 again, causing the same "1 FPS" flash

**Fix**: Initialize `lastTime` to the current time when first activated:

```qml
FrameAnimation {
    running: root.visible
    onRunningChanged: if (running) root.lastTime = new Date().getTime()
    onTriggered: { ... }
}
```

---

### Stats Wiring Diagram

```
C++ Backend                          QML StatsOverlay
──────────────────────────────────────────────────────
SystemMonitor                        
  ├─ cpuUsage          ──────────►  Timer (1s) → cpuHistory[] → UsageGraph
  ├─ memoryUsageMB     ──────────►  Timer (1s) → ramHistory[] → UsageGraph
  ├─ gpuUsage          ──────────►  Timer (1s) → gpuHistory[] → UsageGraph
  ├─ gpuVramUsedMB     ──────────►  Timer (1s) → vramHistory[] → UsageGraph
  ├─ gpuVramTotalMB    ──────────►  "VRAM: X / Y MB" text
  └─ gpuName           ──────────►  "GPU: ..." text  ✅ all wired

SettingsHelper                       
  └─ getCacheStats()   ──────────►  Timer (1s) → cacheCost/cacheMax  ✅ wired
                                    Displays: cacheCost/1024 / cacheMax/1024 MB

AsyncImageProvider
  └─ [static members]  ◄──────────  setCacheMaxCost() called from setCacheSizeMB()
                                    ✅ cache size respects settings slider

Main.qml
  └─ statsOverlay.apiName ◄───────  appSettings.graphicsApi  ✅ wired

GalleryViewSemantic
  └─ imageLoaded(timeMs) ─────────► statsOverlay.reportLoadTime()  ✅ wired

PhotoViewer
  └─ imageLoaded(timeMs) ─────────► statsOverlay.reportLoadTime()  ✅ wired

❌ MISSING: ImageModel.isLoading is NOT connected to StatsOverlay
❌ MISSING: Loaded count / total count not shown in stats  
❌ MISSING: GalleryViewTiles does NOT emit imageLoaded — tiles view shows 0 load times
❌ MISSING: Active thread count not shown
❌ FPS counter initializes to 1 on first tick (see above)
```

---

## Part 3: Feature Comparison Checklist

### ✅ Works in ScrollBench, ❌ Missing/Broken in Legacy App

| Feature | ScrollBench | Legacy App | Priority |
|---|---|---|---|
| **MFT fast directory scan** | ✅ `FastVolumeScanner` — scans 100k+ files in < 1s on local NTFS | ❌ `QDirIterator` only — slow on large folders | 🔴 HIGH |
| **Incremental scan UI updates** | ✅ Images appear at 10, 50, 200, 1000 items progressively | ❌ One big `beginResetModel`/`endResetModel` at the end — blank screen during scan | 🔴 HIGH |
| **Network drive detection** | ✅ `QStorageInfo` checks if mapped drive is UNC — disables incremental mode | ❌ Not implemented — your I:\ SD card gets treated as local | 🔴 HIGH |
| **Burst photo detection** | ✅ Groups photos taken < 2s apart as a "burst" | ❌ Not implemented | 🟡 MED |
| **File type verification** | ✅ `FileTypeRouter::verifyFileType()` reads magic bytes, not just extension | ❌ Extension only — misses renamed files | 🟡 MED |
| **`IsVideoRole` as model data** | ✅ Stored in model (`isVideo` field) — no QML-side `DesktopHelper` call per cell | ❌ Calls `desktopHelper.getFileType(filePath)` per cell in QML — **expensive** | 🔴 HIGH |
| **Sort modes (1–5)** | ✅ `setSortMode()` — Date Asc/Desc, Name, Size, Type | ❌ Always date-descending, no UI to change | 🟡 MED |
| **Live text search / filter** | ✅ `filterQuery` property — live search in filename | ❌ Not implemented | 🟡 MED |
| **`rotateSelected()` on model** | ✅ Batch rotate selected images in C++ | ❌ `rotateClicked` signal goes to `resizeEditor.open()` — wrong dialog | 🔴 HIGH |
| **`cancelScan()`** | ✅ Abort in-progress scan via generation counter | ✅ Legacy also has `++m_scanGeneration` check — works | ✅ |
| **Selection `selectRange(start, end)`** | ✅ Range select | ❌ Not implemented | 🟡 MED |
| **Selection visual rect** | ✅ `selectVisualRect()` for rubber-band selection | ❌ Not implemented | 🟢 LOW |
| **`getMetadata(index)`** | ✅ Returns EXIF including camera, ISO, aperture | ✅ Also implemented in legacy with LibRaw | ✅ |
| **`cropImage(index, rect)`** | ✅ | ✅ | ✅ |
| **`rotateImage(index, deg)`** | ✅ | ❌ Header has it, `.cpp` missing implementation | 🔴 HIGH |
| **VRAM stats in overlay** | ✅ (via `systemMonitor`) | ✅ Wired in StatsOverlay | ✅ |
| **FPS counter** | ✅ (works correctly) | ❌ Shows "1" on first tick — `lastTime` init bug | 🟡 MED |
| **Load time tracking in tiles view** | ✅ | ❌ `GalleryViewTiles` doesn't connect `imageLoaded` to stats overlay | 🟡 MED |
| **`isLoading` scan progress in overlay** | ✅ `scannedCount` / `totalItems` in ScrollBench panel | ❌ Not shown in legacy StatsOverlay | 🟡 MED |
| **Disk cache (persistent thumbnails)** | ❌ Not implemented | ❌ Not implemented | 🟢 LOW (future work) |

---

## Part 4: Recommended Backport Plan (Safe, No FrameBudgetScheduler)

These can be ported **without** touching the rendering pipeline:

### Phase 1 — Immediate Wins (Small changes, high impact)
1. **Fix FPS counter init bug** — 3-line QML fix in `StatsOverlay.qml`
2. **Wire `onRotateClicked` to `imageProcessor.rotateImage()`** — currently opens resize dialog
3. **Add `IsVideoRole` to legacy `ImageModel`** — eliminate per-cell `desktopHelper.getFileType()` in QML
4. **Add `rotateImage(index, degrees)` to `src_legacy/ImageModel.cpp`** — header declares it, body missing
5. **Connect `GalleryViewTiles` `imageLoaded` signal to StatsOverlay**

### Phase 2 — Scan Engine Improvements (Medium effort, big UX improvement)
6. **Add incremental scan updates** — dispatch `beginResetModel`/`endResetModel` at 50/200/1000/2000 item intervals during `scanDirectory`
7. **Add network drive detection** via `QStorageInfo` — prevent I/O thrashing on your SD card
8. **Add scan progress to StatsOverlay** — show "Scanned X / Y" during scan

### Phase 3 — Selective Feature Ports (Larger work)
9. **Port `FileTypeRouter::verifyFileType()`** — magic byte checking, not extension guessing
10. **Port burst photo detection** — tag photos < 2s apart as burst group
11. **Port live search/filter** — `filterQuery` property on `ImageModel`
12. **Port `sortMode`** — add sort UI to Stats Settings tab
