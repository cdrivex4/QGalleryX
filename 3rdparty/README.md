# Third-Party Dependencies Setup

This directory contains external dependencies required to build the Antigravity project. Large build artifacts are excluded from git - follow the instructions below to set up your local environment.

---

## 📦 Required Dependencies

### 1. FFmpeg (Video/Audio Processing)

**Status:** ✅ Included in repository (shared libraries)  
**Location:** `3rdparty/ffmpeg/`

FFmpeg binaries are committed to the repository for convenience.

**Manual Setup (if needed):**
```powershell
# Download FFmpeg release-full-shared build
# From: https://www.gyan.dev/ffmpeg/builds/
# Extract and rename folder to: 3rdparty/ffmpeg/
# Expected structure: 3rdparty/ffmpeg/bin/avcodec-60.dll
```

---

### 2. LibRaw (RAW Image Processing)

**Status:** ✅ Included in repository (source code)  
**Location:** `3rdparty/LibRaw/`

LibRaw source is included via git submodule or direct clone.

**Manual Setup (if needed):**
```powershell
cd 3rdparty
git clone https://github.com/LibRaw/LibRaw.git LibRaw
```

---

### 3. Qt Static Build (Optional - for Single EXE)

**Status:** ⚠️ **NOT included** (large build artifacts, ~2-5 GB)  
**Location:** `3rdparty/qt_static/` (gitignored)

**Automated Setup:**
```powershell
# Run automated Qt static build script
.\scripts\setup_static_qt.ps1

# This will:
# 1. Download Qt 6.9.3 source
# 2. Download and install Strawberry Perl (dependency)
# 3. Configure Qt for static linking
# 4. Compile Qt (takes 2-4 hours)
# 5. Install to 3rdparty/qt_static/
```

**Manual Setup:**
```powershell
# Download Qt source
# From: https://download.qt.io/official_releases/qt/6.9/6.9.3/single/
# Extract to: 3rdparty/qt-everywhere-src-6.9.3/

# Configure and build
cd 3rdparty/qt-everywhere-src-6.9.3
.\configure -static -release -prefix "D:/Dev/antigravity/3rdparty/qt_static" `
  -nomake examples -nomake tests -skip qtwebengine

cmake --build . --parallel
cmake --install .
```

**Expected Structure After Build:**
```
3rdparty/qt_static/
├── bin/
├── include/
├── lib/
├── mkspecs/
└── qml/
```

---

### 4. Build Tools (Perl, etc.)

**Status:** ⚠️ **NOT included** (gitignored)  
**Location:** `3rdparty/tools/` (gitignored)

These are automatically downloaded by `setup_static_qt.ps1`.

**Manual Setup:**
```powershell
# Strawberry Perl (for Qt build)
# Download from: https://strawberryperl.com/
# Install to: 3rdparty/tools/perl/
```

---

## 🔧 Build Workflows

### Standard Build (Dynamic Qt)
Uses system-installed Qt (Qt 6.4+).

```powershell
# No 3rdparty setup needed beyond FFmpeg and LibRaw
.\build.ps1
```

**Outputs:**
- `build/appSamsungGallery.exe` + Qt DLLs
- `test_scrollbench/deploy/appScrollBench.exe` + Qt DLLs

---

### Single Executable Build (Static Qt)
Requires static Qt build from step 3 above.

```powershell
# 1. Build static Qt (one-time, 2-4 hours)
.\scripts\setup_static_qt.ps1

# 2. Build single executable
.\build.ps1 -BuildSingleExe
```

**Output:**
- `single_exe/bin/ScrollBenchPortable.exe` (no DLLs required)

---

## 📂 Directory Structure

```
3rdparty/
├── ffmpeg/                 ✅ Committed (shared libs)
│   ├── bin/
│   ├── include/
│   └── lib/
├── LibRaw/                 ✅ Committed (source)
│   ├── CMakeLists.txt
│   └── libraw/
├── qt_static/              ❌ Gitignored (build with script)
│   └── [see step 3]
├── tools/                  ❌ Gitignored (build with script)
│   └── perl/
└── README.md               ✅ This file
```

---

## ⚠️ Important Notes

1. **Build Time**: Static Qt compilation takes **2-4 hours** on first setup
2. **Disk Space**: Static Qt requires **~5 GB** of disk space
3. **Administrator**: MFT scanner feature requires running as Administrator
4. **Windows Only**: Project is Windows-specific (MinGW/MSVC)

---

## 🚀 Quick Start

**Minimal Setup (dynamic build only):**
```powershell
# 1. Verify FFmpeg and LibRaw are present
ls 3rdparty/ffmpeg/bin/*.dll
ls 3rdparty/LibRaw/

# 2. Build project
.\build.ps1
```

**Full Setup (with single EXE support):**
```powershell
# 1. Build static Qt (one-time)
.\scripts\setup_static_qt.ps1

# 2. Build all targets
.\build.ps1 -BuildSingleExe
```

---

## 🔗 References

- [Build Documentation](../docs/BUILD.md)
- [Single EXE Integration](../docs/SINGLE_EXE_INTEGRATION.md)
- [Static Qt Setup Guide](../docs/STATIC_QT_SETUP.md)

---

**Last Updated:** 2025-12-29  
**Project Version:** v2.2.0
