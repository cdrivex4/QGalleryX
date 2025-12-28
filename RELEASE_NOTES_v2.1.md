# Release Notes - 2025-12-25

## Major Fixes
- **Critical Video Memory Leak**: Fixed a `SwsContext` leak in video thumbnail generation that could cause rapid memory bloat during scrolling.
- **Zombie System Process**: Application now cleanly exits when the last window is closed, preventing orphaned background processes.
- **Missing Files**: Fixed folder scanning to correctly find files with uppercase extensions (e.g., `IMG_0001.JPG`, `VIDEO.MP4`).

## Features
- **Enhanced Performance Overlay**: Added detailed system stats to the ScrollBench overlay:
  - App vs System CPU usage
  - GPU Utilization & VRAM usage
  - RAM consumption breakdown
- **Hardware Acceleration**: 
  - Verified D3D11VA video decoding is active and working.
  - Added smart fallback (D3D11 -> OpenCL -> CPU).
  - Explicitly blocked unsafe backends (Vulkan) to prevent system crashes.

## Performance
- **Folder Scanning**: Now supports recursive scanning with 40+ file extensions.
- **Threading**: TaskScheduler optimized for both CPU-bound (decode) and IO-bound (scan/read) tasks.

## Known Issues
- Image & RAW decoding remains CPU-only (by design for stability).
- Vulkan backend is blacklisted due to system instability.
