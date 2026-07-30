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

### 3. Folder Scanning (Fixed)
**Issue**: Upper-case extensions (JPG, MOV) were skipped.
**Fix**: Implemented case-insensitive filters in `FastVolumeScanner` and `ImageModel`.

### 4. Task Queue Safety & Admission Deadlocks (Fixed)
**Issue**: When scrolling rapidly over non-existent thumbnails on slow/network drives, `QFile::exists` would block the IO threads. Worse, if a task returned early before properly decrementing `activeWeight` in the Admission Control queue, the UI would permanently deadlock.
**Fix**: 
- Added a `DriveConcurrencyGuard` RAII struct in `AsyncImageProvider` to guarantee `activeWeight` is decremented on all return paths.
- Added an `O(1)` memory index (`m_knownKeys`) in `FileCacheManager` to reject non-existent cache keys instantly, completely bypassing the disk IO hit storm.

---

## Verification
- **Build**: Passed
- **Safety**: FFmpeg cleanup structs now correctly handle all retry paths.
