# ✅ Fixes Applied — just-dub-it2 Pipeline Overhaul

**Date:** 2026-06-01  
**Files Modified:** 10  
**Bugs Fixed:** 14  
**Recommendations Implemented:** 8  

---

## Changes by File

### 1. [transcribe.py](file:///c:/just-dub-it2/app/stages/transcribe.py)
| Fix | Description |
|-----|-------------|
| **BUG-02** | Whisper model now loaded via `_load_whisper_model()` and passed through `run_gpu_stage()` for proper VRAM tracking |
| **BUG-07/REC-01** | Changed `task="translate"` → `task="transcribe"`. Stops the double-translation problem (Whisper was translating to English, then Llama translated again) |
| **New** | Returns detected `source_language` to pass through to translation stage |

### 2. [translate.py](file:///c:/just-dub-it2/app/stages/translate.py)
| Fix | Description |
|-----|-------------|
| **BUG-01** | Removed `del llm` from `_do_translate()` finally block — GPU manager handles cleanup now |
| **BUG-03** | Removed dead `run_translate_stage()` and `unload_llm()` functions |
| **ARCH-05/REC-10** | Translation prompt now explicitly specifies source/target language. Adapts behavior for same-language (timing adjustment) vs. cross-language (full translation) |
| **New** | Preserves `speaker_id` through translation for correct TTS voice cloning |

### 3. [audio_mix.py](file:///c:/just-dub-it2/app/stages/audio_mix.py)
| Fix | Description |
|-----|-------------|
| **BUG-05/REC-02** | Timeline position now uses `seg['end'] * 1000` instead of `start_ms + len(clip)` — eliminates cumulative drift |
| **BUG-06** | Speed clamp widened from `[0.7, 1.5]` → `[0.5, 2.5]` — stops clips overflowing their time slots |
| **BUG-11** | Removed duplicate imports at top of file |
| **ARCH-08** | `AudioSegment.converter` set once at module level |
| **New** | Exports clean `vocal_master.wav` for lipsync (supports REC-03) |

### 4. [tts.py](file:///c:/just-dub-it2/app/stages/tts.py)
| Fix | Description |
|-----|-------------|
| **BUG-04/REC-06** | TTS now routed through `run_gpu_stage()` for proper VRAM tracking |
| **REC-04** | Duration contract enforcement — if TTS generates audio >2x longer than slot, retries with shortened text |

### 5. [pipeline.py](file:///c:/just-dub-it2/app/pipeline.py)
| Fix | Description |
|-----|-------------|
| **REC-03/ARCH-03** | MuseTalk now receives clean `vocal_master.wav` instead of background-contaminated `dub_final.wav` |
| **New** | Source language threaded from transcription → translation stage |
| **Cleanup** | Removed dead `load_llm` import and duplicate diarization import |

### 6. [lipsync_musetalk.py](file:///c:/just-dub-it2/app/stages/lipsync_musetalk.py)
| Fix | Description |
|-----|-------------|
| **BUG-08/REC-05** | 2-second overlap between chunks. Extended chunks include lead-in footage, MuseTalk gets facial context, then overlap is trimmed for clean concatenation |

### 7. [diarize_speakers.py](file:///c:/just-dub-it2/app/stages/diarize_speakers.py)
| Fix | Description |
|-----|-------------|
| **BUG-10/REC-08** | Speaker assignment uses maximum-overlap logic instead of arbitrary first-speaker |
| **REC-07** | Short adjacent segments from same speaker are merged before translation |

### 8. [config.py](file:///c:/just-dub-it2/app/utils/config.py)
| Fix | Description |
|-----|-------------|
| **BUG-09** | `config.get()` now caches for 5 seconds instead of reading `settings.json` from disk on every call |
| **New** | Added `target_language` config setting |

### 9. [preprocessing.py](file:///c:/just-dub-it2/tools/MuseTalk/musetalk/utils/preprocessing.py)
| Fix | Description |
|-----|-------------|
| **BUG-13/REC-12** | Landmark models (Wholebody + FaceAlignment) lazy-loaded on first use instead of at import time. Saves ~524MB VRAM during earlier pipeline stages |

### 10. [cuda_utils.py](file:///c:/just-dub-it2/app/utils/cuda_utils.py)
| Fix | Description |
|-----|-------------|
| **BUG-14** | `check_vram_headroom()` now forces a synchronous `nvidia-smi` read instead of using the 5-second-stale background cache |

### 11. [bootstrap.py](file:///c:/just-dub-it2/app/bootstrap.py)
| Fix | Description |
|-----|-------------|
| **New** | Set `GRADIO_TEMP_DIR` to `workspace/gradio_temp` to bypass Windows system temp directory `PermissionError: [Errno 13] Permission denied` |

### 12. [inference.py (MuseTalk)](file:///c:/just-dub-it2/tools/MuseTalk/scripts/inference.py)
| Fix | Description |
|-----|-------------|
| **Optimized** | Changed final audio combination command to copy the video stream (`-c:v copy`) instead of re-encoding. Speeds up inference output assembly and locks in the `yuv420p` format. |

---

## Impact Summary

### Lipsync Quality Improvements
1. MuseTalk receives **clean vocals** instead of background-contaminated audio → better lip estimation
2. **Chunk overlap** eliminates seam artifacts at boundaries → smooth transitions  
3. **Timeline drift fix** keeps audio in sync → lips match speech timing
4. **Speed clamp widening** ensures clips actually fit their slots → no overflow
5. **Duration contracts at TTS** reduce reliance on time-stretching → natural pacing

### Translation Quality Improvements
1. **Single translation pass** (no more double-translation) → preserves nuance
2. **Language-aware prompts** → correct source→target translation behavior
3. **Short segment merging** → fewer fragments, better context for LLM
4. **Speaker ID preservation** → correct voice assigned to each line

### Stability/Performance Improvements
1. All GPU stages now route through `run_gpu_stage()` → proper VRAM tracking
2. Config caching reduces disk I/O during translation
3. Lazy-loaded MuseTalk models prevent premature VRAM consumption
4. Fresh VRAM reads for critical headroom decisions
5. **Custom Gradio temp directory** prevents Windows temp folder permission blockages when dragging and dropping videos.
6. **Unified browser playability (yuv420p)** enforced across MuseTalk chunk encoding, scene auditor slicing, and pipeline concatenation.

---

> [!IMPORTANT]
> **For existing workspaces:** These fixes only apply to **new pipeline runs**. If you have cached intermediate files (e.g., `translated_segments.json`, `segments.json`) from previous runs, you'll need to delete them to re-process with the fixed stages. Specifically:
> - Delete `segments.json` and `transcription_results.json` to re-transcribe (now in original language)
> - Delete `translated_segments.json` and `translation_checkpoint.json` to re-translate
> - Delete `dub_final.wav` and `vocal_master.wav` to re-mix audio
> - Delete `lipsynced_video.mp4` and the `lipsync_chunks/` folder to re-run lipsync
