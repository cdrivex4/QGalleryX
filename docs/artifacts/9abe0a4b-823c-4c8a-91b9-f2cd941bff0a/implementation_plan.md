# Goal Description
The user wants to identify two floating overlays in the UI (the Performance Stats widget in the top left, and the Diagnostics Overlay in the bottom left) and add toggles in the settings panel to turn them on and off.

These two overlays have been identified in `test_scrollbench/qml/MainScrollBench.qml`. We will expose two new boolean flags in the `SettingsHelper` C++ class and map them to switches in the `PerformanceOverlay.qml` settings drawer.

## Proposed Changes

### Configuration / Settings
#### [MODIFY] [SettingsHelper.h](file:///d:/Dev/antigravity/src/SettingsHelper.h)
- Add `Q_PROPERTY(bool showFloatingStats READ showFloatingStats WRITE setShowFloatingStats NOTIFY showFloatingStatsChanged)`
- Add `Q_PROPERTY(bool showDiagnosticsOverlay READ showDiagnosticsOverlay WRITE setShowDiagnosticsOverlay NOTIFY showDiagnosticsOverlayChanged)`
- Add getter/setter declarations and signals for both properties.

#### [MODIFY] [SettingsHelper.cpp](file:///d:/Dev/antigravity/src/SettingsHelper.cpp)
- Implement `showFloatingStats()` and `setShowFloatingStats()` (default `true`).
- Implement `showDiagnosticsOverlay()` and `setShowDiagnosticsOverlay()` (default `true`).
- Ensure settings are saved/loaded to the `QSettings` backend.

### UI / QML
#### [MODIFY] [MainScrollBench.qml](file:///d:/Dev/antigravity/test_scrollbench/qml/MainScrollBench.qml)
- Floating Performance Stats: Update `visible: !root.viewerVisible && !overlayVisible` to `visible: settings.showFloatingStats && !root.viewerVisible && !overlayVisible`.
- DiagnosticsOverlay: Update `visible: true` to `visible: settings.showDiagnosticsOverlay`.

#### [MODIFY] [PerformanceOverlay.qml](file:///d:/Dev/antigravity/test_scrollbench/qml/PerformanceOverlay.qml)
- Add a new "UI Options" section in the settings tab (Page 2).
- Add a `RowLayout` + `Switch` for "Show Floating Performance Stats" bound to `settings.showFloatingStats`.
- Add a `RowLayout` + `Switch` for "Show Diagnostics Overlay" bound to `settings.showDiagnosticsOverlay`.

## Verification Plan
### Automated Tests
Run `./build.ps1` to compile the app and ensure no syntax or linking errors occur.
### Manual Verification
Run the app via `./build.ps1` (or the debug runner).
1. Open the settings drawer (gear icon at the bottom).
2. Go to the "Settings" tab.
3. Toggle "Show Floating Performance Stats" and observe the top-left overlay disappears/reappears.
4. Toggle "Show Diagnostics Overlay" and observe the bottom-left overlay disappears/reappears.
