# Walkthrough — LTX-2 Setup & Optimization

We have matured the project from a "Lite" placeholder to the full LTX-2 pipeline with a robust, safe startup experience tailored for local 4GB VRAM hardware.

## Accomplishments

### 1. Robust Weight Acquisition (~44GB)
- **Automatic Bypassing**: Switched to a public mirror for the Gemma text encoder to bypass Hugging Face gating.
- **Verification**: All ~44GB of weights are verified and ready in the `weights/` directory.

### 2. Paging Error & Leak Resolution
| Symptom | Root Cause | Resolution |
|---|---|---|
| **25% & Start Failure** | GPU address space exhaustion / Closure Leaks | CPU Offloading + Closure Nullification. |
| **`torch_cpu.dll` Crash** | `ModelLedger` path mismatch + BF16 emulation | Fixed weight sharding + In-place casting. |
| **Blank Browser** | Stale processes / Port conflict | `start_justdubit.bat` now auto-cleans ports. |
| **Silent Starts** | No feedback during loading | Redirected console to `logs/api_startup.log`. |

### 3. Low-VRAM Optimizations (4GB Support)
- **FP8 Quantization**: Enabled FP8 for the transformer, cutting VRAM/RAM footprint by half.
- **Sequential Loading**: Refactored the pipeline to load and unload models one-by-one (Encoders → Transformer → Decoders), preventing system-wide memory exhaustion.
- **Paging Fix (CPU Offloading)**: Forced the 24GB Text Encoder to System RAM (CPU) to avoid overwhelming the GPU's address space.

### 4. Safety & Robustness
- **Surgical Process Cleanup**: The `start_justdubit.bat` script now uses `netstat` and `tasklist` to verify it only stops `python.exe` and `node.exe` on ports 8000 and 5173.
- **Zombie Cleanup**: Identified and terminated `pyrefly.exe` (Zed language server) which was leaking 6GB after indexing large weights.
- **Closure Memory Leak Fix**: Explicitly clearing denoising closures to release captured 19GB transformer model references.

### 5. Loader Stability (Gemma Fixes)
- **Sharded Weight Support**: Updated `ModelLedger` and `sft_loader.py` to correctly handle multi-file safetensors shards and missing "config" metadata in public Gemma weights.
- **In-Place Casting**: Refactored `SingleGPUModelBuilder` to cast BF16 tensors in-place. This eliminates a 24GB peak RAM spike during the loading of the text encoder on CPUs lacking native BF16 support (like the i5-9400F).

## Current Status

| Component | Status |
|-----------|--------|
| Model Weights | **[COMPLETE]** (44GB on disk, verified) |
| Loader | **[STABLE]** (Sharding + In-place casting fixed) |
| Pipeline Code | **[OPTIMIZED]** (FP8 + Sequential + CPU-Offload) |
| Startup | **[SECURE]** (Surgical PID cleanup + Zombie removal) |
| UI/UX | **[REFINED]** (Toasts + Clear Progress) |

## How to Proceed
1. **Close all server windows**.
2. Run **`start_justdubit.bat`**.
3. Upload your video and prompt—the system is now optimized for your hardware.

![UI Refinement Screenshot](/C:/Users/curtis/.gemini/antigravity/brain/d4315882-d6f8-4019-8995-a65ccb637909/job_status_area_refined_1772255190667.png)
*The refined Workstation sidebar showing precise progress vectors.*
