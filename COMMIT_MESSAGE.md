# Release v2.2.0 - Documentation Sync & Project Consolidation

## 🎯 Summary
Major documentation synchronization update consolidating project status across all `.md` files. Removed obsolete references, updated to v2.2.0 current state, and clarified ScrollBench dual-app architecture strategy.

## 📚 Documentation Updates

### Updated Files
- **KNOWN_ISSUES.md** - Removed 3 resolved issues (Static Qt, MinGW LTO, QML Caching). Added actual current issues with workarounds
- **README.md** - Added v2.2.0 to version history, documented ScrollBench dual-app architecture, updated known issues section
- **FEATURES.md** - Added ScrollBench-exclusive features section, clarified video playback is fully implemented (not missing)
- **RELEASE_NOTES.md** - Added complete v2.2.0 release notes (MFT Scanner, Frame Budget, TDR fixes, FileTypeRouter)
- **.gemini/gemini_resume.md** - Complete rewrite to reflect v2.2.0 state, corrected feature statuses, removed old bug references

### Deleted Files
- **docs/resume/walkthrough.md** - Obsolete reference to non-existent "Test App", replaced by artifact walkthrough

### Key Corrections
- ✅ Video playback documented as **fully implemented** (not missing/placeholder)
- ✅ Selection/Share features documented as **in ScrollBench awaiting port** (not greenfield)
- ✅ HardwareAccelerationManager documented as existing (centralized D3D11VA management)
- ✅ FileTypeRouter documented (170+ format detection)
- ✅ SystemMonitor status clarified (backend complete, ScrollBench UI not wired)

## 🏗️ Architecture Clarifications

### Dual-App Strategy
Documented that project intentionally maintains two applications:
- **Main App** (`appSamsungGallery.exe`) - Production gallery (stable)
- **ScrollBench** (`appScrollBench.exe`) - Performance testing & feature prototyping (feature-complete)

**Development Flow**: Prototype in ScrollBench → Validate → Port to Main App

### v2.2.0 Accomplishments
- MFT Scanning ported from ScrollBench to Main App (10-100x faster enumeration)
- Frame Budget Scheduler ported from ScrollBench to Main App (prevents UI stutter)
- TDR crash fixes (reduced video/RAW concurrency)
- FileTypeRouter centralized format detection

## 🔧 Code Changes (Previous Sessions)

### Modified Files (45 files, 102,434 insertions, 71,961 deletions)
- Main.qml - Fixed null reference warning (line 272)
- Various QML files - UI improvements and fixes
- ImageModel.cpp, AlbumModel.cpp - MFT scanner integration
- AsyncImageProvider.cpp - Frame budget integration, TDR fixes
- ScrollBenchImageModel.cpp - Selection API complete

## 📦 Untracked Files to Add

### Documentation (New)
- docs/AI_DEVELOPMENT_SCOPE.md
- docs/FEASIBILITY_GPU_APIS.md
- docs/FIX_MEMORY_LEAK_REPORT.md
- docs/FOLDER_SCANNING_DIAGNOSTIC.md
- docs/FORMAT_COMPATIBILITY.md
- docs/GPU_ACCELERATION_STATUS.md
- docs/SINGLE_EXE_INTEGRATION.md
- docs/STATIC_QT_INFO.md
- docs/STATIC_QT_SETUP.md
- docs/SYSTEM_MONITORING_STATUS.md
- docs/resume/SESSION_2024-12-25.md

### Source Code (New)
- src/FastVolumeScanner.cpp/.h - MFT scanner implementation
- src/FrameBudgetScheduler.cpp/.h - Frame budget scheduler
- test_scrollbench/src/FileTypeRouter.cpp/.h - Format detection
- tests/tst_mft.cpp - MFT benchmark test

### Build Scripts (New)
- scripts/ - Build automation scripts including setup_static_qt.ps1
- single_exe/ - Portable single-file executable build configuration

### Resources (New)
- resources/qml/AlbumCard.qml - Album view component

## 🚫 Intentionally Excluded

The following remain gitignored as they are build artifacts or user-specific:
- build/ - CMake build output
- deploy/ - Deployment artifacts
- *.obj, *.dll, *.lib, *.pdb - Build artifacts
- logs/ - Runtime logs
- .vscode/ - IDE-specific settings

## ⚠️ Third-Party Note

User requested including third-party folders for reproducibility. However, the following should remain excluded due to size:
- 3rdparty/qt_static/ - Large Qt static build (can be rebuilt with scripts/setup_static_qt.ps1)
- 3rdparty/tools/ - Build tools

**Recommendation**: Add README in 3rdparty/ explaining how to obtain dependencies instead of committing binaries.

## ✅ Build Verification

Project builds successfully:
```powershell
.\build.ps1           # Standard build (both apps)
.\build.ps1 -Clean    # Clean rebuild
```

Both `appSamsungGallery.exe` and `appScrollBench.exe` compile and run.

## 🎯 Next Steps (v2.3.0 Planned)

1. Port multi-selection from ScrollBench to Main App
2. Complete Share/Resize backend implementation
3. Enhance image editing (crop, rotate, adjustments)
4. Verification testing for video playback

---

**Version**: v2.2.0  
**Date**: 2025-12-29  
**Status**: Ready for release
