# MuseTalk 4GB Compression Patch (Final Ignition)

The current MuseTalk `inference.py` script attempts to load FP32 tensors before converting to FP16. This causes a VRAM spike (>5GB) that deadlocks 4GB cards like the 1050 Ti. I will patch the loading logic for absolute memory safety.

## User Review Required

> [!WARNING]
> **Code Modification**: I am modifying a third-party script inside `tools/MuseTalk`. This is necessary to fit the 1-hour project into your hardware limits.
> **Restart Required**: One last "Deep Purge" is needed to clear the current deadlock before applying the patch.

## Proposed Changes

---

### [Component] MuseTalk Memory Safety

#### [MODIFY] [inference.py](file:///c:/just-dub-it2/tools/MuseTalk/scripts/inference.py)
*   **Direct FP16 Loading**: Update `WhisperModel.from_pretrained` and the UNet loader to specify `torch_dtype=torch.float16` during the initial load.
*   **Sequential Cache Flush**: 
    1.  Load VAE.
    2.  `torch.cuda.empty_cache()`.
    3.  Load UNet.
    4.  `torch.cuda.empty_cache()`.
    5.  Load Whisper. 
*   **Accelerate Integration**: Force `low_cpu_mem_usage=True` for all transformers.

---

### [Component] Finalization Resume 

#### [EXECUTE] The 100% Push
*   Perform one last `taskkill` to unlock the "Frozen" GPU.
*   Restart `launch.bat`.
*   Job 1776007462 will resume at Stage 7 and **finally ignite** on the 1050 Ti.

## Open Questions

1.  **Shall I proceed with the MuseTalk Patch?** (This is the most reliable way to hardware-stabilize the final stage).
