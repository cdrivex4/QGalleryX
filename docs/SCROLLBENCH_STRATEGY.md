# ScrollBench Development Strategy

## Overview

ScrollBench is a **parallel test application** being developed alongside the main gallery app. It serves as a controlled environment to test and refine performance-critical features before integrating them into the main application.

## Why Two Apps?

### ScrollBench Purpose
- **Performance Testing**: Isolated environment for testing viewport culling, frame budgets, and scroll performance
- **Feature Prototyping**: New features (selection, share/resize) built here first
- **Risk Mitigation**: Breaking changes don't affect the stable main app
- **Benchmarking**: Clean slate for measuring performance improvements

### Main App Status
- **Production Ready**: Stable version with network support, RAW processing, albums
- **Feature Rich**: PhotoViewer, semantic grouping, date scrubber, stats overlay
- **Deployment Target**: What users actually run

## Development Process

### Phase 1: Build to Parity (Current)
ScrollBench is being built up with:
- [x] Viewport culling & frame budget system
- [x] Grid zoom & thumbnail controls
- [x] Long-press selection mode
- [x] PhotoViewer integration
- [x] Video thumbnail support
- [x] RAW format support
- [x] Settings persistence
- [/] Share/Resize dialog (UI placeholder)

### Phase 2: Merge Features Back
Once ScrollBench reaches feature parity and proves stable:
1. Performance improvements → Main app
2. Selection system → Main app
3. Share/Resize → Main app
4. Verified optimizations → Main app

### Phase 3: Codebase Consolidation (July 2026)
**COMPLETE**: ScrollBench's modernized, high-performance features (MFT, Selection, FastVolumeScanner, Caching) have been merged directly into the `QGalleryX` primary target.
- **Main App (`QGalleryX`)**: The actively developed, stable production target. It now leverages the `src/` backend alongside the `qml_legacy` UI.
- **ScrollBench**: Returns to its original purpose as an isolated testing environment (`test_scrollbench`) for experimental modules before they graduate to the main app.

## Key Differences

| Feature | QGalleryX (Production) | ScrollBench (Test Bench) |
|---------|------------------------|--------------------------|
| **Focus** | Production Release | Experimental Dev |
| **Stability** | Highly Stable | Fluid |
| **Scanner** | **MFT / FastVolumeScanner** | Experimental Scanners |
| **Caching** | **Hierarchical Disk Cache** | Ephemeral / Disabled |
| **Build** | Standard Deployment | `+ Linkage Verification` |

## Files Layout

```
QGalleryX/
├── src/              # Modern Backend (MFT, Caching, Schedulers)
├── src_legacy/       # Legacy models slowly being phased out/migrated
├── resources/        # Production UI (qml_legacy)
├── test_scrollbench/ # Isolated Test Bench
└── docs/             # Shared docs
```

## Current Status (July 2026)

**QGalleryX**: v2.5 - Full MFT Integration, Hierarchical Cache, D3D11 Video Decodes, Robust UI.
**ScrollBench**: Test bench for future experimental refactors.
