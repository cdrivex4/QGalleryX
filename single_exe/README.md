# Single Executable Build Integration

## Overview

The `single_exe` directory contains a **third build target** that creates a
- **Portable Distribution**: Self-contained folder with all dependencies
- **Cross-Architecture**: Both x86 (32-bit) and x64 (64-bit) builds
- **Optimized Size**: Compiler optimizations for minimal binary size
- **Deployable**: Can be zipped and run on any Windows 7+ machine

## Output Format

The build system creates two deliverables in `single_exe/bin/`:

1.  **ScrollBench_Single.exe** (Main Output)
    -   **Format**: Single Self-Extracting Executable (SFX)
    -   **Mechanism**: C# + .NET Compiler wrapper enclosing a compressed payload
    -   **Behavior**: Auto-extracts to `%TEMP%`, runs the app, and cleans up on exit
    -   **Size**: ~50 MB (Compressed from ~180 MB)
    -   **Dependencies**: Requires .NET Framework 4.5+ (Standard on Win7/8/10/11)

2.  **ScrollBenchPortable.exe** (Intermediate)
    -   The core application executable (requires surrounding DLLs)

3.  **Portable Folder**
    -   The full directory containing all DLLs and plugins (useful for debugging)

## Integration with Main Build Chain

The single_exe build is **integrated into the main build system** and can be built alongside the main application and ScrollBench test application.

### Build Targets

When building the project, you can now create **three binaries**:

1. **appSamsungGallery.exe** - Main gallery application (dynamic linking)
2. **appScrollBench.exe** - ScrollBench test application (dynamic linking)
3. **ScrollBenchPortable.exe** - Single portable executable (static linking)

## How It Works

### Architecture

The `single_exe` directory is a **skeleton build configuration** that:

- **Pulls source code** from `test_scrollbench/src/` (no code duplication)
- **Applies different build settings** (static Qt, static runtime, size optimization)
- **Produces a standalone binary** with all dependencies embedded

### Source Code Sharing

```
single_exe/
├── CMakeLists.txt          # Build configuration (skeleton)
├── README.md               # This file
├── build_single_exe.ps1    # Standalone build script
└── bin/                    # Output directory
    └── ScrollBenchPortable.exe  # Generated binary
```

**No source code lives in single_exe/** - it all comes from:
- `test_scrollbench/src/*` - ScrollBench-specific code  
- `src/*` - Shared main application code

## Building

### Method 1: Integrated Build (Recommended)

Build all three binaries in one command:

```powershell
# Build main app + ScrollBench + Single EXE portable
.\build.ps1 -BuildSingleExe

# Clean build with all targets
.\build.ps1 -Clean -BuildSingleExe
```

**Output:**
- `build/appSamsungGallery.exe` - Main app
- `test_scrollbench/deploy/appScrollBench.exe` - ScrollBench
- `single_exe/bin/ScrollBenchPortable.exe` - Portable static build

### Method 2: Standalone Build

Build only the portable executable:

```powershell
cd single_exe
.\build_single_exe.ps1 -Arch x64
```

This method requires a **static Qt build** to be installed separately.

## Requirements

### For Integrated Build (Method 1)

- Standard Qt dynamic build (same as main project)
- CMake 3.16+
- MSVC 2019+ or MinGW-w64

**Note:** The integrated build currently uses **dynamic Qt** but applies optimization flags. For a true single-file executable, you need static Qt (Method 2).

### For Standalone Static Build (Method 2)

- **Qt Static Build** (see `docs/QT_STATIC_BUILD.md` - to be created)
- CMake 3.16+
- MSVC 2019+ or MinGW-w64
- UPX (optional, for compression)

## Current Status

### ✅ Implemented

- [x] Skeleton build configuration integrated with main CMakeLists.txt
- [x] Source code sharing from test_scrollbench
- [x] Build script integration with `-BuildSingleExe` flag
- [x] Output directory structure
- [x] CMake configuration for optimization

### ⚠️ Requires Static Qt

- [ ] Static Qt build installation
- [ ] True single-file executable (currently requires Qt DLLs)
- [ ] Full Win7-Win11 compatibility testing

### 📝 To Do

- [ ] Create Qt static build guide (`docs/QT_STATIC_BUILD.md`)
- [ ] Add UPX compression post-build step
- [ ] Create distribution package script
- [ ] Add digital signature support
- [ ] Test on all target Windows versions

## Configuration Options

When building via CMake, you can configure:

```cmake
-DBUILD_SINGLE_EXE=ON       # Enable single exe build
-DSTATIC_RUNTIME=ON         # Link C++ runtime statically  
-DMINIMAL_BUILD=ON          # Exclude video support (smaller)
-DENABLE_LTO=ON             # Link-time optimization
-DSTRIP_SYMBOLS=ON          # Strip debug symbols
```

## File Size Estimates

| Configuration | With Qt DLLs | Static Qt | Static Qt + UPX |
|---------------|--------------|-----------|-----------------|
| Full Build    | ~250 MB      | ~45-60 MB | ~15-25 MB       |
| Minimal       | ~200 MB      | ~25-35 MB | ~10-15 MB       |

*Minimal = No video support*

## Technical Details

### Static Linking Strategy

1. **Qt Libraries**: Linked statically (requires static Qt build)
2. **C++ Runtime**: Linked statically (no MSVC redistributable needed)
3. **LibRaw**: Already static (libraw_static target)
4. **FFmpeg**: Currently dynamic (can be made static)
5. **QML Resources**: Embedded in binary via Qt resource system

### Optimization Flags

**MSVC:**
- `/O1` - Optimize for size
- `/GL` - Whole program optimization
- `/LTCG` - Link-time code generation
- `/Gy` - Function-level linking
- `/Gw` - Optimize global data

**MinGW:**
- `-Os` - Optimize for size
- `-s` - Strip symbols
- `-static-libgcc -static-libstdc++` - Static runtime

## Usage Examples

### Build with default Qt (dynamic)

```powershell
.\build.ps1 -BuildSingleExe
```

Result: `single_exe/bin/ScrollBenchPortable.exe` + Qt DLLs required

### Build with static Qt

```powershell
cd single_exe
.\build_single_exe.ps1 -Arch x64 -Compress
```

Result: Single 15-25 MB file, no dependencies

### Build both architectures

```powershell
.\build_single_exe.ps1 -Arch all -Compress
```

Result:
-ScrollBenchPortable_x64.exe` (~15-25 MB)
- `ScrollBenchPortable_x86.exe` (~12-20 MB)

## Relationship to Other Builds

```
antigravity/
├── src/                    # Main app source
├── CMakeLists.txt          # Builds: appSamsungGallery.exe
│
├── test_scrollbench/       # ScrollBench test app
│   ├── CMakeLists.txt      # Builds: appScrollBench.exe (dynamic)
│   └── deploy/             # Output with Qt DLLs
│
└── single_exe/             # Portable build (SKELETON)
    ├── CMakeLists.txt      # Builds: ScrollBenchPortable.exe (static)
    └── bin/                # Output - single file
```

**Key Point:** `single_exe` has **no source code** - it's purely a build configuration.

## Distribution

### Recommended Package

```
ScrollBench_v1.0/
├── ScrollBenchPortable_x64.exe    # For 64-bit Windows
├── ScrollBenchPortable_x86.exe    # For 32-bit Windows  
└── README.txt                      # Usage instructions
```

### System Requirements

- **OS**: Windows 7 SP1 or later
- **RAM**: 2 GB minimum, 4 GB recommended
- **Disk**: 50 MB free space
- **Graphics**: DirectX 11 compatible GPU (for hardware acceleration)

## Troubleshooting

### "Qt platform plugin not found"

- You're using dynamic Qt build. Either:
  - Build with static Qt using `build_single_exe.ps1`
  - Or deploy Qt DLLs alongside the executable

### Build fails with "Qt6 not found"

- Ensure Qt is installed and in PATH
- For static build, update Qt path in `build_single_exe.ps1`

### Large file size (>100 MB)

- You're using dynamic Qt - DLLs are bundled
- For smaller size, use static Qt build + UPX compression

## Next Steps

1. **Create Static Qt Build**: Follow guide at `docs/QT_STATIC_BUILD.md` (to be created)
2. **Test Build**: Run `.\build.ps1 -BuildSingleExe` to ensure integration works
3. **Optimize**: Experiment with `-DMINIMAL_BUILD=ON` for smaller binaries
4. **Compress**: Use UPX to reduce final size by 60-70%
5. **Test Compatibility**: Verify on Windows 7, 8, 10, and 11

## References

- [Main Build Documentation](../docs/BUILD.md)
- [ScrollBench Strategy](../docs/SCROLLBENCH_STRATEGY.md)  
- [Main Project README](../docs/README.md)
- Build Script: `build_single_exe.ps1`
- Parent CMake: `../CMakeLists.txt` (line 57-67)
