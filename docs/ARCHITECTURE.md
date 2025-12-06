# Architecture & Dependency Map

This document outlines the key components of the application and their relationships.

## Core Components (C++)

### 1. `ImageModel`
-   **Role**: Scans directories for images/videos and provides a list model for QML.
-   **Dependencies**: `QtConcurrent` (for async scanning), `QImageReader`.
-   **Used By**: `GalleryViewTiles.qml`, `GalleryViewSemantic.qml`.
-   **Key Methods**: `scanDirectory(path)`, `cropImage`.

### 2. `AlbumModel`
-   **Role**: Scans directories for sub-folders (albums) and provides a list model.
-   **Dependencies**: `QtConcurrent`.
-   **Used By**: `AlbumsView.qml`.
-   **Key Methods**: `scanAlbums(path)`.

### 3. `AsyncImageProvider`
-   **Role**: Loads images asynchronously for QML `Image` components.
-   **Dependencies**: `TaskScheduler`, `VideoThumbnailer`, `LibRaw`, `QImageReader`.
-   **Used By**: All QML `Image` components using `image://async/` scheme.
-   **Key Features**: Caching (LRU), Prioritization (Viewer > Grid), D3D11 Video Decodes, RAW Support.

### 4. `TaskScheduler`
-   **Role**: Manages separate thread pools for CPU-bound (decoding) and IO-bound (scanning) tasks.
-   **Features**: Priority Queues, Dynamic Thread Scaling (Sleep on idle).

### 5. `VideoThumbnailer`
-   **Role**: D3D11 Hardware-accelerated video thumbnail generation.
-   **Dependencies**: `FFmpeg` (avcodec, avformat, swscale, d3d11va), `Direct3D11`.
-   **Features**: Black Frame Detection, Smart Retry, Software Fallback.

### 6. `SystemMonitor`
-   **Role**: Monitors CPU, RAM, and GPU usage.
-   **Dependencies**: `PDH` (Windows), `DXGI` (Windows).
-   **Used By**: `StatsOverlay.qml`.

### 7. `SettingsHelper`
-   **Role**: Persists application settings (Last Folder, Graphics API).
-   **Dependencies**: `QSettings`.
-   **Used By**: `Main.qml`.

## UI Components (QML)

### 1. `Main.qml` (Root)
-   **Role**: Main entry point, layout management.
-   **Children**:
    -   `BottomBar`: Navigation.
    -   `StackLayout`: Switches between Pictures, Albums, Stories.
    -   `PhotoViewer`: Full-screen image viewer.
    -   `StatsOverlay`: Performance stats.
-   **Context**: Instantiates `AlbumModel` and `SettingsHelper`.

### 2. `GalleryViewTiles.qml`
-   **Role**: Grid view of images.
-   **Model**: Has its own internal `ImageModel`.
-   **Input**: `folderPath` property triggers scanning.

### 3. `GalleryViewSemantic.qml`
-   **Role**: Semantic zoom view (grouped by date).
-   **Model**: Uses `GroupedProxyModel` (C++) wrapping `ImageModel`.

### 4. `AlbumsView.qml`
-   **Role**: Grid view of albums (folders).
-   **Model**: Uses `AlbumModel` (passed from `Main.qml`).
-   **Navigation**: Uses `StackView` to navigate to `AlbumDetail` (which reuses `GalleryViewTiles`).

## Data Flow

1.  **Startup**:
    -   `Main.qml` loads.
    -   `SettingsHelper` provides `lastFolder`.
    -   `Main.qml` sets `currentPath`.
    -   `AlbumModel` scans `currentPath`.
    -   `GalleryViewTiles` (in Pictures tab) scans `currentPath`.

2.  **Folder Change**:
    -   User selects folder in `FolderDialog`.
    -   `Main.qml` updates `currentPath`.
    -   `AlbumModel` rescans.
    -   `GalleryViewTiles` rescans.

3.  **Album Navigation**:
    -   User clicks Album in `AlbumsView`.
    -   `AlbumsView` pushes `AlbumDetail`.
    -   `AlbumDetail` sets `folderPath` on its internal `GalleryViewTiles`.
    -   Internal `GalleryViewTiles` scans the album path.

## Key Interactions

-   **Image Loading**: QML `Image` -> `AsyncImageProvider` -> Worker Thread -> QML.
-   **Scanning**: `ImageModel::scanDirectory` -> `QtConcurrent::run` -> `m_images` update -> QML update.
-   **Cancellation**: `ImageModel` clears immediately on scan start. `AsyncImageProvider` cancels pending requests if `Image` is destroyed.