---
description: Code Review & Memory Leak Investigation Workflow
---

# Code Review Workflow

1.  **Identify Hotspots**: Focus on manual memory management (new/delete, malloc/free) and C library interop (FFmpeg, LibRaw).
2.  **Check Loops**: Look for allocations inside loops (`while`, `for`) that overwrite pointers.
    *   *Example*: `ctx = allocate()` inside a loop without `free(ctx)`.
3.  **Verify RAII**: Ensure cleanup structs/classes have valid destructors that handle all members.
    *   *Check*: Are move semantics valid? Is the destructor called on early returns?
4.  **Static Analysis**:
    *   Check for `QObject` parenting.
    *   Check for `std::shared_ptr` usage vs raw pointers.
5.  **Runtime Verification**:
    *   Monitor "RAM App" metric in `SystemMonitor`.
    *   Watch "Active Task Count" in `TaskScheduler` (should return to 0).

# Common Leak Patterns in this Project

-   **FFmpeg**: `AVFrame`, `AVPacket`, `AVCodecContext`, `SwsContext`. Always use `cleanup` structs.
-   **Qt**: `QImage` copies (use `copy()` to detach). `QRunnable` auto-deletion (enabled by default).
-   **QML**: C++ objects exposed to QML must have clear ownership (parented or smart pointers).
