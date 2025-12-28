# Samsung Gallery Clone - Feature Documentation

## Core Features

### 1. Image Gallery
- **Grid-based browsing** with customizable thumbnail sizes
- **Recursive directory scanning** for image discovery
- **Date-based grouping** (Today, Yesterday, Month Year)
- **Video file support** with D3D11 Hardware Accelerated thumbnails
- **Smooth scrolling** with optimized caching
- **Zoom functionality** via Ctrl+Wheel and pinch gestures

### 2. Image Viewer
- **Full-screen viewing** with navigation controls
- **Multi-touch zoom** (pinch to zoom, double-tap)
- **Keyboard navigation** (arrow keys, escape)
- **Image editing** with basic crop functionality
- **Performance monitoring** with load time tracking
- **Video Playback** with basic media controls in viewer

### 3. Performance Optimization
- **Asynchronous image loading** with thread pool
- **Memory-efficient caching** with configurable sizes
- **GPU acceleration** with multiple API support
- **Lazy loading** for smooth performance
- **Background processing** for file operations

### 4. Settings and Configuration
- **Graphics API selection** (Direct3D11, Vulkan, OpenGL, Software)
- **Performance tuning** (thumbnail size, cache size, thread count)
- **System monitoring** (CPU usage, memory usage)
- **Persistent settings** with automatic saving

### 5. User Interface
- **Tab-based navigation** (Pictures, Albums, Stories, Menu)
- **Dark theme** optimized for media viewing
- **Touch-friendly controls** for tablet/pen input
- **Keyboard shortcuts** for power users
- **Performance overlay** with real-time stats

## Technical Features

### 1. Async Image Processing
- **Thread pool management** for concurrent loading
- **Cancelable operations** to prevent memory leaks
- **Cache integration** for repeated access
- **Error handling** with fallback images

### 2. Broken File Overlays
- **Visual indicators** for unhandled or corrupted image/video files
- **Differentiates** between standard images, RAW, and video files
- **Enhances user feedback** for problematic media

### 3. Data Models
- **ImageModel** for file-based data management
- **AlbumModel** for folder-based organization
- **QAbstractListModel integration** for QML compatibility
- **Background data loading** with progress tracking

### 3. System Integration
- **Windows API integration** for system monitoring
- **File system access** with proper path handling
- **Memory management** with Qt's smart pointers
- **Cross-platform compatibility** (Windows-focused)

### 4. Performance Monitoring
- **Real-time FPS counter**
- **Load time tracking** with statistics
- **CPU usage monitoring** (Windows only)
- **Memory usage tracking** (Windows only)
- **Cache usage visualization**
- **GPU Load & VRAM usage** (via PDH/DXGI)

### 5. Build System
- **Unified Build Script**: `build.ps1` handles configuration, building, and cleaning.
- **Stale Build Prevention**: Automatic cleaning of `*_autogen` folders.
- **Dependency Management**: Automated checks for Qt modules and LibRaw.

## File Format Support

### Supported Image Formats
- **Standard**: JPEG, PNG, BMP, GIF, WEBP, TIFF
- **RAW Formats**: ARW, CR2, NEF, DNG, ORF, RW2 (via LibRaw)
- **High Efficiency**: HEIC/HEIF (requires system codecs)

### Supported Video Formats
- **Containers**: MP4, MKV, AVI, MOV, WEBM
- **Codecs**: H.264 (AVC), H.265 (HEVC), VP9, AV1, MPEG-2, MPEG-4
- **Acceleration**: Hardware-accelerated decoding via GPU (DXVA/D3D11)

## User Interface Components

### Main Window
- **1280x720 resolution** (can be resized)
- **Tab navigation** with bottom bar
- **Full-screen photo viewer** overlay
- **Settings menu** with performance options

### Gallery View
- **Dynamic grid sizing** (80px - 400px thumbnails)
- **Video placeholders** with play button
- **Error handling** for corrupted files
- **Loading indicators** for better UX

### Photo Viewer
- **Horizontal swipe navigation**
- **Zoom controls** (mouse wheel, pinch, keyboard)
- **Edit mode** with crop functionality
- **Navigation buttons** for previous/next

### Performance Overlay
- **GPU information** display (Usage, VRAM)
- **Load time statistics**
- **Cache usage monitoring**
- **System performance graphs**
- **Real-time performance sliders**

## Keyboard Shortcuts

### Navigation
- **Left/Right Arrow**: Navigate between images
- **Escape**: Exit photo viewer or cancel editing
- **Tab**: Toggle performance overlay visibility

### Zoom Controls
- **Ctrl + Mouse Wheel**: Zoom in/out in gallery
- **Plus/Equal Key**: Zoom in photo viewer
- **Minus Key**: Zoom out photo viewer

### Editing
- **Double Click**: Enter/exit zoom mode in photo viewer
- **Single Click**: Toggle UI visibility in photo viewer

## Performance Settings

### Adjustable Parameters
- **Thumbnail Size**: 80px - 400px (affects grid display)
- **Cache Size**: 64MB - 2048MB (memory usage)
- **Thread Count**: 1 - 16 (concurrent operations)
- **Graphics API**: Auto, Direct3D11, Vulkan, OpenGL, Software

### System Requirements
- **RAM**: Minimum 4GB, Recommended 8GB+
- **GPU**: DirectX 11 compatible or better (Dedicated GPU recommended for video/RAW)
- **Storage**: SSD recommended for better performance
- **OS**: Windows 10 or later

## Known Limitations

### 1. Album Feature
- Currently placeholder implementation
- No actual album organization
- No album management UI

### 2. Platform Support
- Windows-only implementation
- No macOS or Linux support

### 3. Image Editing
- Basic crop functionality only
- No advanced editing features

## 🎯 ScrollBench-Exclusive Features (Awaiting Port to Main App)

**ScrollBench** (`appScrollBench.exe`) is a feature-complete test application with advanced capabilities not yet available in the main app.

### Multi-Selection System
**Status:** ✅ Fully implemented in ScrollBench

-   **Single-click toggle** - Select/deselect individual images
-   **Shift+Click range** - Select all images between two points
-   **Drag-to-select** - Visual rectangle selection across grid
-   **Bulk operations** - Select all, clear selection, invert selection
-   **Visual feedback** - Selection border and count display

**Implementation:**
-   `test_scrollbench/src/ScrollBenchImageModel.cpp` - Complete selection API
-   Methods: `toggleSelection()`, `selectRange()`, `selectVisualRect()`, `selectAll()`, `clearSelection()`, `invertSelection()`

---

### Share & Resize Dialogs
**Status:** ⚠️ UI complete, backend TODO

-   **ShareDialog** - Three resize presets:
    -   Email (Small) - 1024x768, 80% quality
    -   Manual resize - Custom dimensions with live preview
    -   Original size - No modifications
-   **ResizeEditor** - Interactive resize interface:
    -   Live preview of resized image
    -   Quality/compression sliders
    -   Dimension controls with aspect ratio lock
    -   File size estimator

**Implementation:**
-   `test_scrollbench/qml/ShareDialog.qml` - Complete UI
-   `test_scrollbench/qml/ResizeEditor.qml` - Complete UI
-   Backend resize logic marked TODO (needs implementation)

---

### Advanced Performance Telemetry
**Status:** ✅ Fully implemented

-   **Frame budget tracking** - Monitors task completion within 16ms frame window
-   **Viewport culling metrics** - Reports visible range and buffer efficiency
-   **Real-time graphs** - FPS and frame timing visualization
-   **Note:** CPU/GPU/RAM metrics exist in backend but not displayed in UI

---

## 📹 Video Playback

**Status:** ✅ **Fully Implemented in Both Apps** (needs verification testing)

### Current Implementation

**Both Main App and ScrollBench include:**
-   MediaPlayer with hardware-accelerated decoding (D3D11VA via FFmpeg)
-   VideoOutput for QML integration
-   Full playback controls (play, pause, stop)
-   Timeline scrubbing with time display
-   Audio output with volume control
-   Automatic stop when switching images

**Files:**
-   `resources/qml/PhotoViewer.qml` (Main App, lines 232-390)
-   `test_scrollbench/qml/PhotoViewerScrollBench.qml` (ScrollBench, lines 264-426)

**Additional in ScrollBench:**
-   Debug logging for codec, resolution, bitrate
-   Playback state monitoring

**Verification Needed:**
-   Hardware acceleration confirmation
-   Codec support testing (H.264, H.265, AV1, VP9)
-   Audio sync validation
-   Rotation metadata handling

---

## 🔮 Planned Features (v2.3.0+)

### 1. Port ScrollBench Features to Main App
-   Multi-select system
-   Share & resize dialogs
-   Complete backend resize implementation

### 2. Image Editing Enhancements
-   Enhanced crop with draggable/resizable rectangle
-   90° rotation with EXIF preservation
-   Brightness/contrast adjustments
-   "Save As" option to preserve originals

### 3. Advanced Features
-   Drag & drop to Explorer/Email
-   Print support
-   Album search and filtering
-   GPU-accelerated DNG demosaicing
