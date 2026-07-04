# Interactive Scene Auditor & Production Stabilization Plan (Final)

This plan finalizes the "Interactive Auditor" and "Fast Dub" features. It addresses the 20-hour lipsyncing bottleneck by implementing a per-job toggle while keeping data-rich lipsyncing as the default production standard.

## User Review Required

> [!IMPORTANT]
> **Environment Reset**: I will now proceed to kill the zombie Python process (PID 18244). I will then relaunch the Studio exclusively through the **VirtualEnv** (`env\Scripts\python.exe`) to resolve all "Module Not Found" errors.

> [!TIP]
> **Per-Job Toggle**: The "Skip Lipsync" option will be available on the dashboard. Jobs will default to "Lipsync ON," but you can flip the switch before clicking "Begin" for an instant high-resolution dub.

## Proposed Changes

### 1. UI Refinements
#### [MODIFY] [main_ui.py](file:///c:/just-dub-it2/app/main_ui.py)
*   **Timeline Sliders**: 
    - `Preview Start`: Slider (0s to Total Duration).
    - `Preview Duration`: Slider (1s to 300s, Default: **30s**).
*   **Production Toggle**: Add `gr.Checkbox(label="Enable Lipsync (MuseTalk)", value=True)`.
*   **Master Linkage**: Dropdown to select workspace `1776007462` as the data source for Auditor scene inheritance.

### 2. Pipeline Orchestration
#### [MODIFY] [pipeline.py](file:///c:/just-dub-it2/app/pipeline.py)
*   **Job Metadata**: Add `enable_lipsync` to the `Job` class (default: `True`).
*   **Execution Branch**: In `_run_translation_job`, wrap the Lipsync stage in an `if job.enable_lipsync:` block.
*   **Auditor Inheritance**: If the Auditor is triggered, automatically copy `translated_segments.json` (offset-calculated) from the selected Master workspace.

### 3. MuseTalk Pathing Fixes
#### [MODIFY] [lipsync_musetalk.py](file:///c:/just-dub-it2/app/stages/lipsync_musetalk.py)
*   **PYTHONPATH**: Ensure `tools/MuseTalk` is prepended to the environment on every run.
*   **Asset Correction**: Point the UNet and FaceParsing configs to the absolute paths in `models/` verified during research.

## Verification Plan

### Automated Tests
1.  **Toggle Logic**: Launch a job with `enable_lipsync=False` and verify it hits 100% immediately after the Audio Mix.
2.  **Slider Scaling**: Confirm the "Preview Start" slider correctly adjusts its maximum value when a long video (86 mins) is loaded.

### Manual Verification
1.  **86-Minute Audit**: Select a 30s clip from the feature film, link it to `1776007462`, and confirm it "inherits" the translations instantly.
2.  **Environment Check**: Verify that `import musetalk` no longer fails when the worker thread starts.
