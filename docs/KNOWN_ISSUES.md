# Known Issues & Workarounds

**Last Updated:** February 10, 2026

---

## 🔴 Current Issues

### 1. Selection/Share Features Only in ScrollBench
- **Issue**: Multi-select and share/resize dialogs are fully implemented in ScrollBench but not yet ported to main application
- **Impact**: Users must use ScrollBench to select multiple images or resize for sharing
- **Workaround**: Use `appScrollBench.exe` for these features
- **Status**: Planned for next release (v2.3.0)
- **Files**: 
  - `test_scrollbench/src/ScrollBenchImageModel.cpp` (selection API)
  - `test_scrollbench/qml/ShareDialog.qml` (share UI)
  - `test_scrollbench/qml/ResizeEditor.qml` (resize UI)

---

### 2. SystemMonitor Metrics Not Displayed in ScrollBench
- **Issue**: Performance overlay in ScrollBench shows FPS and frame timing but does not display CPU/GPU/RAM usage
- **Root Cause**: `PerformanceOverlay.qml` not wired to SystemMonitor context property
- **Impact**: Cannot monitor system resources in ScrollBench (works fine in main app)
- **Workaround**: Use main app's StatsOverlay (Tab key) for full system metrics
- **Status**: Easy fix, low priority
- **Fix**: See implementation code in `docs/SYSTEM_MONITORING_STATUS.md`

---## 🔴 High Priority

### Concurrency Leak & Stall Recovery (Feb 10, 2026)
**Status:** Identified, pending implementation  
**Issue:** `AsyncImageProvider` has a double-increment bug in `activeWeight` and lacks a timer to trigger `checkStalls()`.  
**Impact:** UI may freeze or starve when loading from slow/stalled network shares.  
**Files:** `src/AsyncImageProvider.cpp`

### DNG Proprietary Compression Support ⏸️ DEFERRED
- **Issue**: DNG files with proprietary compression require long load times.
- **Status**: Partially mitigated in v2.2.1 via Task Weighting (prevents system saturation).
- **Files**: `src/AsyncImageProvider.cpp`

---

### 4. Case-Sensitive File Extension Matching
- **Issue**: Files with uppercase extensions (`.JPG`, `.PNG`, `.MP4`) are not detected during directory scanning
- **Root Cause**: QDirIterator filters use lowercase patterns only (`*.jpg`, `*.png`, `*.mp4`)
- **Impact**: Camera files saved with uppercase extensions are invisible to the gallery
- **Workaround**: Manually add uppercase variants to filter lists
- **Status**: Documented fix available
- **Fix**: See complete implementation in `docs/FOLDER_SCANNING_DIAGNOSTIC.md`
- **Files**: `src/ImageModel.cpp` (line 203), `test_scrollbench/src/ScrollBenchImageModel.cpp` (line 256)

---

### 7. AsyncImageProvider Concurrency Leak
- **Issue**: `activeWeight` is incremented twice per task (admission + guard constructor)
- **Impact**: Concurrency slots are consumed 2x faster than intended, causing starvation
- **Status**: Identified, fix planned for next session
- **File**: `src/AsyncImageProvider.cpp`

### 8. Stall Recovery Timer Missing
- **Issue**: `checkStalls()` is implemented but never called by any timer
- **Impact**: Stalled I/O tasks (e.g., on offline network shares) never recover their weight
- **Status**: Identified, fix planned for next session
- **File**: `src/AsyncImageProvider.cpp`

---

## ✅ Previously Reported Issues (Now Resolved)

### Fixed in v2.2.1 (Feb 10, 2026)
- ✅ **1 FPS UI Slowdown** → RESOLVED: Replaced O(N) loops with O(1) counters in `ScrollBenchImageModel`
- ✅ **Viewport Culling Broken** → RESOLVED: Fixed path normalization in `VisibleRangeManager`
- ✅ **Build Failures** → RESOLVED: Implemented process killing and hash verification in `build.ps1`
- ✅ **CPU Overload** → RESOLVED: Added task weighting and CPU-aware backoff throttling

### Fixed in v2.2.0 (MFT Scanning & Frame Budget Release)
- ✅ **Missing Static Qt Build** → RESOLVED: `scripts/setup_static_qt.ps1` implemented and working
- ✅ **MinGW LTO Issues** → RESOLVED: Build system properly configured, LTO working
- ✅ **QML Caching Conflicts** → RESOLVED: Fixed in CMakeLists.txt with `CACHE_GEN OFF` for single_exe

### Fixed in v2.1.0 (Network & Deployment Release)
- ✅ **UNC Path Support** → Fixed with `QUrl::toLocalFile()` for `\\\\Server\\Share` paths
- ✅ **Missing DLL Errors** → Fixed by including MinGW runtime libraries in deploy script
- ✅ **UI Freezes on Network Scans** → Fixed by removing aggressive memory limits

---

## 🔗 Related Documentation

- **Feature Status:** [FEATURES.md](FEATURES.md)
- **Folder Scanning Fix:** [FOLDER_SCANNING_DIAGNOSTIC.md](FOLDER_SCANNING_DIAGNOSTIC.md) 
- **System Monitoring Fix:** [SYSTEM_MONITORING_STATUS.md](SYSTEM_MONITORING_STATUS.md)
- **Outstanding Tasks:** [resume/OUTSTANDING_TASKS.md](resume/OUTSTANDING_TASKS.md)
- **Format Compatibility:** [FORMAT_COMPATIBILITY.md](FORMAT_COMPATIBILITY.md)

---

**Note:** This document reflects actual current issues and known workarounds. Historical issues that have been resolved are documented in the "Previously Reported" section for reference.
