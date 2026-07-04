# Selection Interaction Fix Plan

## Goal
Restore standard desktop file selection behavior (`Ctrl+Click` to toggle, `Shift+Click` to range select) which is currently unreliable in the QML views.

## User Review Required
> [!IMPORTANT]
> **Switching to MouseArea**: `TapHandler` in Qt Quick can be finicky with modifier keys on some platforms or focus scopes. We will revert the item delegate interaction to `MouseArea` which has historically proven more reliable for proper mouse button + modifier handling in desktop scenarios.

## Proposed Changes

### QML Layer (`GalleryViewScrollBench.qml` & `GalleryViewSemanticScrollBench.qml`)

#### [MODIFY] Interaction Delegate
- Replace `TapHandler` with `MouseArea`.
- **Reason**: `MouseArea` provides the `mouse` event object which explicitly contains `modifiers` in `onClicked` and `onPressed`, avoiding reliance on global state or focus issues.
- **Logic**:
  - `onClicked`:
    - Check `mouse.modifiers & Qt.ControlModifier` -> `ToggleSelect`.
    - Check `mouse.modifiers & Qt.ShiftModifier` -> `RangeSelect`.
    - Else -> `Open` (Exclusive Select).

#### [MODIFY] `performAction`
- Ensure `RangeSelect` uses a valid `lastSelectedIndex`.
- If `lastSelectedIndex` is -1 (invalid), treat `Shift+Click` as a normal selection start.

### Backend (`ScrollBenchImageModel.cpp`)
- Verify `selectRange(start, end)` logic handles:
  - Backward ranges (start > end).
  - Forward ranges (start < end).
  - Clearing existing selection *unless* `Ctrl` is held (this is usually handled by the UI decision, but the backend must support additive range selection if needed, though standard Shift+Click usually resets selection to the range).

## Verification Plan

### Manual Verification
1. **Single Click**: Selects item, deselects others.
2. **Ctrl + Click**: Toggles item, preserves others.
3. **Shift + Click**: Selects all items between `lastSelected` and `current`.
4. **Ctrl + Shift + Click**: (Advanced) Adds range to existing selection.

### Automated Test
- The `tst_linkage` only checks backend existence. We will rely on manual verification for the UI interaction feel.
