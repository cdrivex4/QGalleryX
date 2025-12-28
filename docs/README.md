# Samsung Gallery Clone

A modern Qt-based photo gallery application that replicates the functionality and user interface of Samsung's native gallery application. Built with C++ backend and QML frontend for optimal performance and user experience.

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
- [Contributing](#contributing)

## 🎯 Project Overview

Samsung Gallery Clone is a feature-rich photo gallery application designed to provide a smooth, responsive experience for browsing and viewing photos and videos. The application leverages Qt 6's modern capabilities to deliver:

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
- **GPU acceleration** with Direct3D 11 (D3D11VA) for video decoding
- **Smart Video Thumbnails** with Black Frame Detection (skips dark intros)
- **Background processing** for file operations
- **Smart resource management** with RAII-based FFmpeg handling

### User Interface
- **Dark theme** optimized for media viewing
- **Touch-friendly controls** for tablet/pen input
- **Keyboard shortcuts** for power users
- **Responsive design** that adapts to different screen sizes
- **Tab-based navigation** between different views

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
cd SamsungGalleryClone

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
.\appSamsungGallery.exe
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
| **Main App** | Production gallery | ✅ Stable | `appSamsungGallery.exe` |
| **ScrollBench** | Performance testing & feature prototyping | ✅ Feature-complete | `appScrollBench.exe` |

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

- **[ARCHITECTURE.md](ARCHITECTURE.md)** - Detailed system architecture and component relationships
- **[BUILD.md](BUILD.md)** - Complete build instructions, troubleshooting, and deployment guide
- **[FEATURES.md](FEATURES.md)** - Comprehensive feature documentation and technical specifications
- **[PROGRESS.md](PROGRESS.md)** - Analysis notes, findings, and recommendations

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

### Pull Request Process
1. Create feature branch from main
2. Make changes and add tests
3. Ensure all tests pass
4. Submit pull request with detailed description
5. Respond to code review feedback

## 📄 License

This project is licensed under the MIT License. See the LICENSE file for details.

## 🏗️ Engineering Philosophy (How It Works)

This project adopts modern C++ best practices to ensure stability and performance:

-   **RAII (Resource Acquisition Is Initialization)**: We use RAII wrappers (e.g., `VideoThumbnailer::FFmpegCleanup`) to manage complex C-style resources like FFmpeg contexts. This ensures no memory leaks, even during exceptions or early returns.
-   **Modular Design**: Components are loosely coupled. The `TaskScheduler` doesn't know about Images, and `AsyncImageProvider` doesn't know about the UI. This makes testing and refactoring safer.
-   **Re-use**: We wrap standard libraries (`LibRaw`, `FFmpeg`, `DirectX`) into reusable helper classes (`VideoThumbnailer`, `DesktopHelper`) rather than scattering API calls throughout the code.
-   **Thread Safety**: Core resources (FFmpeg HW Contexts) are protected by mutexes, and concurrent access is managed via Semaphores to prevent GPU overload.

## 🔄 Version History

-   **v2.2.0 (Current)** - **MFT Scanning & Performance**
    -   **MFT Scanner**: 10-100x faster file enumeration via Windows MFT (requires Admin)
    -   **Frame Budget**: Prevents UI stuttering during heavy thumbnail operations
    -   **FileTypeRouter**: Centralized detection for 170+ formats (RAW, Image, Video)
    -   **ScrollBench**: Feature-complete test application with selection & share
    -   **TDR Fixes**: Reduced video/RAW concurrency to prevent GPU timeout crashes
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
SamsungGalleryClone/
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