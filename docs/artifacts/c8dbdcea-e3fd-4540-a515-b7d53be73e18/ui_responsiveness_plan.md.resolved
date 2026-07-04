# UI Responsiveness & Telemetry Optimization Plan

We have discovered that `nvidia-smi` takes **~4 seconds** to execute on your hardware. Because the UI was set to refresh every **2 seconds**, the system was constantly "choking" on overlapping, blocking hardware queries.

## User Review Required

> [!IMPORTANT]
> **Telemetry Accuracy**: To fix the lag, I am moving the VRAM display to a **5-second cache**. This means the "VRAM Load" bar in the UI will update every 5 seconds instead of every 2, but the interface will become **100% responsive** and snappy again.

## Proposed Changes

---

### [Component] Hardware Utilities

#### [MODIFY] [cuda_utils.py](file:///c:/just-dub-it2/app/utils/cuda_utils.py)
*   Implement a background `threading.Thread` that polls `nvidia-smi` every 5-10 seconds.
*   Update `get_vram_info()` to return the last known cached values instantly.
*   This removes the 4-second "blocking" penalty from the UI loop.

---

### [Component] Dashboard Interface

#### [MODIFY] [main_ui.py](file:///c:/just-dub-it2/app/main_ui.py)
*   **[UPDATE] Refresh Intervals**: 
    *   Change `vram_gauge` from `every=2` to `every=5`.
    *   Change `stat_box` from `every=2` to `every=5`.
*   This drastically reduces the data-synchronized pressure on the Gradio websocket.

---

### [Component] System Recovery

#### [EXECUTE] Clean Restart
*   Forcibly terminate all existing Python processes (Zombie cleanup).
*   Restart the engine and verify that Tab switching and the "Pause" button are now instantaneous.

## Verification Plan

### Automated Tests
*   Run the latency profiler again to verify `get_vram_info()` now returns in `< 0.001s`.

### Manual Verification
*   Verify that navigating between the "Production Hub" and "Settings" tabs is smooth and immediate.
*   Confirm the "Persistent Save" button provides instant feedback.
