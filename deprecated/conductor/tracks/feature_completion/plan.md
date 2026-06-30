# Track: Feature Completion

**Objective**: Implement core missing features as outlined in `docs/resume/OUTSTANDING_TASKS.md`.

## Plan

1.  **Implement Albums View (FEAT-001)**
    *   **Context**: `AlbumsView.qml` was a placeholder.
    *   **Action**: 
        *   Created `AlbumCard.qml` for individual album display.
        *   Refactored `AlbumsView.qml` to use `AlbumCard`, added better empty states and navigation.
        *   Verified connection to `AlbumModel`.
    *   **Status**: COMPLETED

2.  **Implement Video Playback (FEAT-002)**
    *   **Context**: Videos show thumbnails but don't play.
    *   **Action**: Improved `MediaPlayer` integration in `PhotoViewer.qml` and fixed file path resolution.
    *   **Status**: COMPLETED (Basic playback implemented)

3.  **Implement Share/Resize Dialog (FEAT-003)**
    *   **Context**: The `ShareDialog.qml` and `ResizeEditor.qml` exist but contain only UI placeholders.
    *   **Action**: The backend logic for resizing and sharing images needs to be implemented and connected to the QML front-end.
    *   **Status**: PLACEHOLDER (UI Only)