# Project Cleansing & Prompt Hardening Plan

We have confirmed why the 1-hour movie is taking too long: The LLM was adding lengthy "Translation Notes" and "Explanation notes" to the segments. The TTS were then dutifully synthesizing every single robot explanation, ballooning the project time and ruining the dub quality.

## User Review Required

> [!WARNING]
> **Data Cleansing (Destructive Action)**: To fix the quality and speed, I must delete your current `translated_segments.json` and the partial TTS audio clips for Job 1776007462. This will force the system to regenerate clean, note-free English translations.

## Proposed Changes

---

### [Component] Translation Prompt Engineering

#### [MODIFY] [translate.py](file:///c:/just-dub-it2/app/stages/translate.py)
*   **Prompt Overhaul**: Update the `translate_text` function with a high-authority instruction set:
    *   `DO NOT UNDER ANY CIRCUMSTANCES PROVIDE EXPLANATIONS OR NOTES.`
    *   `IF NO TRANSLATION IS NEEDED, RETURN THE ORIGINAL TEXT ALONE.`
    *   `STOP TOKENS`: Add more aggressive stop tokens to cut the LLM off if it starts "chattering."

---

### [Component] Data Integrity & Cleansing

#### [DELETE] Movie Workspace Artifacts
*   Remove `workspace/1776007462/translated_segments.json` (The polluted data).
*   Remove `workspace/1776007462/translation_checkpoint.json`.
*   Remove everything in `workspace/1776007462/tts_output/`.
*   This resets the project to Stage 3, where it will now "blast" through with the new fast-prompt.

---

### [Component] System "Flattening"

#### [CLEANUP] Diagnostic Tools
*   Move `app/benchmark_translate.py` to a new `tools/benchmarks/` folder or delete it to keep the core root "flat" and clean as requested.
*   Consolidate the current `settings.json` and `launch.bat` state to ensure we are running in the "Goldilocks Zone" (CPU Translation + GPU TTS).

## Open Questions

1.  **Wipe Confirmation**: Are you okay with me wiping the current translation progress for this specific movie to ensure we get a "Note-Free" dub? 
2.  **Diagnostic Tool**: Would you like to keep the `benchmark_translate.py` tool for future cards, or should I "flatten" it (remove it) now that we have the results?

## Verification Plan

### Automated Tests
*   `findstr` test: After the new run starts, I will search the new segments for the word "Note:" to verify 100% silence from the robot.

### Manual Verification
*   Verify that the ETA for TTS has dropped by at least 50% now that the extra explanations are gone.
