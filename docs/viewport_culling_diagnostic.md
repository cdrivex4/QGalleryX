# Viewport Culling & Settings Diagnostic

## Issue Identified
**Problem**: Only 19 items are being loaded regardless of viewport size

## Root Causes

### 1. **Semantic View Range Detection**
- `updateVisibleRange()` uses `itemAt(25, 20)` and `itemAt(25, list.height - 20)`
- This can hit header rows (type === 0) which don't have image data
- If both coordinates hit the same row or headers, the range is incorrectly narrow

### 2. **Buffer Size Constraint**
- `BUFFER_SIZE = 10` items ahead/behind
- If `m_visibleStartIndex` and `m_visibleEndIndex` are close (e.g., 0-9), total range is ~19 items
- This explains the exact "19 items" observation

### 3. **Settings Integration Issues to Verify**

#### Viewport Culling Toggle
- **Location**: `PerformanceOverlay.qml:273`
- **Binding**: `imageModel.viewportCullingEnabled`
- **Default**: `true` (from `ScrollBenchImageModel.h:142`)
- **Behavior**:
  - When `true`: Only loads visible range + buffer
  - When `false`: Should load ALL items
  
#### Disk Cache Toggle
- **Location**: `PerformanceOverlay.qml:302`
- **Binding**: `settings.useDiskCache`
- **C++ Property**: `AsyncImageProvider::s_useDiskCache`
- **Need to verify**: Is the toggle actually updating the static variable?

## Action Plan

### Phase 1: Fix Viewport Range Detection
1. Improve `updateVisibleRange()` to skip headers
2. Add fallback logic to ensure we capture actual image rows
3. Add debug logging to show what indices are being set

### Phase 2: Increase Buffer Size
1. Change `BUFFER_SIZE` from 10 to 50 for better coverage
2. Make it configurable via settings if needed

### Phase 3: Verify Settings Integration
1. Add logging when viewport culling is toggled
2. Add logging when disk cache is toggled
3. Verify AsyncImageProvider receives the settings changes
4. Check SettingsHelper property bindings

### Phase 4: Add Telemetry
1. Expose current viewport range in overlay
2. Show actual loaded item count vs total items
3. Display staging queue depth
4. Show disk cache hit/miss ratio

## Expected Behavior After Fix

### With Viewport Culling ON:
- Should load visible items + 50 buffer on each side
- Should update range smoothly during scrolling
- Should cancel out-of-range requests when jumping

### With Viewport Culling OFF:
- Should immediately request ALL items
- Should not cancel any requests
- Should load entire gallery

### Disk Cache:
- When ON: Should save/load from disk
- When OFF: Should skip disk operations entirely
- Should be toggleable in real-time
