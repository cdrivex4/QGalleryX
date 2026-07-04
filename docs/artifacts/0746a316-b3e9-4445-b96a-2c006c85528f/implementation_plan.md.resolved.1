# Open in Explorer Feature Plan

## Goal Description
Allow users to open the file explorer and highlight the current media file directly from the "Info" overlay in the Photo Viewer.

## Proposed Changes

### C++ Backend
#### [NEW] [DesktopHelper.h](file:///d:/Dev/antigravity/src/DesktopHelper.h)
#### [NEW] [DesktopHelper.cpp](file:///d:/Dev/antigravity/src/DesktopHelper.cpp)
- Create a new class `DesktopHelper` inheriting from `QObject`.
- Implement `Q_INVOKABLE void openInExplorer(const QString &path)`.
- Use Windows-specific command `explorer.exe /select,"<path>"` to open and highlight the file.

#### [MODIFY] [main.cpp](file:///d:/Dev/antigravity/src/main.cpp)
- Include `DesktopHelper.h`.
- Register `DesktopHelper` type (optional).
- Instantiate `DesktopHelper` and expose it as a context property `desktopHelper`.

#### [MODIFY] [CMakeLists.txt](file:///d:/Dev/antigravity/CMakeLists.txt)
- Add `src/DesktopHelper.cpp` and `src/DesktopHelper.h` to `appSamsungGallery` and `appSamsungGalleryTest` targets.

### QML Frontend
#### [MODIFY] [PhotoViewer.qml](file:///d:/Dev/antigravity/resources/qml/PhotoViewer.qml)
- In the `infoOverlay` delegate:
    - Check if the key is "Path".
    - If so, display a "Folder" icon button next to the value.
    - On click, call `desktopHelper.openInExplorer(value)`.

## Verification Plan

### Manual Verification
- Build and run the application.
- Open a photo/video.
- Click the "Info" button.
- Verify the "Path" row has a folder icon.
- Click the folder icon.
- Verify Windows Explorer opens and the correct file is selected.
