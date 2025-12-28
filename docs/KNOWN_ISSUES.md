# Known Issues & Workarounds

**Last Updated:** December 28, 2024

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

---

### 3. DNG Proprietary Compression Performance
- **Issue**: DNG files with proprietary compression (`PhotometricInterpretation=32803`) require 120+ seconds to load
- **Root Cause**: LibRaw performs CPU-only Bayer demosaicing with no GPU acceleration
- **Impact**: Severe performance degradation for certain manufacturer-specific DNG files
- **Workaround**: 
  - Use embedded JPEG preview extraction (faster but lower quality)
  - Use corresponding JPG file if available
  - Avoid opening these files in full-resolution mode
- **Status**: Deferred (requires custom GPU-accelerated demosaicing shaders)
- **Files**: `src/AsyncImageProvider.cpp` (lines 378-415)

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

## 🟡 Minor Issues

### 5. Hardcoded Test Paths in Code
- **Issue**: Some files reference non-existent test paths (e.g., `I:/`)
- **Impact**: Warnings in debug logs, no functional impact
- **Status**: Cleanup task tracked in task checklist

---

### 6. Excessive Crash Log Files
- **Issue**: 28+ crash log files accumulated in project root directory
- **Impact**: Repository clutter
- **Recommended:** Move to `logs/` subdirectory or delete historical logs

---

## ✅ Previously Reported Issues (Now Resolved)

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
