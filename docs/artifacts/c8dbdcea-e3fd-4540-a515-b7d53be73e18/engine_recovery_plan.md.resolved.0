# Engine Recovery Plan (Deadlock Resolution)

The "Parallel Production" engine hit a thread deadlock at 1:59 PM. I will resolve the hang and implement a timeout guard to ensure the 86-minute transcode can finish unattended.

## User Review Required

> [!IMPORTANT]
> **Process Kill**: I must terminate the current "Zombie" process. You may see the dashboard briefly go offline.
> **Timeout Guard**: I will add a 5-minute timeout to the I/O thread shutdown. If a thread hangs, the engine will now force-continue to the next chunk rather than waiting forever.

## Proposed Changes

---

### [Component] MuseTalk Inference Script

#### [MODIFY] [inference.py](file:///c:/just-dub-it2/tools/MuseTalk/scripts/inference.py)
*   **Robust Shutdown**: Replace `executor.shutdown(wait=True)` with a timeout-aware wait.
*   **Thread Safety**: Ensure `cv2.imread` and `cv2.imwrite` are wrapped in generic exception handlers within the worker thread to prevent thread crashes from hanging the executor.

---

### [Component] Execution

#### [EXECUTE] The Resurrection
*   `taskkill /F /IM python.exe /T`
*   Restart `launch.bat`.
*   **Verification**: Ensure the chunk-resumption logic skips the finished `ws_000`-`ws_003` folders.

## Open Questions

1.  **Do you want me to reduce the 'Buffer Size'?** (Currently 60 frames, reducing to 30 might save more VRAM for the threads).
