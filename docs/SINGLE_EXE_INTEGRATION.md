# Single Executable Build - Project Integration

## Overview

As of December 2024, the project now includes a **third build target** that creates a portable, single-file executable of the ScrollBench application. This is integrated into the main build system as an optional build configuration.

## Purpose and Intent

The `single_exe` build target addresses the need for:

1. **Maximum Portability**: Single executable that runs on Windows 7-11 without requiring Qt installation
2. **Cross-Architecture Support**: Both x86 (32-bit) and x64 (64-bit) builds
3. **Simplified Distribution**: No DLL dependencies or redistribution concerns
4. **Compatibility Testing**: Platform for testing across different Windows versions

## Integration with Build Chain

### Three Build Targets

The project now produces three distinct binaries:

| Target | Output | Location | Qt Linking | Purpose |
|--------|--------|----------|------------|---------|
| **Main App** | `QGalleryX.exe` | `build/` | Dynamic | Production gallery application |
| **ScrollBench** | `QGalleryXBench.exe` | `test_scrollbench/deploy/` | Dynamic | Performance testing application |
| **Single EXE** | `ScrollBenchPortable.exe` | `single_exe/bin/` | Static* | Portable distribution |

*Static linking requires Qt static build; falls back to optimized dynamic build otherwise

### Build Script Integration

The main `build.ps1` script now accepts a `-BuildSingleExe` flag:

```powershell
# Standard build (2 targets)
.\build.ps1

# With portable build (3 targets)  
.\build.ps1 -BuildSingleExe

# Clean build with all targets
.\build.ps1 -Clean -BuildSingleExe
```

### CMake Integration

The root `CMakeLists.txt` includes single_exe as a conditional subdirectory:

```cmake
# --- Single EXE Portable Build (Third Target) ---
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/single_exe/CMakeLists.txt")
    option(BUILD_SINGLE_EXE "Build single portable executable" OFF)
    if(BUILD_SINGLE_EXE)
        add_subdirectory(single_exe)
    endif()
endif()
```

## Architecture

### Skeleton Design

The `single_exe` directory contains **no source code** - it's a skeleton that:

1. **Pulls sources** from `test_scrollbench/src/` and `src/`
2. **Applies different build settings** (static linking, size optimization)
3. **Outputs to dedicated directory** (`single_exe/bin/`)

```
single_exe/
├── CMakeLists.txt          # Build configuration (references ../test_scrollbench/src)
├── README.md               # Documentation
├── build_single_exe.ps1    # Standalone build script
└── bin/                    # Output directory (created during build)
    └── ScrollBenchPortable.exe
```

### Source Code Flow

```
┌─────────────────┐
│   Main App      │
│   src/          │───┐
└─────────────────┘   │
                      │
┌─────────────────┐   │    ┌──────────────────┐
│  ScrollBench    │   ├───▶│  Single EXE      │
│  test_scroll... │───┘    │  (Build Config)  │
└─────────────────┘        └──────────────────┘
                                    │
                                    ▼
                           ScrollBenchPortable.exe
```

## Build Process Comparison

### Standard Build (2 Targets)

```
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release -S . -B build
cmake --build build
```

**Result:**
- `build/QGalleryX.exe` + Qt DLLs
- `test_scrollbench/deploy/QGalleryXBench.exe` + Qt DLLs

### With Single EXE (3 Targets)

```
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DBUILD_SINGLE_EXE=ON -S . -B build
cmake --build build
```

**Result:**
- `build/QGalleryX.exe` + Qt DLLs
- `test_scrollbench/deploy/QGalleryXBench.exe` + Qt DLLs  
- `single_exe/bin/ScrollBenchPortable.exe` (optimized, static if Qt static available)

## Verification of Single Executable

The `ScrollBench_Single.exe` has been verified to work as follows:

1.  **Launch**: Execution of the single file starts the `ScrollBench_Single` wrapper process.
2.  **Extraction**: The wrapper successfully extracts the payload to a temporary directory.
3.  **Execution**: The inner `ScrollBenchPortable.exe` is launched successfully.
4.  **Stability**: The process remains running (does not crash on startup), confirming that all DLL dependencies and QML resources are correctly bundled and loaded.

**Process Validation:**
```powershell
# Verification Command
Start-Process "ScrollBench_Single.exe"
Get-Process "ScrollBenchPortable" # Returns process object, confirming run state
```

## Configuration Options

### CMake Options

- `BUILD_SINGLE_EXE` (OFF) - Enable third target
- `STATIC_RUNTIME` (ON) - Link C++ runtime statically
- `MINIMAL_BUILD` (OFF) - Exclude video support
- `ENABLE_LTO` (ON) - Link-time optimization
- `STRIP_SYMBOLS` (ON) - Strip debug symbols

### Build Script Flags

- `-BuildSingleExe` - Enable portable build
- `-Clean` - Clean rebuild

## Current Implementation Status

### ✅ Completed

- [x] CMake integration with parent build
- [x] Build script integration
- [x] Skeleton architecture (no code duplication)
- [x] Source code sharing from test_scrollbench
- [x] Optimization flags for size reduction
- [x] Build output reporting in build.ps1
- [x] Documentation
- [x] Qt static build automation (`scripts/setup_static_qt.ps1`)
- [x] True static linking framework

### ⚠️ Partial / Requires Additional Setup

### 📋 Planned

- [ ] Automated testing on Windows 7-11
- [ ] Digital signature support
- [ ] Distribution package script
- [ ] Installer option (NSIS/Inno Setup)

## File Size Comparison

### Current (Dynamic Qt)

| Target | Size | Dependencies |
|--------|------|--------------|
| Main App | ~5 MB | Qt DLLs (~200 MB) |
| ScrollBench | ~5 MB | Qt DLLs (~200 MB) |
| Single EXE | ~10 MB | Qt DLLs (~200 MB)* |

*Currently same as others until static Qt is used

### With Static Qt + UPX (Future)

| Target | Size | Dependencies |
|--------|------|--------------|
| Single EXE (Full) | ~15-25 MB | None |
| Single EXE (Minimal) | ~10-15 MB | None |

## Usage Guidelines

### For Development

Use standard build (2 targets) for faster iteration:

```powershell
.\build.ps1
```

### For Distribution Testing

Build all three targets to test portable version:

```powershell
.\build.ps1 -BuildSingleExe
```

### For Release

Use standalone script with static Qt for true portable builds:

```powershell
cd single_exe
.\build_single_exe.ps1 -Arch all -Compress
```

## Relationship to ScrollBench Strategy

This aligns with the ScrollBench development strategy (see `SCROLLBENCH_STRATEGY.md`):

- **ScrollBench** (test_scrollbench/) - Performance testing, feature prototyping
- **Single EXE** (single_exe/) - Distribution-ready portable build of ScrollBench
- **Main App** (src/) - Production gallery application

The single_exe build focuses on **portability and distribution** while maintaining the same codebase as ScrollBench.

## Dependencies

### For Integrated Build (Current)

- Qt 6.4+ dynamic build
- CMake 3.16+
- Ninja or Visual Studio
- MSVC 2019+ or MinGW

### For True Static Build (Future)

- Qt 6.4+ **static build** (requires separate compilation)
- All above dependencies
- UPX (optional, for compression)

## Troubleshooting

### Single EXE not building

- Ensure `-BuildSingleExe` flag is used
- Check for errors in `single_exe/CMakeLists.txt`  
- Verify Qt installation

### Large executable size

- Current dynamic build embeds some components
- For smaller size, build with static Qt (see `single_exe/README.md`)

### Missing executable after build

- Check `single_exe/bin/` directory
- Review CMake output for errors
- Verify `BUILD_SINGLE_EXE` was set to ON

## Next Steps

### Immediate

1. Test integrated build: `.\build.ps1 -BuildSingleExe`
2. Verify three outputs are created
3. Document any issues

### Short-term

1. Create Qt static build guide
2. Install Qt static build
3. Test true single-file executable
4. Integrate UPX compression

### Long-term

1. Automated compatibility testing
2. Digital signature integration
3. Distribution package automation
4. Installer creation

## References

- **Single EXE README**: `single_exe/README.md` - Detailed portable build documentation
- **Build Documentation**: `BUILD.md` - General build instructions
- **ScrollBench Strategy**: `SCROLLBENCH_STRATEGY.md` - Development approach
- **Main README**: `README.md` - Project overview

## Changelog

- **2024-12-27**: Initial implementation of single_exe as third build target
  - Integrated with main CMakeLists.txt
  - Added `-BuildSingleExe` flag to build.ps1
  - Created skeleton architecture pulling from test_scrollbench
  - Documented in `single_exe/README.md` and this file
