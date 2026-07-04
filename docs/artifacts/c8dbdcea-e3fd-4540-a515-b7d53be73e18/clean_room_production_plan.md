# Clean Room Production Plan (VRAM Ceiling Bypass)

Your 1050 Ti is currently pinned at 4.0/4.0 GB VRAM because of "Resource Sharing" with background apps (Chrome, Docker, PowerToys). This causes a "Thrashing" state where the CPU hits 100% in a busy-wait loop while the GPU stalls.

I will implement **Extreme Offloading** to reclaim enough "Fresh Air" for the transcode to move.

## User Review Required

> [!IMPORTANT]
> **External App Interference**: For 86-minute content on 4GB hardware, **you MUST close Chrome and Docker.** These apps together steal ~1.5GB of the card, leaving MuseTalk zero room to "breathe".
> **Dynamic Loading Patch**: I am modifying the AI boot sequence to be even more aggressive.

## Proposed Changes

---

### [Component] MuseTalk Extreme Offloading

#### [MODIFY] [inference.py](file:///c:/just-dub-it2/tools/MuseTalk/scripts/inference.py)
*   **Lazy Whisper Loading**: Move the `WhisperModel.from_pretrained` call *inside* the task loop, after frame extraction is complete.
*   **Preprocessing isolation**: During frame extraction (landmark detection), the VRAM will ONLY hold the VAE. The UNet and Whisper will be Loaded/Unloaded only when needed for the actual "lip-painting" frames.

---

### [Component] Production Refresh 

#### [EXECUTE] The Cold Start
*   Kill all Python processes once more.
*   Apply the Lazy-Loading patch.
*   **User Action**: Close Chrome and other high-VRAM background tasks.
*   Restart `launch.bat`.

## Open Questions

1.  **Are you able to close Chrome and Docker during this final render?** (This is the most effective fix for your hardware).
