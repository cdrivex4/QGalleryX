# Project Cleanup & Consolidation Plan

## Phase 1: Audit & Deprecation (✅ Completed)
Instead of deleting legacy code, all orphaned or duplicate code was explicitly migrated to the `deprecated/` directory to preserve history without polluting the active pipeline.

- [x] **`resources/qml/`**: Safely moved to `deprecated/qml/`.
- [x] **Pre-borking structure**: Moved `single_QGalleryX-Standalone/` and `LMstudiotest/` to `deprecated/`.
- [x] **Root Directory Sweep**: Swept unused scripts (`analyze_perf.py`, etc.), log files, and the `conductor/` folder into `deprecated/`.
- [x] **Fixed Deployment Scripts**: Hard-patched `deploy.ps1` and `increment_build.ps1` to correctly target `qml_legacy` and the new decoupled structures without breaking.

## Phase 2: Build Pipeline Validation (✅ Completed)
- [x] Ensure `appSamsungGallery` builds entirely from `src_legacy/`.
- [x] Ensure `appScrollBench` builds from `src/` and `test_scrollbench/`.
- [x] Ensure all C++ tests point to the active `src/` module.
- [x] Triggered final `.\build.ps1 -Clean`. Result: `BUILD SUCCESS` & `VERIFICATION SUCCESSFUL`.

## Phase 3: Knowledge Graph Sync (⏳ Pending)
- [ ] Run the headless AST extraction (`uv run --all-extras graphify update .`) to sync the map. This ensures Graphify structurally understands the `deprecated/`, `src_legacy/`, and `src/` boundaries and drops the stale nodes. *(Waiting on local AI server to come back online)*.

## Phase 4: Resume The Critical Path: 10k Eviction Pipeline (🎯 Next Up)
Once the workspace is completely sanitized and the graph is refreshed, we immediately resume our core architectural work on `test_scrollbench`:
- [ ] **Tier 0:** Implement VRAM eviction in `FastImageItem.qml`.
- [ ] **Tier 1:** Implement CPU Queue aborts in `ResponseTracker`.
- [ ] **Tier 2:** Implement RAM Cache LRU in `AsyncImageProvider::m_cache`.
- [ ] **Tier 3:** Implement Disk Cache Pruning in `FileCacheManager`.
