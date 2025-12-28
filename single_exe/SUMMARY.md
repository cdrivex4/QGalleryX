# Single Executable Status
Date: 2025-12-27

## Achievement Unlocked
Successfully created **ScrollBench_Single.exe** - a true single-file self-extracting executable.

## How it Works
1.  **Build**: Compiles `ScrollBenchPortable.exe` using CMake (Optimized, No LTO).
2.  **Deploy**: Runs `windeployqt` to gather all Qt dependencies into `single_exe/bin`.
3.  **Consolidate**:
    -   Compresses the entire folder into `payload.zip`.
    -   Generates a C# `Launcher.cs` stub that embeds the zip.
    -   Compiles the launcher using `csc.exe` (Built-in Windows C# Compiler).

## Result
-   **File**: `single_exe/bin/ScrollBench_Single.exe`
-   **Type**: Portable Single Executable
-   **Size**: ~50 MB
-   **Requirements**: None (Self-contained, uses system .NET for extraction)

## Usage
Simply run:
```powershell
.\build.ps1 -BuildSingleExe
```
The output file is ready for distribution.
