# Final Data Healing & Prompt Hardening Plan

The GPU-powered Llama engine is being too "helpful" and is prepending things like **"2.00s"** or **"Duration:"** to your dialogue lines. This is causing the characters to announce their own line-durations before speaking. 

We need to surgically clean the data and harden the engine to prevent this.

## User Review Required

> [!CAUTION]
> **Audio Regeneration**: I must delete the audio files (Stage 5) for any segments that have been "polluted" with duration text. The system will automatically redo them with the clean text once the "Surgery" is complete.

## Proposed Changes

---

### [Component] Stage 3 Engine Hardening (GPU)

#### [MODIFY] [translate.py](file:///c:/just-dub-it2/app/stages/translate.py)
*   **Prompt Upgrade**: Add a "Negative Constraint" to the prompt: `NEVER output durations (e.g. 2.00s) in the DIALOGUE section`.
*   **Parser Rocketry**: Update `parse_hybrid_emission` with a "Cleanup Pass" that strips any leading duration markers (Regex: `^\d+\.\d+s\s*`) if the LLM ignores the prompt.

---

### [Component] Data Healing (1-hour Movie)

#### [MODIFY] [scrub_pollution.py](file:///c:/just-dub-it2/app/scratch/scrub_pollution.py)
*   Update the script to specifically target the `X.XXs` pollution I found in `translated_segments.json`.
*   Run the script to "heal" the 1-hour movie data without losing your translation progress.

---

### [Component] Audio Reset

#### [EXECUTE] Delete Polluted Chunks
*   Wipe `workspace/1776007462/vocal_segments/*.wav`.
*   This forces the TTS engine to re-read the "Healed" text, giving you perfect, note-free audio.

## Verification Plan

### Automated Tests
*   Run a single-segment test through the new parser to ensure `2.00s Hello` becomes `Hello`.

### Manual Verification
*   User to listen to Segment #1 of the 1-hour movie. It should no longer say "Two point zero zero s."
