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

### Phase 3: Long-Term Strategy (Jan 2026)
ScrollBench is now the **primary development platform**.
- **Main App**: Maintained as a legacy reference implementation (`v2.1` frozen).
- **ScrollBench**: All new features (MFT, Selection, Verify Tool) live here.

## Key Differences

| Feature | Main App (Legacy) | ScrollBench (Primary) |
|---------|-------------------|-----------------------|
| **Focus** | Reference Impl. | Production Dev |
| **Stability** | Frozen | Active Dev |
| **Scanner** | Recursive (Slow) | **MFT / FastVolumeScanner** |
| **Selection** | Basic | **Unified (Mouse/Touch/Key)** |
| **Build** | Standard | **+ Linkage Verification** |

## Files Layout

```
antigravity/
├── src/              # Common Backend & Legacy App
├── resources/qml/    # Legacy UI
├── test_scrollbench/ # PRIMARY DEVELOPMENT (ScrollBench)
│   ├── src/          # ScrollBench C++
│   ├── qml/          # ScrollBench UI
│   └── deploy/       # Deployment Target
└── docs/             # Shared docs
```

## Current Status (Jan 2026)

**ScrollBench**: v1.0 - Full MFT Integration, Robust Selection, Verification Tooling.
**Main App**: v2.1 (Legacy) - Reference only.
