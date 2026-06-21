# Hyper-Granular Migration Plan: QGalleryX (SAFE PHASED APPROACH)

This document provides a comprehensive roadmap for safely renaming the project from "antigravity" and "Samsung Gallery Clone" to its new official name: **QGalleryX**.

> **CRITICAL LESSON LEARNED**: We must **NEVER** use global find-and-replace scripts (like PowerShell `-replace` over all files) to blindly rename paths like `src` to `src_QGalleryX_Core`. Doing so destroys relative C++ `#include` directives (e.g. `../../src/`) and breaks the build. We must also **always** use `git mv` instead of OS-level folder renames to preserve Git history.

## Phase 1: Target Renames (Executables Only)
The goal is to safely rename the output executables without touching any folders or C++ variables.
1. **`CMakeLists.txt` (Main)**
   - Update `project(SamsungGallery)` -> `project(QGalleryX)`
   - Update `qt_add_executable(appSamsungGallery)` -> `qt_add_executable(QGalleryX)`
   - Update URI from `SamsungGallery` to `QGalleryX`
2. **`test_scrollbench/CMakeLists.txt`**
   - Update `project(ScrollBench)` -> `project(QGalleryX-Bench)`
   - Update `add_executable(appScrollBench)` -> `add_executable(QGalleryX-Bench)`
3. **`test_scrollbench/src/main_scrollbench.cpp`**
   - Update the UI crash title strings only.
4. **`build.ps1` and `deploy.ps1`**
   - Safely update hardcoded executable names expected by `windeployqt` and `taskkill` (`appSamsungGallery.exe` and `appScrollBench.exe`) to match the new targets.
5. **VERIFICATION**: Run `.\build.ps1` and verify the new `QGalleryX.exe` works.

## Phase 2: Folder Renames (Preserving Git History)
1. Use `git mv src src_QGalleryX_Core`
2. Use `git mv test_scrollbench test_QGalleryX-Bench`
3. Use `git mv single_exe single_QGalleryX_Standalone`
4. Use `git mv resources resources_QGalleryX`
5. Carefully manual-update `CMakeLists.txt` `add_subdirectory()` paths.
6. Use targeted code editing to update C++ `#include` paths.
7. **VERIFICATION**: Run `.\build.ps1` and verify.

## Phase 3: Internal String and C++ Variable Updates
1. Rename `ScrollBenchImageModel` -> `QGalleryXBenchImageModel` directly via code edits in header/cpp files and QML types.
2. Update QML imports (`import SamsungGallery 1.0` -> `import QGalleryX 1.0`).
3. Update specific strings (`"Samsung Gallery"` -> `"QGalleryX"`) in UI files.
4. **VERIFICATION**: Run `.\build.ps1`.

## Reversion Strategy (Rollback)
In case of a failure:
1.  **Immediate Git Reset**: `git reset --hard HEAD` and `git clean -fd`
2.  **Verify Status**: Run `.\build.ps1` to prove the baseline works before attempting changes again.
