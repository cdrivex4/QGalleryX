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
- [/] Share/Resize dialog (in progress)
- [ ] PhotoViewer integration
- [ ] Video thumbnail support
- [ ] RAW format support
- [ ] Settings persistence

### Phase 2: Merge Features Back
Once ScrollBench reaches feature parity and proves stable:
1. Performance improvements → Main app
2. Selection system → Main app
3. Share/Resize → Main app
4. Verified optimizations → Main app

### Phase 3: Deprecate ScrollBench
After successful integration, ScrollBench can be:
- Archived as reference implementation
- Kept as performance test harness
- Removed if no longer needed

## Key Differences

| Feature | Main App | ScrollBench |
|---------|----------|-------------|
| **Focus** | Full-featured gallery | Performance testing |
| **Stability** | Production-ready | Experimental |
| **Network** | Full support | ✅ Async Scanning (v0.4) |
| **RAW Files** | LibRaw integration | Planned |
| **Video** | Basic support | Thumbnails planned |
| **Selection** | Not yet | ✅ Drag & Visual Box |
| **Share/Resize** | Not yet | In progress |

## Commit Strategy

**Main App**:
- Stable features only
- Full testing required
- Tagged releases

**ScrollBench**:
- Rapid iteration
- Breaking changes OK
- Experimental features

## Files Layout

```
antigravity/
├── src/              # Main app C++
├── resources/qml/    # Main app UI
├── test_scrollbench/ # ScrollBench (separate)
│   ├── src/          # Test app C++
│   ├── qml/          # Test app UI
│   └── deploy/       # Isolated binary
└── docs/             # Shared docs
```

## Current Status (Dec 2024)

**Main App**: v2.1.0 - Network & Deployment Stability
**ScrollBench**: v0.4 - Async Scan & Drag Selection

**Next**: Complete Share/Resize dialog, then begin parity assessment
