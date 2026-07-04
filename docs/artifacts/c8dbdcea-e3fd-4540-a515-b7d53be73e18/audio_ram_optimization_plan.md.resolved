# Audio RAM Optimization Strategy (86-Minute Scale)

The current audio mixing engine is designed for short clips. At the 1-hour scale, it attempts to load several gigabytes of raw PCM data into memory, causing the system to stall. 

I will transition the engine to a **Streaming FFmpeg** architecture.

## User Review Required

> [!IMPORTANT]
> **Data Integrity**: I am NOT deleting any translations or voice files. I am only changing the "Glue" used to combine them.
> **Restart Required**: I must stop the current "Frozen" process and restart it. It will immediately resume at the mix stage.

## Proposed Changes

---

### [Component] Audio Mix Engine (FFmpeg Stream)

#### [MODIFY] [audio_mix.py](file:///c:/just-dub-it2/app/stages/audio_mix.py)
*   **Remove Pydub Bottleneck**: Stop using `AudioSegment` for the 1-hour timeline.
*   **Implement 'Concat-File' Logic**: 
    1.  Generate a temporary `ffmpeg_concat.txt` containing the paths and timestamps for all 683 clips.
    2.  Execute a single `ffmpeg` command to overlay the background and the vocal list.
*   **Efficiency**: This move reduces RAM footprint from ~4GB to ~10MB.

---

### [Component] Finalization

#### [EXECUTE] Production Resume
*   Restart the server.
*   **Job 1776007462** will detect Stage 5 is complete.
*   Stage 6 (the new Mix engine) will trigger and finish in roughly 30-60 seconds.

## Open Questions

1.  **Permission to restart the process?** (Since it's currently at 99% but effectively deadlocked, a restart is the only way to trigger the new efficient code).
