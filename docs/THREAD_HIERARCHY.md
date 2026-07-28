# Application Thread Hierarchy

This document outlines the threading model of the QGalleryX application to help diagnose performance bottlenecks and understand execution flow.

## 1. Main Thread (GUI Thread)
*   **ID**: The initial thread of the process.
*   **Responsibilities**:
    *   **Event Loop**: Runs `QCoreApplication::exec()`. Handles all OS events (mouse, keyboard, window system).
    *   **QML Engine**: Executes JavaScript logic in QML, property bindings, and signal handlers.
    *   **Models**: `ImageModel`, `AlbumModel`, and `SystemMonitor` live here.
        *   *Note*: `ImageModel::scanDirectory` offloads work, but the *initiation* happens here.
        *   *Note*: `SystemMonitor::updateStats` runs here via `QTimer`. **Warning**: PDH queries run here and can block if slow.
    *   **File I/O (Blocking)**: Any `QDir::exists()` or `QSettings` access in constructors happens here. This is the likely cause of the "default folder" hang.

## 2. Render Thread (Qt Quick Scene Graph)
*   **ID**: Managed internally by Qt.
*   **Responsibilities**:
    *   Takes the state from the Main Thread and draws it using the selected Graphics API (Direct3D, Vulkan, OpenGL).
    *   **Sync**: Blocks the Main Thread briefly to synchronize state before rendering a frame.

## 3. Worker Threads (QThreadPool)
*   **ID**: Pool of threads managed by `QThreadPool::globalInstance()`.
*   **Responsibilities**:
    *   **Image Loading**: `AsyncImageProvider` creates `ImageLoaderRunnable` tasks.
        *   **Priority**: Full-size images (PhotoViewer) have `INT_MAX` priority. Thumbnails have `1` priority (FIFO).
    *   **Directory Scanning**: `ImageModel::scanDirectory` uses `QtConcurrent::run` to list files in the background.

## 4. Potential Bottlenecks
*   **Main Thread Blocking**:
    *   `ImageModel` constructor checks `QDir::exists()` on the default "Pictures" location. If this is a network drive, it will freeze the app startup.
    *   `SystemMonitor` queries PDH (Performance Counters) on the main thread every 1 second.
*   **Disk I/O**:
    *   Multiple worker threads competing for disk access (loading thumbnails) can saturate I/O, slowing down the Main Thread if it tries to read settings or check paths.

## 5. Logging Strategy
*   **Atomic Logging**: We have installed a custom message handler.
*   **Format**: `[YYYY-MM-DD HH:mm:ss.zzz] [Thread ID] Level: Message`
*   **Usage**: Run the app from a terminal and redirect output to a file to trace execution order and timing.
    *   `.\build\QGalleryX.exe > log.txt 2>&1`
