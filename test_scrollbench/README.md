# ScrollBench - Viewport Culling & Frame Budget Test Application

## Purpose

ScrollBench is an isolated test application designed to validate viewport culling and frame budget optimizations for handling 10,000+ image thumbnails without blocking the UI thread.

## Features

### Optimizations Under Test

1. **Viewport Culling**
   - Only loads thumbnails for visible items +/- 10 item buffer
   - Dynamically updates visible range as user scrolls
   - Prevents unnecessary decode queue buildup

2. **Frame Budget**
   - Limits texture uploads to configurable amount per frame (default: 10)
   - Uses 16ms frame boundary detection (60 FPS target)
   - Defers excess completions to next frame to prevent UI stuttering

3. **Performance Telemetry**
   - Real-time FPS monitoring
   - Decode queue depth tracking
   - Memory usage tracking (Windows)
   - Per-frame completion statistics

### UI Features

- **10,000 Item ListView**: Synthetic colored rectangles with metadata
- **Performance Overlay**: Real-time stats display
- **Toggle Controls**: Enable/disable optimizations for A/B testing
- **Configurable Frame Budget**: Slider to tune uploads per frame

## Building

### Prerequisites

- Qt 6.4+
- CMake 3.16+
- C++17 compiler (MSVC 2019+ on Windows)

### Build Steps

```powershell
cd test_scrollbench
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### Run

```powershell
.\Release\appScrollBench.exe
```

### Network Diagnostics (Watchdog)
If running over a network share where execution is unstable or silent crashes occur:
```powershell
.\Release\appScrollBenchNet.exe
```
**Note:** `appScrollBenchNet.exe` is a temporary debug watchdog. It launches the main app, captures exit codes, holds the terminal open on crash, and dumps `logs/crash.log` contents. This is NOT the main application and should be cleaned up after network diagnostics are completed.

## Testing Methodology

### Baseline Test (No Optimizations)

1. Launch ScrollBench
2. Disable both "Viewport Culling" and "Frame Budget" toggles
3. Rapidly scroll through the 10,000 item list
4. **Expected**: FPS drops below 30, queue depth grows to 100+, UI stutters

### Viewport Culling Only

1. Enable "Viewport Culling" toggle
2. Disable "Frame Budget" toggle
3. Rapidly scroll
4. **Expected**: Queue depth controlled (<50), FPS improves but may still drop

### Frame Budget Only

1. Disable "Viewport Culling" toggle
2. Enable "Frame Budget" toggle (set to 10)
3. Rapidly scroll
4. **Expected**: FPS stays ≥30, but queue depth grows, some delayed pops

### Both Optimizations

1. Enable both toggles
2. Scroll through entire list
3. **Expected**: FPS ≥60 normal, ≥30 fast scroll, queue <50, no UI freezes

## Success Criteria

| Metric | Without Optimizations | With Optimizations | Target |
|--------|----------------------|-------------------|---------|
| Scroll FPS | 15-25 | 50-60 | ≥30 |
| Peak Queue Depth | 200+ | 20-50 | <50 |
| UI Responsive | No (stutters) | Yes (smooth) | Yes |
| Memory Usage | 2GB+ | <1GB | <1GB |

## Architecture

```
ScrollBenchImageModel
├── Viewport Culling Logic
│   ├── visibleStartIndex (Q_PROPERTY)
│   ├── visibleEndIndex (Q_PROPERTY)
│   └── updateVisibleRange() (triggers loads)
└── Synthetic Test Data (10,000 colored rectangles)

FrameBudgetScheduler
├── Frame Boundary Detection (16ms timer)
├── Task Queueing (defer if over budget)
└── Configurable Budget (1-50 uploads/frame)

TelemetryMonitor
├── FPS Tracking (rolling 60-frame average)
├── Memory Usage (Windows PROCESS_MEMORY_COUNTERS)
└── Queue Depth Monitoring
```

## Integration Plan

If tests show significant improvement:

1. Integrate `FrameBudgetScheduler` logic into main app's `TaskScheduler`
2. Add `visibleStartIndex`/`visibleEndIndex` to main `ImageModel`
3. Update `GalleryViewTiles.qml` and `GalleryViewSemantic.qml` with viewport tracking
4. Add telemetry to `StatsOverlay.qml`

## Known Limitations

- Synthetic test data (colored rectangles) - not real image decoding
- Simplified loading simulation (no actual AsyncImageProvider integration)
- Windows-only memory monitoring

For real-world validation, integrate optimizations into main application and test with actual image directory.

## License

Same as main Antigravity project (MIT).
