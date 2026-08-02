# Fix UI Overlays, Print Preview Crashes, and Restored Global View/Filter State

## 🎯 Summary
Resolved multiple regressions and long-standing UI/UX bugs spanning from print preview crashes to text filtering regressions caused by previous architecture rewrites. Enhanced the global view state to seamlessly propagate Semantic/Tiles view modes and Grouping modes even into Album detail views. 

## 🔧 Code Changes
- **Text Filtering & Global View States (`Main.qml`, `AlbumsView.qml`, `GalleryViewSemantic.qml`, `GalleryViewTiles.qml`)**: 
  - Restored `ImageModel { id: imageModel }` to `Main.qml` to fix broken bindings affecting the search field and `isLoading`/`loadedCount` displays.
  - Extracted `useTiles` and `groupingMode` state out of the local gallery tab and into the global `ApplicationWindow` properties.
  - Rewrote `AlbumsView.qml`'s detail content to dynamically use a `Loader` instead of hardcoding `GalleryViewTiles`. Album detail views now fully support toggling between Semantic and Tiles layouts, and adhere to Grouping selections.
  - Implemented smart default models so isolated gallery views maintain functionality without trashing the global `imageModel`.

- **Print Preview Crash (`AsyncImageProvider.cpp`, `PhotoViewer.qml`)**:
  - Fixed a critical thread deadlock/crash in `AsyncImageProvider` during print preview generation by properly separating image requests and managing thread lifecycle.
  - Fixed `PhotoViewer.qml` components lacking required bindings for Print UI execution.

- **Overlays (`StatsOverlay.qml`)**:
  - Fixed z-depth bleed-through issues where the top bar or elements were occluded incorrectly.
  - Added a dedicated close (`X`) button tied to the application's global "show performance stats" settings toggle for quicker dismissal without returning to settings.
