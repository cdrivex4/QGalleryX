# Overlay Settings Implementation

The two floating overlays in `test_scrollbench/qml/MainScrollBench.qml` (Floating Performance Stats and Diagnostics Overlay) have been linked to new settings toggles.

## Changes Made
- **C++ Backend**: Added `showFloatingStats` and `showDiagnosticsOverlay` boolean properties to `SettingsHelper.cpp`.
- **Settings UI**: Added a new "UI Options" section inside the "Settings" tab in `test_scrollbench/qml/PerformanceOverlay.qml` containing switches for the two new properties.
- **Main View Bindings**: Bound the `visible` properties of the `Floating Performance Stats` rectangle and the `DiagnosticsOverlay` component in `test_scrollbench/qml/MainScrollBench.qml` to respect these new settings.

## Validation Results
- The application compiled successfully using the `.\build.ps1` script.
- The `tst_linkage.exe` backend verification passed.

## Manual Verification
You can now run `appScrollBench.exe` (or use your debug run script).
1. Click the **gear icon** ⚙️ at the bottom to open the Settings Overlay.
2. Navigate to the **"Settings"** tab.
3. Scroll down (or look above "Advanced Optimizations") for the **"UI Options"** section.
4. Toggle **"Show Floating Performance Stats"** to control the top-left stats view.
5. Toggle **"Show Diagnostics Overlay"** to control the bottom-left diagnostics view.

---

# Main App Fix (appSamsungGallery.exe)
The main application was failing to launch because several QML components (`WatermarkOverlay.qml`, `BuildInfo.qml`, `MainSemantic.qml`) were missing from the `CMakeLists.txt` build configuration.

## Changes Made
- Updated `CMakeLists.txt` to include the missing `.qml` files in the `appSamsungGallery` QML module.
- Rebuilt the project.

## Validation
- Ran `build/appSamsungGallery.exe` and confirmed it launches successfully and starts scanning the filesystem.

