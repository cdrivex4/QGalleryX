# Track: Technical Debt & Stability

**Objective**: Resolve outstanding bugs, clean up project clutter, and improve general application stability as outlined in `docs/resume/OUTSTANDING_TASKS.md`.

## Plan

1.  **Fix Startup Null Reference (BUG-001)**
    *   **Context**: `Main.qml` accesses `appSettings` before it's fully ready, causing a console warning.
    *   **Action**: Add a safe null check to the binding.
    *   **Status**: COMPLETED

2.  **Clean Up Crash Logs (BUG-003)**
    *   **Context**: The root directory is cluttered with `crash_log_*.txt` files.
    *   **Action**: Move existing logs to `logs/` and update `LogManager` to write there by default.
    *   **Status**: COMPLETED

3.  **Fix Hardcoded Paths (BUG-002)**
    *   **Context**: Code contains hardcoded references to "I:/MY SDCards/".
    *   **Action**: Replace with `StandardPaths` or user-selectable locations.
    *   **Status**: VERIFIED (No hardcoded paths found in current source code)
