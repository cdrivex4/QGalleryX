# Build System Improvements Walkthrough

## Completed Work
We have significantly improved the robustness and reliability of the build system.

### 1. Build Script Enhancements (`build.ps1`)
- **Stale Build Prevention**: Added a `-Clean` switch to allow a full clean build (removing the entire `build` directory).
- **Automatic Autogen Cleaning**: The script now automatically removes `*_autogen` folders on every build to ensure QML and MOC changes are picked up.
- **Process Management**: Improved the logic to kill running instances (`appSamsungGallery.exe`, `appSamsungGalleryTest.exe`) using `Stop-Process` with error handling.
- **Error Handling**: Added explicit checks for CMake configuration and build steps. If any step fails, the script now exits immediately with a clear error message.

### 2. Dependency Verification
- **LibRaw**: Verified that the `LibRaw` dependency is correctly configured in `CMakeLists.txt` and that the library files exist in `3rdparty/LibRaw`.
- **File References**: Confirmed that all source and QML files referenced in `CMakeLists.txt` exist on disk.

### 3. Verification
- **Clean Build**: Successfully ran `.\build.ps1 -Clean` to verify the full build process from scratch.
- **Incremental Build**: Verified that subsequent builds work correctly.

### 4. Open in Explorer Feature
- **Implementation**: Created `DesktopHelper` class to interface with Windows Explorer via `QProcess`.
- **UI Integration**: Added a folder icon button to the "Info" overlay in `PhotoViewer.qml`.
- **Functionality**: Clicking the button opens Windows Explorer with the current media file selected and highlighted.

### 5. Bug Fixes
- **Metadata Keys**: Standardized metadata keys to Capitalized Case (e.g., "Path", "Filename") in `ImageModel.cpp` to match QML expectations.
- **Info Overlay**: Added a `MouseArea` to the info dialog to prevent accidental closing when clicking inside the dialog.
- **Explorer Arguments**: Updated `DesktopHelper.cpp` to use `QProcess::setNativeArguments` to correctly pass the `/select,` argument to `explorer.exe` without incorrect auto-quoting.

## Next Steps
- Implementing the "Open in Explorer" feature in the Photo Viewer.
