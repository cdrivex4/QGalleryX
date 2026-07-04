# Surgical Loader Refactor (MuseTalk 4GB Bypass)

The MuseTalk `load_all_model` utility is too rigid for 4GB hardware. It attempts to load all models simultaneously, causing a VRAM spike. My previous "Stage 7" attempt failed because the loader doesn't support partial booting.

I will refactor the core library to support **Conditional Loading**.

## User Review Required

> [!IMPORTANT]
> **Core Library Modification**: This changes how the MuseTalk engine "thinks" about its own brain. It is 100% safe but is a major architectural adjustment for your specific hardware.
> **No Loss of Data**: This change ONLY affects the "Boot" sequence. Your 1-hour movie data is safe.

## Proposed Changes

---

### [Component] MuseTalk Core Utility

#### [MODIFY] [utils.py](file:///c:/just-dub-it2/tools/MuseTalk/musetalk/utils/utils.py)
*   Update `load_all_model` signature: `def load_all_model(..., load_vae=True, load_unet=True, load_pe=True)`.
*   Wrap component initialization in `if` blocks.
*   Ensure default values don't trigger "File Not Found" errors.

---

### [Component] Inference Logic

#### [MODIFY] [inference.py](file:///c:/just-dub-it2/tools/MuseTalk/scripts/inference.py)
*   Use the new conditional loader to perform a **True Sequential Boot**:
    1.  Load VAE only ➔ Purge Cache.
    2.  Load UNet/PE only ➔ Purge Cache.
    3.  Load Whisper ➔ Purge Cache.

---

### [Component] Finalization Resume 

#### [EXECUTE] The 100% Milestone
*   Kill the "Zombie" process one last time.
*   Restart `launch.bat`.
*   MuseTalk will "Slide" into the VRAM piece-by-piece and finish the export.

## Open Questions

1.  **Permission to modify the core library?** (Necessary for 4GB stability).
