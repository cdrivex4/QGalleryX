# JustDubit Startup Robustness Plan

The objective is to fix the "blank page" issue and ensure the system starts reliably on Windows without port conflicts.

## Proposed Changes

### [Component Name] Startup Infrastructure
Summary: Update the batch script to be "self-healing" and diagnostic.

#### [MODIFY] [start_justdubit.bat](file:///c:/just-dub-it/start_justdubit.bat)
- Add `taskkill` to clear stale `python.exe` and `node.exe` processes (using filters where possible, but defaulting to safe cleanup).
- Add a loop to wait for ports `8000` and `5173` to be LISTENING using `netstat`.
- Redirect API and Webapp output to `logs/startup_api.log` and `logs/startup_web.log`.
- Provide immediate feedback if one service fails to start.

### [Component Name] Workspace Cleanup
#### [DELETE] [scripts/verify_pipeline.py](file:///c:/just-dub-it/scripts/verify_pipeline.py)
Cleaning up temporary diagnostic scripts once the pipeline is confirmed.

## Verification Plan

### Automated Tests
1. Run the new `start_justdubit.bat`.
2. Verify that it correctly identifies if a port is in use and cleans it up.
3. Confirm that the browser only opens *after* the services are responsive.

### Manual Verification
1. User runs `start_justdubit.bat` and observes the terminal for "Service Online" messages.
2. Verify that the web interface loads and the "Weights Ready" banner is displayed.
