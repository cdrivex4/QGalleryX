# Hyper-Granular Migration Plan: QGalleryX

This document provides a comprehensive, file-by-file roadmap for renaming the project from "antigravity" and "Samsung Gallery Clone" to its new official name: **QGalleryX**.

## 1. Staging Phase (Safety First)
Before any code changes are made:
1.  **Commit the Plan**: This document will be saved to `docs/NAME_MIGRATION_PLAN.md` and pushed to GitHub.
2.  **Tag the Current State**: A git tag `pre-rename-antigravity` has been created.
3.  **AI Context Link**: We will create a `docs/AI_RESUME_TICKET.md` that contains the current Brain ID and Conversation ID. This ensures that even if the folder is renamed, future AI sessions can "find" the old history.

## 2. GitHub Migration
1.  **Rename Repository**: Rename to `QGalleryX`.
2.  **Update Remote URL**: After renaming, the local git remote must be updated:
    `git remote set-url origin https://github.com/cdrivex4/QGalleryX.git`

## 3. Global Find-and-Replace Mappings
To ensure consistency across the codebase, we will perform the following mappings:
- `antigravity` → `QGalleryX` (Case-insensitive where appropriate)
- `Samsung Gallery Clone` → `QGalleryX`
- `appSamsungGallery` → `QGalleryX` (For binaries and target names)
- `appScrollBench` → `QGalleryX-Bench` (For performance binaries)

## 4. Comprehensive File List & Planned Changes

| Category | File Path | Old Value | New Value |
| :--- | :--- | :--- | :--- |
| **Build System** | `CMakeLists.txt` | `project(SamsungGallery)` | `project(QGalleryX)` |
| | `CMakeLists.txt` | `add_executable(appSamsungGallery ...)` | `add_executable(QGalleryX ...)` |
| | `CMakeLists.txt` | `URI SamsungGallery` | `URI QGalleryX` |
| | `test_scrollbench/CMakeLists.txt` | `project(ScrollBench)` | `project(QGalleryX-Bench)` |
| | `test_scrollbench/CMakeLists.txt" | `add_executable(appScrollBench ...)` | `add_executable(QGalleryX-Bench ...)` |
| | `test_scrollbench/CMakeLists.txt" | `add_executable(appScrollBenchNet ...)` | `add_executable(QGalleryX-NetWatch ...)` |
| | `test_scrollbench/CMakeLists.txt" | `URI ScrollBench` | `URI QGalleryX-Bench` |
| | `build.ps1` | `$ExeName = "appSamsungGallery.exe"` | `$ExeName = "QGalleryX.exe"` |
| | `deploy.ps1` | `SamsungGallery` | `QGalleryX` |
| **Source Code** | `src/main.cpp` | `setApplicationName("SamsungGallery")` | `setApplicationName("QGalleryX")` |
| | `src/main_test.cpp` | `SamsungGalleryTest` | `QGalleryX-Test` |
| | `resources/qml/Main.qml` | `title: "Samsung Gallery Clone"` | `title: "QGalleryX"` |
| | `resources/qml/MainSemantic.qml` | `Samsung Gallery` | `QGalleryX` |
| **Tests & Utils** | `CMakeLists.txt` | `tst_imagemodel` | `verify-imagemodel` |
| | `CMakeLists.txt` | `tst_scheduler` | `verify-scheduler` |
| | `test_scrollbench/...` | `tst_linkage` | `verify-linkage` |
| | `test_scrollbench/...` | `tst_automation` | `verify-automation` |
| **Documentation** | `README.md` | `Samsung Gallery Clone` | `QGalleryX` |
| | `RELEASE_NOTES.md` | `v2.x` | `v3.0 (QGalleryX Migration)` |
| | `docs/README.md` | `antigravity` | `QGalleryX` |
| | `docs/resume/*.md` | `antigravity` | `QGalleryX` |
| **Scripts** | `scripts/dev_watcher.ps1` | `antigravity` | `QGalleryX` |
| | `scripts/run_automation_suite.ps1` | `antigravity` | `QGalleryX` |
| | `scripts/verify_folder.ps1` | `antigravity` | `QGalleryX` |
| | `run_test_2min.ps1` | `antigravity` | `QGalleryX` |

## 5. Reversion Strategy (Rollback)
In case of a failure:
1.  **Immediate Git Reset**: `git reset --hard pre-rename-antigravity` (Restores all files).
2.  **GitHub Reversion**: Rename the repo back to `antigravity`.
3.  **URL Reversion**: `git remote set-url origin https://github.com/cdrivex4/antigravity.git`

## 6. AI Context Preservation (CRITICAL)
To prevent "Google Antigravity" (this AI) from losing context when the folder is renamed:
1.  **Stay in current session**: Do not start a new chat until the re-link is verified.
2.  **The Resume Ticket**: I will create `docs/AI_RESUME_TICKET.md` with:
    - **Brain ID**: `84bcdcfa-db43-4495-9499-8c61a3eafd31`
    - **Project Origin**: `antigravity`
    - **Target Name**: `QGalleryX`
3.  **Manual Re-hydrate**: If you start a new chat in the renamed folder, simply say: *"Read docs/AI_RESUME_TICKET.md and load that brain ID."*

## 7. Verification Checklist
- [ ] `.\build.ps1` produces `QGalleryX.exe`.
- [ ] `.\test_scrollbench\build.ps1` produces `QGalleryX-Bench.exe`.
- [ ] All 20+ documentation files updated correctly.
- [ ] App window title shows "QGalleryX".
- [ ] `docs/AI_RESUME_TICKET.md` created and verified.
