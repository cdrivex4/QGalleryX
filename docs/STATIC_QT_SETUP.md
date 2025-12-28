# Static Qt Build Strategy

## Overview
To create a true single-file executable without using self-extraction wrappers, we compile Qt from source with the `-static` configuration. This build is local to the project.

## Automated Setup
Run the following script to download Perl (portable), fetch Qt source, and compile it:

```powershell
.\scripts\setup_static_qt.ps1
```

### What it does:
1.  **Dependencies**: Downloads Strawberry Perl Portable to `3rdparty/tools/perl`.
2.  **Source**: Clones Qt 6.9.3 (Minimal modules) to `D:\Qt\Src_6.9.3`.
3.  **Build**: Compiles Release Static binaries.
4.  **Install**: Installs the SDK to `3rdparty/qt_static`.

## Usage
Once the static kit is built, update `build.ps1` or `CMakeLists.txt` to use the new kit:

```powershell
$env:CMAKE_PREFIX_PATH = "$PSScriptRoot/3rdparty/qt_static"
```

Then run the standard build:
```powershell
.\build.ps1 -BuildSingleExe
```
This will produce a native, non-extracting `ScrollBenchPortable.exe`.
