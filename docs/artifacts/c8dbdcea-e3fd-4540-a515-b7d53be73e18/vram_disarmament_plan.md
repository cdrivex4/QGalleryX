# Sequential VRAM Disarmament (4GB Safety Plan)

The current pipeline is "Stacking" models in the 1050 Ti's VRAM, which has caused a total deadlock at 100% usage (4.0/4.0 GB). I will transition the engine to a strict **One-In, One-Out** architecture.

## User Review Required

> [!IMPORTANT]
> **Data Safety**: All your segments, translations, and voice files are ALREADY ON DISK. We are not deleting any progress. 
> **Restart Required**: I must kill the "Frozen" process and restart it. It will see that Segments 1-683 are already finished and will jump directly to the Lip-Sync (Stage 6) with a clean, empty GPU.

## Proposed Changes

---

### [Component] Stage Cleanup Logic

#### [MODIFY] [translate.py](file:///c:/just-dub-it2/app/stages/translate.py)
*   Add a `unload_llm()` function.
*   Explicitly call `del llm`, `gc.collect()`, and `torch.cuda.empty_cache()` inside a `finally` block.

#### [MODIFY] [tts.py](file:///c:/just-dub-it2/app/stages/tts.py)
*   Implement the same `unload_tts()` cleanup logic to ensure the GPU is 100% empty before Lip-Sync starts.

---

### [Component] Environment Optimization

#### [EXECUTE] Performance Boost
*   Run `pip install accelerate` to resolve the MuseTalk warning and improve loading efficiency.

---

### [Component] Finalization Resume 

#### [EXECUTE] The 100% Push
*   Restart `launch.bat`.
*   The job will resume at Stage 6 with **3.5GB of Free VRAM**, allowing MuseTalk to "Ignite" and finish the movie.

## Open Questions

1.  **Permission to restart the process?** (It is currently frozen at 100% CPU/VRAM, so a restart is required to apply the new memory-safe logic).
