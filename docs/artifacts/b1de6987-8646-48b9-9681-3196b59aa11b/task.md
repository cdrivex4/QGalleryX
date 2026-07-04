- [x] Fix ScrollBench Selection (Broken)
- [x] Fix Index Mismatch / Wrong Image on Click
- [x] Fix Viewport Load Priority
- [x] Increase RAW/Video Concurrency (+1 in Provider)
- [x] Verify FrameBudgetScheduler (Found default too low, increased)
- [x] Fix Settings Overlay Toggle (Semantic/Grid)
- [x] Add DateScrubber to Album View
- [x] Implement Image Editing in Single Image View (Basic Rotation)
- [x] Project Cleanup (Move redundant files to `_RecycleBin`)

# Task: ScrollBench Improvements
- [x] Integrate MFT Scanner into ScrollBench
    - [x] Update `appScrollBench` CMake target
    - [x] Update `ScrollBenchImageModel` to use `FastVolumeScanner`
- [x] **ScrollBench: Fix Album View** <!-- id: 4 -->
- [x] Integrate MFT Scanner into ScrollBench
    - [x] Update `appScrollBench` CMake target
    - [x] Update `ScrollBenchImageModel` to use `FastVolumeScanner`
    - [x] Verify scanner fallback logic (Non-NTFS/Network)
- [x] **Viewport & Playback Optimization**
    - [x] Implement adaptive task throttling (pause background during video).
    - [x] Hide stale images/video output in PhotoViewer.
    - [x] Verify LIFO priority for visible items.
- [x] **Selection Interaction Fixes** <!-- id: 4 -->
    - [x] Diagnose why `Ctrl+Click` and `Shift+Click` are failing.
    - [x] Repalce TapHandler with MouseArea.
    - [x] Implement centralized performAction.
    - [x] Verified build with `tst_linkage.exe`.
- [/] Port Features to Main App
    - [ ] Update ScrollBench Settings/Stats overlay styles

# Completed Tasks (TDR Fix)
- [x] Analyze memory dump/crash symptoms (BBADBEEF/TDR)
- [x] Reduce `AsyncImageProvider` video concurrency (4 -> 1)
- [x] Port `FrameBudgetScheduler` from ScrollBench to Main App
- [x] Integrate `FrameBudgetScheduler` into `AsyncImageProvider`
- [x] Fix compilation errors in `main.cpp` and `AsyncImageProvider` headers
- [x] Verify Build (Rebuild after syntax fixes)
- [x] Update Documentation (Completed)

# Task: Outstanding Work - Feature Completion

## 🔴 Critical: Missing Core Features (Code Review Recommended)

### 1. Selection Feature
- [ ] **Port selection API from ScrollBench to Main App**
    - [ ] Add selection methods to `src/ImageModel.h/.cpp`
        - [ ] `toggleSelection(int index)`
        - [ ] `selectRange(int start, int end)`
        - [ ] `selectVisualRect(int colMin, colMax, rowMin, rowMax, columns)`
        - [ ] `selectAll()`, `clearSelection()`, `invertSelection()`
        - [ ] `selectedCount()` property
    - [ ] Add `isSelected` field to ImageModel's ImageItem struct
- [ ] **Implement drag-to-select in Grid Views**
    - [ ] Add DragHandler to `GalleryViewSemantic.qml`
    - [ ] Add DragHandler to `GalleryViewTiles.qml`
    - [ ] Visual feedback (selection border/checkbox/highlight)
    - [ ] Calculate grid cell coordinates from drag positions
- [ ] **Implement drag-to-select in Album View**
    - [ ] Add selection to `AlbumsView.qml` when folder is opened
    - [ ] Reuse grid selection logic from gallery views

### 2. Share & Resize Feature
- [x] **Port Share Components from ScrollBench**
    - [x] Copy `ShareDialog.qml` to `resources/qml/` (Available in ScrollBench)
    - [x] Copy `ResizeEditor.qml` to `resources/qml/` (Implemented in ScrollBench)
    - [x] Register in CMakeLists.txt QML module
- [x] **Backend Implementation**
    - [x] Add `getSelectedPaths()` to ImageModel
    - [x] Add `getSelectedTotalSizeBytes()` to ImageModel
    - [x] Implement `resizeImages()` backend (Verified in `ImageProcessor.cpp`)
    - [x] Add file save/export logic (Basic Desktop Save)
- [x] **Selection Interaction Fixes** <!-- id: 4 -->
    - [x] Diagnose why `Ctrl+Click` and `Shift+Click` are failing despite previous changes.
    - [x] Prototype absolute reliable input handling (MouseArea vs TapHandler).
    - [x] Implement `Ctrl+Click` (Toggle) and `Shift+Click` (Range) with visually verifiable tests.
    - [x] Verify selection state persistence during range selection.
- [ ] **Data Model & Backend** <!-- id: 2 -->
    - [x] Fix `openInExplorer` argument passing.
    - [x] Create `tst_linkage` verification tool.
- [ ] **UI Integration**
    - [ ] Add "Share" button to main UI (Gallery context action bar)
    - [ ] Add "Share" button to PhotoViewer (Single image context)
    - [x] Wire Share button to ShareDialog @completed
    - [ ] **Workflow Logic: Resize/Share**
        - [ ] **Single Mode (PhotoViewer)**:
            - If no items selected in Gallery, Share button in Viewer targets `currentIndex`.
            - `ShareDialog` mode set to `Single`.
            - `ResizeEditor` allows interactive preview of the specific image.
        - [ ] **Batch Mode (Gallery Selection)**:
            - If items strictly selected, Share targets `getSelectedPaths()`.
            - `ShareDialog` mode set to `Batch`.
            - `ResizeEditor` shows first image as preview but applies settings to all.
    - [ ] Test single image share from PhotoViewer
    - [ ] Test batch resize with multiple selections

### 3. Image Editing (Standard Images)
- [x] **Implement Rotation**
  - [x] Add `rotateImage` to `ImageProcessor` (backend) @completed
  - [x] Add `rotateImage` to `ScrollBenchImageModel` @completed
  - [x] Add Rotate Left/Right buttons to PhotoViewer toolbar @completed
  - [x] Implement cache busting for rotated images (VersionRole) @completed
- [ ] **Enhance Crop Functionality**
    - [ ] Create `CropOverlay.qml` with draggable handles
    - [ ] Connect to `cropImage()` backend
    - [ ] Implement "Save Copy" (Save As) for cropped images @in-scope
    - [ ] Implement "Save" (Overwrite) @in-scope
- [ ] **Constraints**
    - [x] No generic file copying/moving (Out of Scope) @completed
    - [ ] Disable editing for RAW files (isRaw === true)
    - [ ] Disable editing for videos (isVideo === true)

### 5. Video Playback Verification
- [x] **Implement Video Feedback Overlay**
  - [x] Create visible overlay button/QML in PhotoViewer @completed
  - [x] Add feedback buttons (Black Screen, No Sound, etc.) @completed
  - [x] Log feedback to console/file with metadata @completed
    - [ ] Use this for correlating logs with visual issues
- [ ] Test hardware acceleration is active
- [ ] Test H.264, H.265, AV1 codec support
