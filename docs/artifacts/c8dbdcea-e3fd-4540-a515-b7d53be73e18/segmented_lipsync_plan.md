# Segmented Lip-Sync Ignition (Feature-Length Bypass)

The 86-minute movie has hit a "Memory Wall" at the 32-minute mark (frame 46,761). On 4GB hardware, the landmark dictionary and frame-mapping table become too large to hold in VRAM. I will transition to a **Segmented Relay** strategy.

## User Review Required

> [!IMPORTANT]
> **Segmented Processing**: I will be splitting the final render into 5-minute blocks. This is the industrial standard for processing feature-length AI dubs on consumer hardware.
> **FFmpeg Merge**: There will be one final "Stitching" phase at the end. Your high-performance audio mix is already safe.

## Proposed Changes

---

### [Component] Pipeline Segmentation

#### [MODIFY] [pipeline.py](file:///c:/just-dub-it2/app/pipeline.py)
*   **Lipsync Logic Update**: Instead of passing the whole 86-minute video to MuseTalk, I will slice it into 5-minute `.mp4` chunks (using `-c copy` for speed).
*   **Relay Loop**: Iterate through chunks, calling `run_lipsync_stage` for each.
*   **Final Concatenation**: Use FFmpeg `concat` to join the processed chunks.

---

### [Component] MuseTalk Optimizer

#### [MODIFY] [lipsync_musetalk.py](file:///c:/just-dub-it2/app/stages/lipsync_musetalk.py)
*   Update to handle shorter, faster inference jobs.
*   Ensure temporary directories are cleared between chunks to prevent SSD overflow.

---

### [Component] Production Resume 

#### [EXECUTE] The Final Pull
*   The system will "Resume" from the 0-minute mark but skip the landmark detection for the already-processed 32 minutes (if cached).
*   The relay will then continue past the 32-minute mark and finish the 86-minute movie.

## Open Questions

1.  **Chunk Size**: Is **5 minutes** acceptable, or would you prefer shorter (**2 minutes**) for maximum stability? (5 minutes is faster but uses ~3GB VRAM).
