# QGalleryX — Feature Backlog

> Items here are **scoped, not forgotten**. They are confirmed-valid features that have been intentionally deferred.
> Last Updated: 2026-08-01

---

## 🗂️ Deferred / Queued

| Priority | Feature | Notes |
|---|---|---|
| Medium | **Scrubber for Tiles/Grid View** | The date scrubber exists on the semantic/grouping view. Tiles view (raw flat grid) needs its own fast-scroll scrubber so users can seek by date or alphabetically without scrolling thousands of thumbnails |
| High | **Drive Surface Health Monitor — Phase 1** | Passive QElapsedTimer wrap around existing AsyncImageProvider file reads. Feed per-drive latency stats to StatsOverlay badge. Zero new threads. See badclus_integration_plan.md |
| High | **Drive Surface Health Monitor — Phase 2** | DriveHealthModel singleton + DriveHealthOverlay.qml with per-drive degradation heatmap, affected files list, and threshold alerts (>80ms warn, >250ms critical) |
| High | **Drive Surface Health Monitor — Phase 3** | Option C in-place data refresh: SHA256-verified stream rewrite + FlushFileBuffers force hardware cache commit. Invoked from DriveHealthOverlay UI |

---

## 📌 Notes on Scrubber Feature

The **tiles view scrubber** should behave similarly to iOS / Samsung Gallery fast-scroll sidebar:
- A thin vertical drag handle on the right side of the grid
- Dragging it seeks by **date group** (year -> month -> day), not by item index
- Should show a floating "bubble" tooltip with the date while dragging (e.g. `July 2021`)
- The existing `DateSectionRole` in `ImageModel` already provides date groupings — data is available
- Distinct from the existing **semantic view scrubber** which is already implemented
