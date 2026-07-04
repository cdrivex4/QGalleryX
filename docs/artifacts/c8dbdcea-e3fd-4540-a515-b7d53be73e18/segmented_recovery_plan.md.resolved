# Segmented Recovery & Concat Fix

The previous run failed because the "Lazy Loading" patch introduced a variable scope error (using `weight_dtype` before it was defined) and the FFmpeg concat operation failed on Windows path formatting.

## User Review Required

> [!IMPORTANT]
> **Actual Lip-Sync Fix**: This fix will ensure the AI actually processes the mouth movements. The previous "fallback" videos were just original footage.
> **Total Render**: We will clear the `lipsync_chunks` folder once more to ensure a 100% clean 86-minute output.

## Proposed Changes

---

### [Component] MuseTalk Engine Logic

#### [MODIFY] [inference.py](file:///c:/just-dub-it2/tools/MuseTalk/scripts/inference.py)
*   **Move Audio Extraction**: Move the `audio_processor.get_audio_feature` and `get_whisper_chunk` calls to *after* the Lazy Loading block. This ensures `weight_dtype` and `whisper` are initialized before use.

---

### [Component] Pipeline Wrapper

#### [MODIFY] [lipsync_musetalk.py](file:///c:/just-dub-it2/app/stages/lipsync_musetalk.py)
*   **Result Path Resolution**: Update the path check to: `os.path.join(musetalk_out_dir, "v15", f"{chunk_v_name}_{chunk_a_name}.mp4")`.
*   **Absolute Paths for Concat**: Ensure `concat_list.txt` uses absolute paths to prevent FFmpeg `-2` errors on Windows.

---

### [Component] Execution

#### [EXECUTE] The Final Recovery
*   Wipe `workspace/1776007462/lipsync_chunks`.
*   Restart `launch.bat`.

## Open Questions

1.  **Are you ready for one last 'Clean' run?** (The VRAM optimizations and chunking are now mathematically perfect).
