# Critical Memory Leak Fix & Code Review

## Status: ✅ FIXED
**Severity**: High (Potential to crash application during video scrolling)
**Component**: `VideoThumbnailer` (FFmpeg integration)

## The Leak
A significant memory leak was identified in `VideoThumbnailer::extractFrame`, specifically affecting video thumbnail generation.

### Details:
When the thumbnailer encountered a "black frame" (common at start of videos) or had to retry extraction:
1. It entered a retry loop (up to 3 times).
2. Inside the loop, it called `sws_getContext` to create a scaling context.
3. **CRITICAL ERROR**: It overwrote the `cleanup.swsCtx` pointer **without freeing the previous context**.
4. Result: Every retry leaked a full FFmpeg `SwsContext` structure (KB to MBs depending on resolution).

### The Fix:
Added explicit cleanup before re-allocation:
```cpp
if (cleanup.swsCtx) {
    sws_freeContext(cleanup.swsCtx);
    cleanup.swsCtx = nullptr;
}
cleanup.swsCtx = sws_getContext(...);
```

---

## Other Code Review Findings

### 1. Zombie Processes (Fixed)
**Issue**: Closing the UI window didn't terminate the background process.
**Fix**: Added `app.setQuitOnLastWindowClosed(true);` to `main_scrollbench.cpp` and `main.cpp`.

### 2. Stats Visibility (Fixed)
**Issue**: CPU/GPU/RAM stats were tracked but hidden.
**Fix**: Updated `PerformanceOverlay.qml` to display full system resource metrics.

### 3. Folder Scanning (Pending)
**Issue**: Upper-case extensions (JPG, MOV) are skipped.
**Recommendation**: Apply case-insensitive filter fix (next step).

### 4. Task Queue Safety (Verified)
**Result**: `TaskScheduler` and `AsyncImageProvider` correctly manage object lifecycles using shared pointers and atomic trackers. No task leaks detected.

---

## Verification
- **Build**: Passed
- **Safety**: FFmpeg cleanup structs now correctly handle all retry paths.
