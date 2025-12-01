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
- **GPU acceleration** with multiple API support
- **Background processing** for file operations
- **Smart resource management**

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

# Create build directory
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
# Clean build (optional)
.\build_clean.ps1

# Build the project
.\build.ps1

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

The application follows a modern Qt-based architecture with clear separation of concerns:

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
│  QML Context Properties  │  QML Type Registration  │        │
│  (appSettings)          │  (ImageModel, etc.)     │        │
│                         │                         │        │
│  Image Provider         │  Signal-Slot Connections │        │
│  (AsyncImageProvider)   │  (Backend-Frontend)     │        │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                   C++ Backend Layer                         │
├─────────────────────────────────────────────────────────────┤
│  ImageModel.cpp/h      │  AlbumModel.cpp/h      │        │
│  (Image Data)          │  (Album Data)          │        │
│                         │                         │        │
│  SettingsHelper.cpp/h  │  AsyncImageProvider.cpp/h│        │
│  (Configuration)       │  (Async Loading)       │        │
└─────────────────────────────────────────────────────────────┘
```

For detailed architectural information, see [ARCHITECTURE.md](ARCHITECTURE.md).

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

### Build Issues
1. **Qt Version Mismatch**: Build scripts reference Qt 6.9.3 but project requires 6.4+
2. **Hardcoded Paths**: PowerShell scripts contain hardcoded Qt installation paths
3. **Platform Dependencies**: Windows-specific APIs limit cross-platform compatibility

### Runtime Issues
1. **Default Path**: Hardcoded path "I:/MY SDCards/dir0064.chk" may not exist
2. **Incomplete Features**: Albums and Stories features are placeholder implementations
3. **Video Playback**: No actual video player implementation (placeholders only)

### Code Quality Issues
1. **Error Handling**: Some functions return generic error messages
2. **Code Duplication**: Similar path handling in multiple files
3. **Limited Documentation**: Missing API documentation and user guides

For a complete analysis of issues and recommendations, see [PROGRESS.md](PROGRESS.md).

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

## 🔄 Version History

- **v1.0.0** - Initial release with core gallery functionality
- **v0.9.0** - Alpha release with basic features
- **v0.8.0** - Development version with performance optimizations

## 📞 Support

For support and questions:
- Create an issue on GitHub
- Check the [BUILD.md](BUILD.md) file for troubleshooting
- Review [PROGRESS.md](PROGRESS.md) for known issues and solutions

## 🗂️ File Structure

```
SamsungGalleryClone/
├── src/                    # C++ source files
│   ├── main.cpp           # Application entry point
│   ├── ImageModel.cpp/h   # Image data model
│   ├── AlbumModel.cpp/h   # Album data model
│   ├── SettingsHelper.cpp/h # Configuration and system info
│   └── AsyncImageProvider.cpp/h # Async image loading
├── resources/qml/          # QML user interface files
│   ├── Main.qml           # Main application window
│   ├── GalleryView.qml    # Grid-based image browser
│   ├── PhotoViewer.qml    # Full-screen image viewer
│   ├── AlbumsView.qml     # Album organization (placeholder)
│   ├── BottomBar.qml      # Tab navigation
│   ├── StatsOverlay.qml   # Performance monitoring
│   └── UsageGraph.qml     # Real-time performance graphs
├── tests/                  # Test files
│   └── tst_imagemodel.cpp # ImageModel unit tests
├── docs/                   # Documentation
│   ├── README.md          # This file
│   ├── ARCHITECTURE.md    # Architecture documentation
│   ├── BUILD.md           # Build instructions
│   ├── FEATURES.md        # Feature documentation
│   └── PROGRESS.md        # Progress notes
├── CMakeLists.txt          # CMake build configuration
├── build.ps1              # PowerShell build script
├── deploy.ps1             # PowerShell deployment script
└── build_clean/           # Build output directory
```

---

**Note**: This is a work-in-progress project. Some features are incomplete or in placeholder state. See [FEATURES.md](FEATURES.md) and [PROGRESS.md](PROGRESS.md) for detailed information about current capabilities and future plans.