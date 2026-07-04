# Multi-Phase Implementation Plan: 10k Architecture & Project Restoration

This roadmap breaks down the massive undertaking of restoring the legacy application, protecting the build pipeline, and transforming `test_scrollbench` into a fully working, 10k-image capable production application.

---

## Phase 1: Structural Cleanup & Legacy Decoupling
*The goal of this phase is to untangle the codebases so we can safely experiment without breaking what already works.*

**Tasks:**
1.  **Extract Legacy Core:** Retrieve the exact state of `src/` and `resources/qml/` from commit `33419c0` (v2.1.0).
2.  **Folder Reorganization:** 
    *   Create a `src_legacy/` folder to hold the frozen v2.1.0 code.
    *   Create a `qml_legacy/` folder for the old UI.
    *   Maintain the active, experimental code in `src/` and `test_scrollbench/qml/`.
3.  **Documentation Sync:** Update `ARCHITECTURE.md` and `SCROLLBENCH_STRATEGY.md` to formally document `src_legacy/` as the frozen reference architecture.

**Expected Result:** A clean physical separation of files. The legacy app code is quarantined, leaving `src/` free for aggressive modification.

---

## Phase 2: Build Pipeline Refactoring & Validation
*The goal here is to rewire CMake so the automated tests (`build.ps1` and `tst_linkage.exe`) pass flawlessly.*

**Tasks:**
1.  **CMake Target Isolation:**
    *   Rewrite `CMakeLists.txt` so `appSamsungGallery.exe` compiles strictly against `src_legacy/` and `qml_legacy/`.
    *   Ensure `appScrollBench.exe` compiles against the active `src/` and `test_scrollbench/qml/`.
2.  **Linkage Fixes:** Resolve any `#include` path conflicts or Qt MOC generation errors caused by the folder move.
3.  **Validation:** Run `build.ps1 -Clean`.

**Expected Result:** A green build pipeline. We will have two independent executables that share zero internal state, proving the decoupling is safe.

---

## Phase 3: The 10k Eviction Pipeline Implementation
*With the build protected, we return to active development on `test_scrollbench` to implement the multi-tiered memory architecture.*

**Tasks:**
1.  **Tier 0 (VRAM Eviction):** Implement deterministic texture destruction in `FastImageItem.qml` when delegates exit the `ScrollBenchImageModel` buffer zone.
2.  **Tier 1 (CPU Queue Aborting):** Wire `ResponseTracker` deep into `TaskScheduler`. If a delegate is destroyed, any pending LibRaw/FFmpeg decode task in the queue must immediately abort.
3.  **Tier 2 (RAM LRU Limit):** Enforce strict `setMaxCost()` limits on `AsyncImageProvider::m_cache`.
4.  **Tier 3 (Disk Cache Management):** Implement the `FileCacheManager` background thread to prune the `%LOCALAPPDATA%` thumbnail cache when it exceeds 1GB.

**Expected Result:** `appScrollBench.exe` can scroll through 10,000 items rapidly. Memory stays completely flat. The UI never drops below 60 FPS.

---

## Phase 4: ScrollBench Feature Completion & Redundancy Cleanup
*The final phase bridges the gap from "Test Bench" to "Fully Working Application", while deleting old hacks.*

**Tasks:**
1.  **Redundancy Purge:** 
    *   Remove legacy CPU-throttling hacks that were attempting (and failing) to solve the memory leak.
    *   Remove redundant double-increment logic in `DriveConcurrencyGuard`.
2.  **Feature Porting:** Ensure the Album View, Date Scrubber, and Semantic Zoom logic are fully wired to the new 4-tier eviction pipeline.
3.  **Stall Recovery:** Implement the `checkStalls()` watchdog timer to rescue any rogue I/O locks on network drives.

**Expected Result:** ScrollBench completely eclipses the legacy app in performance and stability, becoming the new defacto `Main App` moving forward.
