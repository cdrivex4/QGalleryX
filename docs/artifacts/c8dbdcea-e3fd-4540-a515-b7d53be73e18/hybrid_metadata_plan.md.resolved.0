# Hybrid Metadata & Dialogue Scrubbing Plan

We will implement a "Hybrid" translation system that treats LLM commentary as **Metadata**, not **Dialogue**. This keeps your valuable translation notes safe but prevents the voice actors (TTS) from speaking them out loud.

## User Review Required

> [!IMPORTANT]
> **Data Migration**: I will run a custom "Scrubber" script on your `translated_segments.json`. This will automatically move phrases like "(Note: ...)" and "English: ..." out of the speech text and into a hidden `translation_notes` field. This prevents us from having to restart the 1-hour movie from scratch.

## Proposed Changes

---

### [Component] Metadata Storage Logic

#### [MODIFY] [translate.py](file:///c:/just-dub-it2/app/stages/translate.py)
*   **Dual-Output Parser**: Update the `translate_text` function to return a dictionary: `{"text": "Clean Dialogue", "notes": "Robot Commentary"}`.
*   **Prompt Formatting**: Instruct the LLM to use a strict separator:
    `--- DIALOGUE ---`
    `[Translated Text]`
    `--- NOTES ---`
    `[Comments]`

---

### [Component] Pipeline Synchronization

#### [MODIFY] [pipeline.py](file:///c:/just-dub-it2/app/pipeline.py)
*   Update the translation loop to handle the new dictionary format.
*   Ensure that when passing data to the **TTS Stage**, only the `text` field is sent, keeping the synthesis clean and fast.

---

### [Component] Data Recovery (The "Scrubber")

#### [NEW] [scrub_pollution.py](file:///c:/just-dub-it2/app/scratch/scrub_pollution.py)
*   A one-off script that:
    1.  Scans `workspace/1776007462/translated_segments.json`.
    2.  Uses Regex to identify "(Note: )", "[Action]", "English: ", and other chatter.
    3.  Moves that chatter to a `notes` field.
    4.  Overwrites the JSON with a clean version, making it "TTS-Ready."

## Verification Plan

### Automated Tests
*   Run the Scrubber and verify that the file size decreases (due to stripped chatter) and the structure now includes `notes`.

### Manual Verification
*   Check Segment #588 in the UI. It should now show "You" as the dialogue and the explanation as a note, with an ETA indicating the synthesis is much faster.
