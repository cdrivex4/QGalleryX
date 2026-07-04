# GPU Hardware Migration Plan (Job 1776007462)

To fulfill your request to move "this" specific 1-hour job to the GPU, we need to reset the progress for the translation stage. Currently, the movie has "finished" its CPU translations, so the system is moving onto the next stage. Moving it to the GPU requires re-running the inference.

## User Review Required

> [!CAUTION]
> **Loss of Progress**: To "move" this to the GPU, I must **wipe the 1600+ segments** of English translation already created. The system will then restart from the beginning of Stage 3 using the Llama-3 model on your 1050 Ti.

## Proposed Changes

---

### [Component] Hardware Flag Enforcement

#### [VERIFY] [settings.json](file:///c:/just-dub-it2/settings.json)
*   Confirmed Flag: `"translate_device": "gpu"`
*   Confirmed Intensity: `"translate_gpu_layers": 33`

---

### [Component] Workflow Reset (Stage 3)

#### [DELETE] Translation Artifacts
*   Remove `workspace/1776007462/translated_segments.json`.
*   Remove `workspace/1776007462/translation_checkpoint.json`.
*   This triggers the `PipelineManager` to detect that translation is needed and engage the GPU engine.

---

### [Component] Process Control

#### [EXECUTE] Clean Restart
*   Kill current TTS process.
*   Relaunch the server.
*   The dashboard will now show the status: **`TRANS_GPU (0%)`** and you will see the VRAM load spike.

## Verification Plan

### Automated Tests
*   `nvidia-smi` check: Once Stage 3 begins, I will verify that the LLM is consuming 3.5GB+ of VRAM, proving the GPU is doing the work.

### Manual Verification
*   Verify the dashboard shows "GPU Translating" instead of "CPU Translating".
