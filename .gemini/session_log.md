# Session Logs: 2025-12-25

## Part 1: Hardware Acceleration (Safety Guards)

### Summary
Added comprehensive safety guards to `HardwareAccelerationManager` to prevent system crashes while maintaining functionality. Implemented smart fallback sequence with CPU feature detection.

### Key Actions:
1.  **Safety Guards**: Blacklisted Vulkan (confirmed system crash), but allowed OpenCL with fallback.
2.  **Smart Fallback Sequence**: D3D11VA → OpenCL → CPU (None)
   - Each backend is tested automatically
   - Graceful degradation to software decoding
3.  **CPU Feature Detection**: Added runtime detection of SSE/AVX extensions for logging.
4.  **Error Recovery**: All failures automatically fall back to next option instead of crashing.
5.  **Testing**: Verified D3D11VA works correctly on your system.

---

## Part 2: Video Playback & Thumbnail Debugging

### Summary
Resolved critical bugs where the same video would play regardless of selection and fixed broken video thumbnails. Hardened the `AsyncImageProvider` with better cancellation and cache management.

### Key Actions:
1.  **Video Playback Fix**:
    - Implemented `onVisibleChanged` and delegate-level `onFilePathChanged` guards in `PhotoViewer.qml` to stop stale media and reset state.
    - Synchronized `ListView.currentIndex` with the root viewer state to prevent "stuck" indices.
2.  **Thumbnailer Refactor**:
    - Cleaned up `VideoThumbnailer.cpp` to properly handle GPU-to-CPU transfers and scale frames correctly.
    - Added a luma-check to skip black frames during extraction.
3.  **Stability & Performance**:
    - Updated `AsyncImageProvider` to pass cancellation tokens to worker tasks, preventing background work from clogging the queue for closed views.
    - Increased memory cache limit to 512MB for smoother scrolling.
    - Fixed an `ImageModel` bug where incremental updates caused UI jumps during initial scans.

### Current State:
*   **Build**: ✅ Passing
*   **Features**:
    *   Video Playback: ✅ Correct media plays; no stale audio.
    *   Thumbnails: ✅ Generating correctly with cancellation support.
    *   HW Acceleration: ✅ Safe with fallback
