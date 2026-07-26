# Video Playback Architecture Research Handoff

## Application Context
**Application**: Antigravity (C++/Qt6 QML Photo and Video Gallery)
**Target Platforms**: Currently Windows (using MinGW 64-bit, Qt 6.9.3)
**Primary Goal**: High-performance, low-latency media browsing, supporting both massive local drives and high-latency network shares.

## Current Video Architecture & Wiring
Currently, the application manages video files across two distinct layers:

1. **Scanner & Model Layer (`d:\Dev\antigravity\src_legacy\ImageModel.cpp`)**:
   - Videos are identified during the `scanDirectory` method using `QDirIterator` name filters, explicitly capturing formats like `*.mp4`, `*.mkv`, `*.avi`, `*.mov`, `*.webm`, etc.
   - These are passed down as generic `ImageInfo` structs, allowing the grid to populate instantly.

2. **Background Thumbnail Generation (`d:\Dev\antigravity\src_legacy\VideoThumbnailer.cpp`)**:
   - Triggered by `AsyncImageProvider`, this class relies directly on the native **FFmpeg API** (`avformat`, `avcodec`, `swscale`).
   - It hooks into the bundled DLLs (e.g., `avcodec-62.dll`, `avformat-62.dll`) to seek into the video stream and extract a raw frame as a `QImage`, which is then cached by the `FileCacheManager`.

3. **Video Playback UI (`d:\Dev\antigravity\resources\qml_legacy\PhotoViewer.qml`)**:
   - The QML layer parses the extension: `property bool isVideo: fileExt === "mp4" || fileExt === "mkv" || fileExt === "avi" || fileExt === "mov"`.
   - If `isVideo` is true, the viewer hides the standard `Image` component and instead activates a `MediaPlayer` + `VideoOutput` component.
   - It also exposes playback controls (play/pause overlay, scrubber) and triggers `root.model.pauseBackgroundTasks()` when playing to prevent the I/O scanner from starving the video decoder.

## Research Objectives for the Next Agent
We need a deep-dive analysis on the pros, cons, and feasibility of swapping our current Qt `MediaPlayer` playback mechanism with either **libVLC** (VLC's backend) or a custom rendering pipeline using raw **FFmpeg**. 

Please research and address the following questions:

### 1. Qt MediaPlayer vs. LibVLC vs. FFmpeg
- **Qt MediaPlayer**: Since Qt6 recently transitioned heavily toward utilizing an FFmpeg-backed multimedia module, is the `MediaPlayer` component just a wrapper over FFmpeg anyway? What limitations exist in Qt's QML `MediaPlayer` regarding obscure formats, hardware acceleration, or granular playback control?
- **LibVLC (VLC)**: VLC is known for "playing everything." How difficult is it to integrate `libvlc` (or `VLC-Qt`) into a modern Qt6/QML application? What is the licensing overhead (LGPL vs GPL) for bundling VLC plugins? Would it improve format support compared to our current implementation?
- **Raw FFmpeg**: We already bundle FFmpeg to extract thumbnails. If we were to build our own video decoder using our existing FFmpeg binaries and render the frames to a Qt `QQuickImageProvider` or custom OpenGL/Vulkan texture, what is the performance delta? How difficult is it to maintain A/V sync manually compared to using a pre-built player?

### 2. Specific Antigravity Considerations
- **Performance**: We recently implemented a Two-Pass network scanning system with heavy background I/O. The playback engine must be resilient and not stutter heavily when disk I/O is saturated.
- **Dependency Bloat**: We already ship FFmpeg DLLs for the thumbnailer. If we move to LibVLC, we'd have to ship the VLC core and its massive plugin folder. Is the extra size (often 100MB+) worth the benefit?
- **QML Integration**: We need an engine that effortlessly binds into a QML scene graph. The current `VideoOutput` is hardware-accelerated. If we switch, how do we guarantee zero-copy hardware decoding directly to the GPU in QML?

### Deliverable
Please provide a recommendation on whether Antigravity should:
A. Stick with QML `MediaPlayer` (and ensure Qt is utilizing its FFmpeg backend).
B. Migrate to a `libvlc` wrapper for maximum compatibility.
C. Build a custom QML video renderer using the FFmpeg libraries already present in the codebase.
