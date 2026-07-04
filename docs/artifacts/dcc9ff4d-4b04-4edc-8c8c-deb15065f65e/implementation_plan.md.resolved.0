# Implementation Plan: Disk I/O Benchmarking and Context Restoration

The user has replaced a failing 2TB WD Purple drive with a new 16TB WDC Ultrastar drive (Disk 1, Drive `I:`). We need to re-run the previous `async_io_benchmark.py` on this new hardware to verify its performance and stability, compare it with the previous baseline, and restore the search plugin repair context that was "lost" due to session history issues.

## User Review Required

> [!IMPORTANT]
> The previous "WD Purple" benchmark results in `async_io_results.txt` actually fell back to the System SSD (Drive `C:`) because the WD Purple was already failing and inaccessible at that time. If we have a different set of results for the WD Purple from an earlier session, please provide them. Otherwise, I will compare the new 16TB HDD against the SSD baseline.

## Proposed Changes

### 1. Benchmark Execution
We will run the `async_io_benchmark.py` script specifically targeting the new 16TB drive (`I:\torrentz\`).

#### [MODIFY] [async_io_benchmark.py](file:///c:/Users/curtis/Desktop/gemini_qbittorrent/async_io_benchmark.py)
*   Ensure `TARGET_DIR` is set to `i:\torrentz\torrentscratch_test`.
*   Run the script and capture the output.

### 2. Context Restoration
We will update the `handover.md` and `io_optimization_report.md` to reflect the drive replacement and the latest search plugin status.

#### [MODIFY] [io_optimization_report.md](file:///c:/Users/curtis/Desktop/gemini_qbittorrent/io_optimization_report.md)
*   Add a section for the New 16TB Drive performance.
*   Compare with the old WD Purple failure state and SSD baseline.

#### [MODIFY] [handover.md](file:///c:/Users/curtis/Desktop/gemini_qbittorrent/handover.md)
*   Update the "Hardware Diagnosis" section to mark the WD Purple as replaced.
*   Resume the "Next Steps" for the remaining search plugins.

## Verification Plan

### Automated Tests
*   Run `python async_io_benchmark.py` on drive `I:`.
*   Verify no "Fatal Device Hardware Errors" occur during the test.
*   Check that `Max Latency` stays well below previous failure thresholds (e.g., < 500ms).

### Manual Verification
*   Present the comparison table to the user.
*   Confirm with the user if the "Lost History" context is sufficiently restored.
