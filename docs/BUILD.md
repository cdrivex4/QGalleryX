# QGalleryX - Build and Deployment Guide

## Build System

The project uses CMake as the build system with Qt 6.4 as the primary framework.

### Prerequisites

#### System Requirements
- Windows 10 or later
- CMake 3.16 or higher
- C++17 compatible compiler (MSVC 2019 or later recommended)
- Qt 6.4 or higher

#### Required Qt Modules
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

#### Additional Dependencies
- **psapi.lib** (Windows API for process information)
- **Windows SDK** for Direct3D11/Vulkan support
- **FFmpeg 6.x Shared Libraries** (avcodec, avformat, avutil, swscale)
- **LibRaw** (for RAW image support)

### 3rdparty Setup (Crucial!)

Before building, you must populate the `3rdparty/` directory:

1.  **LibRaw** (Source Code):
    ```bash
    git clone https://github.com/LibRaw/LibRaw.git 3rdparty/LibRaw
    ```

2.  **FFmpeg** (Shared Binaries):
    *   Download the **release-full-shared** build from [gyan.dev](https://www.gyan.dev/ffmpeg/builds/).
    *   Extract the contents.
    *   Rename the folder to `ffmpeg` and place it in `3rdparty/`.
    *   Ensure structure is: `3rdparty/ffmpeg/bin/avcodec-60.dll` etc.

### Build Instructions

#### Method 1: Using CMake (Recommended)

```bash
# Create build directory
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

#### Method 2: Using PowerShell Scripts (Recommended)

The project includes a robust PowerShell script for building and deployment:

```powershell
# 1. Standard Dynamic Build (Development & Testing)
# Builds QGalleryX.exe (Main) and QGalleryXBench.exe (Test) linked dynamically to Qt.
.\build.ps1

# 2. Clean Build
# Removes build directory and rebuilds from scratch. Recommended after major changes.
.\build.ps1 -Clean

# 3. Single Executable Portable Build (Production/Distribution)
# Builds ScrollBenchPortable.exe sttically linked (No DLLs required).
# REQUIRES: Static Qt Setup first (see below).
.\build.ps1 -BuildSingleExe
```

### Static Qt Setup (For Single Executable)

To build the Single Executable, you must first setup a static build of Qt. We provide an automated script for this:

```powershell
# Download and compiles a minimal Static Qt 6.9.3 kit
.\scripts\setup_static_qt.ps1
```

**Build Targets:**
- **Standard Build**: Dynamic linking. Fast iteration. producing `QGalleryX.exe`.
- **ScrollBench**: Use `test_scrollbench` for performance testing.
- **Single Exe**: `ScrollBenchPortable.exe`. Native static binary. Ideal for USB drives/deployment without installers.
  - See `single_exe/README.md` for details.

#### Method 3: Visual Studio Integration

1. Open CMakeLists.txt in Visual Studio 2022
2. Select x64 Release configuration
3. Build → Build Solution
4. Run the application from the output directory

### Build Configuration

#### CMake Options
- `-DCMAKE_BUILD_TYPE=Release` - Release build (default)
- `-DCMAKE_BUILD_TYPE=Debug` - Debug build with symbols
- `-G "Visual Studio 17 2022"` - Visual Studio 2022 generator
- `-A x64` - 64-bit architecture

#### Build Artifacts
- `build/Release/QGalleryX.exe` - Main executable
- `build/Release/` - All compiled binaries and dependencies
- `deploy/` - Deployment directory with all runtime dependencies

### Deployment

#### Automatic Deployment
The `deploy.ps1` script automatically:
- Copies the executable to the deploy directory
- Copies all required Qt DLLs
- Copies QML modules and plugins
- **Copies MinGW Compiler Runtime** (`libgcc`, `libstdc++`, etc.) to ensure portability.
- Creates a complete standalone package capable of running from network shares.

#### Manual Deployment
1. Create a deployment directory
2. Copy the executable
3. Copy required Qt DLLs from Qt installation
4. Copy QML modules from Qt installation
5. Copy any required plugins (multimedia, image formats)

### Runtime Dependencies

#### Required Qt DLLs
- Qt6Core.dll
- Qt6Gui.dll
- Qt6Quick.dll
- Qt6QuickControls2.dll
- Qt6Multimedia.dll
- Qt6MultimediaQuick.dll
- Qt6Network.dll
- Qt6OpenGL.dll
- Qt6Svg.dll
- Qt6Widgets.dll
- Qt6Qml.dll
- Qt6QmlModels.dll
- Qt6QuickTemplates2.dll
- Qt6QuickLayouts.dll
- Qt6QuickShapes.dll
- Qt6QuickEffects.dll

#### QML Modules
- QtQuick/Controls
- QtQuick/Layouts
- QtQuick/Shapes
- QtQuick/Effects
- QtMultimedia

#### Platform Plugins
- platforms/qwindows.dll

### Build Issues and Known Problems

#### Common Build Errors

1. **Qt Module Not Found**
   - Ensure Qt 6.4+ is properly installed and in PATH
   - Verify CMake can find Qt using `cmake -DQt6_DIR=D:/Qt/6.9.3/mingw_64/lib/cmake/Qt6`

2. **Missing psapi.lib**
   - Install Windows SDK
   - Ensure Visual Studio includes Windows SDK components

3. **Direct3D11/Vulkan Headers Missing**
   - Install Windows SDK
   - Update graphics drivers

4. **QML Files Not Found**
   - Ensure QML files are properly included in CMakeLists.txt
   - Check Qt installation includes QML modules

#### Platform-Specific Issues

**Windows:**
- Ensure Windows SDK is installed for Direct3D11/Vulkan support
- Use Visual Studio 2019 or later for proper C++17 support
- Enable Windows SDK in Visual Studio installer

**Linux/Mac:**
- Not officially supported but may work with Qt 6.4+
- Would need X11/Wayland support
- OpenGL drivers required

### Testing

The project includes unit tests:

```bash
# Run tests
cd build
ctest --output-on-failure

# Or run specific test
.\Release\tst_imagemodel.exe
```

### Performance Considerations

#### Build Optimization
- Use Release configuration for optimal performance
- Enable Link Time Optimization (LTO) if available
- Use appropriate compiler optimization flags

#### Debug Build
- Use Debug configuration for development
- Include debug symbols for troubleshooting
- Disable optimizations for easier debugging

### Troubleshooting

#### Build Verification
1. Check CMake configuration output
2. Verify all Qt modules are found
3. Confirm all source files are included
4. Test executable runs independently

#### Runtime Issues
1. Check all DLLs are in executable directory
2. Verify QML modules are accessible
3. Ensure graphics drivers are up to date
4. Check system meets minimum requirements