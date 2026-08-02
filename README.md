# QGalleryX

A modern Qt-based photo gallery application that replicates the functionality and user interface of Samsung's native gallery application. Built with C++ backend and QML frontend for optimal performance and user experience.

In English - imagine a gallery for photos and videos on a Windows tablet or PC. It looks like Samsung's gallery app and I have gone through great pains to make it as fast and possible on low end hardware and scale to more powerful beasts. Point it to a folder of 20,000+ images and videos and it should show you everything, instantly. You can scroll through thousands of images and videos with no stutter, no lag, and no slowdowns. Pinch to zoom, rotate, pan, view full screen, copy to any folder that one/group of files so you can have , email, vie by albums, and whatever else you expect from a modern photo gallery app. It has a smart video thumbnailer that will find the best frame in a video to use as a thumbnail. It supports RAW files, HEIC/HEIF, videos, GIFs, and more. Infact if you have the HEIC formats the decode for that has been tweaked to use FFMPEG and GPU accleration. 

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Qt](https://img.shields.io/badge/Qt-6.4+-green.svg)
![Platform](https://img.shields.io/badge/Platform-Windows-orange.svg)

## 📋 Table of Contents

- [Project Overview](#project-overview)
- [Key Features](#key-features)
- [Quick Start](#quick-start)
- [Build Instructions](#build-instructions)
- [Architecture](#architecture)
- [Documentation](#documentation)
- [Requirements](#requirements)
- [Known Issues](#known-issues)
- [Handover & Future Work](#-handover--future-work)
- [Contributing](#contributing)

## 🎯 Project Overview

QGalleryX is a feature-rich photo gallery application designed to provide a smooth, responsive experience for browsing and viewing photos and videos. The application leverages Qt 6's modern capabilities to deliver:

- **High-performance image loading** with asynchronous processing
- **Touch-friendly interface** optimized for tablets and touch devices
- **Advanced graphics support** with Direct3D11, Vulkan, and OpenGL
- **Real-time performance monitoring** and optimization
- **Customizable settings** for performance tuning

## ✨ Key Features

### Core Functionality
- **Grid-based browsing** with dynamic thumbnail sizing
- **Full-screen photo viewer** with zoom and navigation
- **Video file support** (placeholder implementation)
- **Date-based organization** with smart grouping
- **Performance overlay** with real-time statistics

### Performance Features
- **Asynchronous image loading** with thread pool management
- **Memory-efficient caching** with configurable sizes
- **GPU Hardware Acceleration** with Direct3D 11 (D3D11VA) for Video decoding
- **GPU-Accelerated HEIC/HEIF Decoding**: Natively routes High-Efficiency Image Containers through FFmpeg's video pipeline, providing full D3D11 hardware acceleration for HEIC photos.
- **Smart Video Thumbnails** with Black Frame Detection (skips dark intros)
- **Background processing** for file operations
- **Smart resource management** with RAII-based FFmpeg handling

### User Interface
- **Unified Interaction Model**: Consistent Mouse, Keyboard, and Touch controls.
- **Robust Selection**: `Ctrl+Click` (Toggle), `Shift+Click` (Range), and Drag-to-Select.
- **Dark theme** optimized for media viewing
- **Touch-friendly controls** for tablet/pen input
- **Keyboard shortcuts** for power users (Ctrl+A, Esc, Arrow Keys, Zoom)
- **Responsive design** that adapts to different screen sizes
- **Tab-based navigation** between different views

### Build System & Stability
- **Automated Module Verification**: `tst_linkage.exe` runs post-build to verify backend validity.
- **Unified Build Acript**: `build.ps1` handling dependencies and cleaning.

## 🚀 Quick Start

### Prerequisites
- Windows 10 or later
- Qt 6.4 or higher
- CMake 3.16 or higher
- C++17 compatible compiler

### Build from Source
```bash
# Clone the repository
git clone <repository-url>
cd QGalleryXClone

# 2. Setup Dependencies
# Clone LibRaw
git clone https://github.com/LibRaw/LibRaw.git 3rdparty/LibRaw

# Download FFmpeg (Manual Step)
# 1. Download "release-full-shared" from https://www.gyan.dev/ffmpeg/builds/
# 2. Extract and rename folder to 'ffmpeg' inside '3rdparty/'
# Structure: 3rdparty/ffmpeg/bin/avcodec-60.dll...

# 3. Build Project
mkdir build
cd build

# Configure with CMake
cmake .. -G "Visual Studio 17 2022" -A x64

# Build the project
cmake --build . --config Release

# Run the application
cd Release
.\QGalleryX.exe
```

### Using PowerShell Scripts
```powershell
# Build the project
.\build.ps1

# Clean build
.\build.ps1 -Clean

# Deploy the application
.\deploy.ps1
```

## 📦 Build Instructions

For detailed build instructions, troubleshooting, and deployment information, see [BUILD.md](BUILD.md).

### Build Requirements
- **Qt 6.4+** with modules: Core, Gui, Quick, QuickControls2, Multimedia, Concurrent
- **CMake 3.16+** for build configuration
- **Windows SDK** for Direct3D11/Vulkan support
- **psapi.lib** for system monitoring

### Build Methods
1. **CMake Command Line** (Recommended)
2. **Visual Studio Integration**
3. **PowerShell Scripts** (Simplified)

## 🏗️ Architecture

### Dual-Application Strategy

The project uses a two-application development approach:

| Application | Purpose | Status | Output |
|-------------|---------|--------|--------|
| **Main App** | Production gallery | ✅ Stable | `QGalleryX.exe` |
| **ScrollBench** | Performance testing & feature prototyping | ✅ Feature-complete | `QGalleryXBench.exe` |

**Development Flow:** 
```
Prototype in ScrollBench → Validate → Port to Main App
```

**Benefits:**
- Risk-free experimentation without breaking production app
- Performance testing in controlled environment
- Proven features before main app integration

### Technical Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    QML Frontend Layer                       │
├─────────────────────────────────────────────────────────────┤
│  Main.qml          │  GalleryView.qml   │  PhotoViewer.qml  │
│  (Application)     │  (Grid View)       │  (Image Viewer)   │
│                    │                    │                   │
│  AlbumsView.qml    │  BottomBar.qml     │  StatsOverlay.qml │
│  (Albums)          │  (Navigation)      │  (Performance)    │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                   Qt Integration Layer                      │
├─────────────────────────────────────────────────────────────┤
│  QML Context Properties  │  QML Type Registration          │
│  (appSettings)          │  (ImageModel, etc.)             │
│                         │                                  │
│  Image Provider         │  Signal-Slot Connections         │
│  (AsyncImageProvider)   │  (Backend-Frontend)             │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                   C++ Backend Layer                         │
├─────────────────────────────────────────────────────────────┤
│  ImageModel           │  FastVolumeScanner  │  TaskScheduler │
│  AlbumModel           │  VideoThumbnailer   │  SystemMonitor │
│  AsyncImageProvider   │  FrameBudgetScheduler │           │
└─────────────────────────────────────────────────────────────┘
```

For detailed architectural information, see [ARCHITECTURE.md](docs/ARCHITECTURE.md).

## 📚 Documentation

Comprehensive documentation has been created for this project:

- **[ARCHITECTURE.md](docs/ARCHITECTURE.md)** - Detailed system architecture and component relationships
- **[BUILD.md](docs/BUILD.md)** - Complete build instructions, troubleshooting, and deployment guide
- **[FEATURES.md](docs/FEATURES.md)** - Comprehensive feature documentation and technical specifications
- **[KNOWN_ISSUES.md](docs/KNOWN_ISSUES.md)** - Tracked bugs and their current status
- **[OUTSTANDING_TASKS.md](docs/resume/OUTSTANDING_TASKS.md)** - Active backlog and next steps for development
- **[PROGRESS.md](docs/PROGRESS.md)** - Analysis notes, findings, and recommendations

## 🔧 Requirements

### System Requirements
- **Operating System**: Windows 10 or later
- **RAM**: Minimum 4GB, Recommended 8GB+
- **Storage**: SSD recommended for better performance
- **GPU**: DirectX 11 compatible or better

### Development Requirements
- **Qt 6.4+** with the following modules:
  - Qt6::Core
  - Qt6::Gui
  - Qt6::Quick
  - Qt6::QuickControls2
  - Qt6::Multimedia
  - Qt6::Concurrent
  - Qt6::Network
  - Qt6::OpenGL
  - Qt6::Svg
  - Qt6::Widgets

### Build Tools
- **CMake 3.16+** for build configuration
- **C++17 compatible compiler** (MSVC 2019+ recommended)
- **Windows SDK** for Direct3D11/Vulkan support

## ⚠️ Known Issues

See [KNOWN_ISSUES.md](docs/KNOWN_ISSUES.md) for detailed list and workarounds.

### Current Limitations

**Feature Gaps:**
- Selection/Share features require ScrollBench (porting in progress)
- Image editing limited to basic crop (rotation/adjustments planned)

**Performance:**
- DNG proprietary compression has slow performance (120+ seconds)
- Case-sensitive file extension matching (uppercase files like `.JPG` missed)

**Platform:**
- Windows-only (Linux/macOS not supported)
- MFT scanner requires Administrator privileges for optimal performance

For detailed information and workarounds, see [docs/KNOWN_ISSUES.md](docs/KNOWN_ISSUES.md).

---

## 🤖 Handover & Future Work

This section is designed for the next AI or developer taking over this project.

### 📍 Entry Points
- **Current Backlog**: [docs/resume/OUTSTANDING_TASKS.md](docs/resume/OUTSTANDING_TASKS.md) (Contains the latest bugs identified: Concurrency Leak & Stall Timer).
- **Known Issues**: [docs/KNOWN_ISSUES.md](docs/KNOWN_ISSUES.md) (Comprehensive list of UI, Performance, and Platform bugs).
- **Build System**: Use [build.ps1](build.ps1) for all compilation and deployment tasks. It includes built-in diagnostics for file locks.

### 🎯 Immediate Next Steps
1.  **Fix AsyncImageProvider Leak**: Remove the double-increment of `activeWeight` in `DriveConcurrencyGuard`.
2.  **Implement Stall Recovery Timer**: Add a periodic timer to `AsyncImageProvider` to trigger `checkStalls()`.
3.  **Port ScrollBench Features**: Continue porting selection and share dialogs to the main `QGalleryX`.

## 🤝 Contributing

### Development Setup
1. Fork the repository
2. Set up Qt 6.4+ development environment
3. Configure CMake with appropriate generator
4. Build and test the application

### Code Style
- Follow Qt's coding conventions
- Use meaningful variable and function names
- Add appropriate comments for complex logic
- Implement proper error handling

### Testing
- Run existing unit tests before making changes
- Add new unit tests for new functionality
- Test on different Windows configurations
- Verify performance impact of changes

### Privacy & Security
- Do not commit sensitive data, personal test media (`*.jpg`, `*.png`), or execution logs (`*.log`).
- Ensure all debug media and local logs remain ignored via `.gitignore`.
- Sanitize developer-specific paths before committing documentation or scripts.

## 📄 License

This project is licensed under the MIT License. See the LICENSE file for details.

## 🏗️ Engineering Philosophy (How It Works)

This project adopts modern C++ best practices to ensure stability and performance:

-   **RAII (Resource Acquisition Is Initialization)**: We use RAII wrappers (e.g., `VideoThumbnailer::FFmpegCleanup`) to manage complex C-style resources like FFmpeg contexts. This ensures no memory leaks, even during exceptions or early returns.
-   **Modular Design**: Components are loosely coupled. The `TaskScheduler` doesn't know about Images, and `AsyncImageProvider` doesn't know about the UI. This makes testing and refactoring safer.
-   **Re-use**: We wrap standard libraries (`LibRaw`, `FFmpeg`, `DirectX`) into reusable helper classes (`VideoThumbnailer`, `DesktopHelper`) rather than scattering API calls throughout the code.
-   **Thread Safety**: Core resources (FFmpeg HW Contexts) are protected by mutexes, and concurrent access is managed via Semaphores to prevent GPU overload.

## 🔄 Version History

-   **v2.2.1 (Current)** - **Performance & Robustness**
    -   **GUI Optimization**: O(1) counters in `ScrollBenchImageModel` to prevent UI thread lockups.
    -   **Task Weighting**: Intelligent concurrency management for RAW/Video decodes.
    -   **CPU Backoff**: Dynamic throttling of I/O when system CPU usage is high (>70%).
    -   **Culling Fix**: Restored viewport culling by aligning path normalization in `VisibleRangeManager`.
    -   **Build Robustness**: `build.ps1` now handles file locks and verifies binary freshness via SHA256 hashes.
-   **v2.2.0** - **MFT Scanning & Performance**
-   **v2.1.0** - **Network & Deployment**
    -   **Network**: Full support for UNC paths (`\\\\Server\\Share`)
    -   **Deployment**: Self-contained builds with MinGW runtime included
    -   **Stability**: Removed aggressive memory limits to prevent UI freezes
-   **v2.0.0** - **Major "Reforged" Release**
    -   **Engine**: Full D3D11 Hardware Acceleration for videos
    -   **Smart Feature**: Black Frame Detection for meaningful thumbnails
    -   **Core**: TaskScheduler v2 with Priority Queues (CPU/IO separation)
    -   **Format**: Native RAW support via LibRaw
    -   **Performance**: Hybrid RAW loading (Embedded Preview vs Full Decode)
    -   **UI**: Semantic Zoom (Day/Month/Year) implemented
-   **v1.0.0** - Initial release with core gallery functionality

## 🗂️ File Structure

```
QGalleryXClone/
├── src/
│   ├── main.cpp                 # Application Entry
│   ├── TaskScheduler.cpp/h      # Thread Pool & Priority Queues
│   ├── VideoThumbnailer.cpp/h   # FFmpeg D3D11 Abstraction
│   ├── AsyncImageProvider.cpp/h # QQuickImageProvider implementation
│   ├── ImageModel.cpp/h         # File System Logic
│   ├── LogManager.cpp/h         # Thread-safe Logging
│   └── ...
├── resources/qml/
│   ├── Main.qml
│   ├── GalleryViewSemantic.qml  # The Smart Zoom View
│   ├── GalleryViewTiles.qml     # The Grid View
│   ├── StatsOverlay.qml         # Diagnostic Tools
│   └── ...
├── 3rdparty/
│   ├── ffmpeg/                  # FFmpeg 6.x Shared Libs
│   └── LibRaw/                  # LibRaw Headers/Source
├── docs/                        # Project Documentation
└── ...
```

---

**Note**: This is a work-in-progress project. Some features are incomplete or in placeholder state. See [FEATURES.md](FEATURES.md) and [PROGRESS.md](PROGRESS.md) for detailed information about current capabilities and future plans.

---

## ❤️ Dedication

To Goeffery "Your complaining about not being able to send images via email spured this exploration to a methodology that would intuitavely quell your frustrations, and start my personal journey down this rabbit whole.

Massandra - you had a shit computer and reminded me that one should never compromise scale and optimisation even for the lowest denominator."