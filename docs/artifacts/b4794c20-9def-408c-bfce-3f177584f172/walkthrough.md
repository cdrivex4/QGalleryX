# ScrollBench Migration & Fixes

## Overview
We have successfully migrated `AsyncImageProvider` to `ScrollBench`, resolved deployment issues, aligned the project structure, and **eliminated UI freezes during directory scanning**.

## Changes

### 1. Async Scanning (Performance Fix)
- **Problem**: UI froze when scanning large directories (esp. network drives like `\\quake2`) because scanning was on the Main Thread.
- **Fix**: Implemented `TaskScheduler` in `ScrollBenchImageModel`.
- **Result**: Scanning now happens in a background thread. UI remains responsive. Thumbnails populate as they are found.
- **Critical Fix (RAM)**: Updated `MainScrollBench.qml` to respect `isLoaded` flag. Previously, real images loaded immediately, bypassing the Frame Budget. Now, they only load when the C++ culling logic permits.
- **Critical Fix (Culling)**: Implemented robust viewport calculation in `MainScrollBench.qml` (multi-point probing + math fallback). This prevents `visibleEndIndex` from getting stuck at 0 (which stopped thumbnail loading).
- **Feature (Drag Select)**: Implemented "Visual Rubber Band" selection. Uses `Rectangle` in QML to effectively calculate 2D grid logic in C++ (`selectVisualRect`). Matches Windows Explorer "box" selection behavior.
- **Interaction Fix**: Bound `GridView.interactive` to disable scrolling while dragging the selection box. This prevents the "layer move vs cursor move" conflict.
- **Safety**: Existing "Viewport Culling" and "Frame Budget" logic was **strictly preserved**.

### 2. Structural Alignment
- **Output Location**: `test_scrollbench/deploy/`.
- **Build Script**: Updated to deploy to this new location.

### 3. Unified Real Image Loading
- **AsyncImageProvider**: Integrated into ScrollBench.
- **SystemMonitor**: Added as a dependency.

### 4. Deployment Fixes
- **Missing DLLs**: `windeployqt` now runs for `appScrollBench.exe`.

### 5. UI Adjustments
- **Thumb Resolution**: Lowered minimum resolution to **20px**.

## Verification
### functionality
- [x] **Launch**: App runs from `test_scrollbench/deploy/appScrollBench.exe`.
- [x] **Network Scan**: Pointing to `\\quake2` performs async scan without freezing UI.
- [x] **Performance**: Frame Budget controls loading speed.

### Files
render_diffs(file:///d:/Dev/antigravity/test_scrollbench/src/ScrollBenchImageModel.cpp)
render_diffs(file:///d:/Dev/antigravity/test_scrollbench/src/ScrollBenchImageModel.h)
