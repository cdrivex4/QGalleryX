# Granular Migration Plan: QGalleryX

This document outlines the systematic process for renaming the project from "antigravity" and "Samsung Gallery Clone" to its new official name: **QGalleryX**.

## 1. Staging Phase (Safety First)
Before any code changes are made:
1.  **Commit the Plan**: This document will be saved to `docs/NAME_MIGRATION_PLAN.md` and pushed to GitHub.
2.  **Tag the Current State**: A git tag `pre-rename-antigravity` will be created to allow for an instant, foolproof rollback.

## 2. GitHub Migration
1.  **Rename Repository**: Rename to `QGalleryX`.
2.  **Update Remote URL**: After renaming, the local git remote must be updated:
    `git remote set-url origin https://github.com/[username]/QGalleryX.git`

## 3. Granular File Changes

### Tier 1: Core Build System (Critical Path)
*   **CMakeLists.txt**:
    - `project(SamsungGallery)` → `project(QGalleryX)`
    - `add_executable(appSamsungGallery ...)` → `add_executable(QGalleryX ...)`
*   **build.ps1**:
    - `$ExeName = "appSamsungGallery.exe"` → `$ExeName = "QGalleryX.exe"`
    - Update kill list to include both old and new names during transition.

### Tier 2: Branding & Documentation
*   **README.md**:
    - Update title, description, and internal GitHub links.
*   **RELEASE_NOTES.md**:
    - Add a special entry for v3.0 (QGalleryX Migration).
*   **resources/qml/Main.qml**:
    - `title: qsTr("Samsung Gallery Clone")` → `title: qsTr("QGalleryX")`
*   **src/main.cpp**:
    - `QCoreApplication::setApplicationName("SamsungGallery")` → `QCoreApplication::setApplicationName("QGalleryX")`

### Tier 3: Secondary Docs
*   Update all files in `docs/` and `docs/resume/` using automated find-and-replace for consistency.

## 4. Reversion Strategy (Rollback)
In case of a catastrophic build failure or broken dependencies:
1.  **Immediate Git Reset**: `git reset --hard pre-rename-antigravity`
2.  **GitHub Reversion**: Rename the repo back to `antigravity` and revert the remote URL locally.
3.  **Clean Build**: Delete the `build/` directory and re-run `cmake`.

## 5. Verification Checklist
- [ ] `.\build.ps1` produces `QGalleryX.exe` and it launches correctly.
- [ ] `test_scrollbench` produces `QGalleryX-Bench.exe`.
- [ ] `docs/` links are valid for the new GitHub URL.
- [ ] All UI titles reflect the new name.
