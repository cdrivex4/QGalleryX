# Track: Debug Image Loading Issue in ScrollBench

**Objective**: Resolve the issue where `test_scrollbench` requires user interaction to load all images, despite the implemented `FrameBudgetScheduler` and culling logic.

## Plan

1.  **Verify Initial State:**
    *   **Status**: Completed
    *   **Action**: Observed that only 11 images load initially due to early `indexAt` failures in QML before layout is stable.

2.  **Examine `generateTestData` and `scanDirectory` in `ScrollBenchImageModel.cpp`:**
    *   **Status**: Completed
    *   **Action**: Verified that `beginInsertRows`/`endInsertRows` are used. Added `scanComplete` signal emission to trigger post-scan logic.

3.  **Trace `updateVisibleRange()` calls:**
    *   **Status**: Completed
    *   **Action**: Confirmed that `updateVisibleRange` fails initially because QML `indexAt` returns -1 when called too early.

4.  **Trace `requestThumbnail()` calls:**
    *   **Status**: Completed
    *   **Action**: Verified that `requestThumbnail` is only called for the range returned by `indexAt`.

5.  **Review `MainScrollBench.qml` viewport logic:**
    *   **Status**: Completed
    *   **Action**: Added robustness to `updateVisibleRange()` with fallbacks and a `Connections` object to handle a delayed `forceUpdateGridView` signal from C++.

6.  **Analyze QML `Image` element bindings:**
    *   **Status**: Completed
    *   **Action**: Confirmed bindings are correct; the issue was the initial range calculation.

7.  **Consider Initial Load Trigger:**
    *   **Status**: Completed
    *   **Action**: Implemented a 200ms `m_forceUpdateTimer` in `ScrollBenchImageModel` that emits `forceUpdateGridView` after `scanComplete`. This ensures QML has time to perform its initial layout before we probe the visible range.
