# Dubbing Studio Efficiency Ledger

This ledger tracks the performance metrics for Job **1776007462** (1-hour movie) across different hardware configurations and data states.

| Stage | Mode | State | Avg. Speed / Segment | Quality | Status |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **S1: Stem Split** | GPU | Clean | ~0.5s | Perfect | ✅ Completed |
| **S2: Transcribe** | GPU | Clean | ~1.2s | High | ✅ Completed |
| **S3: Translate** | **CPU** | **Polluted** | **~0.2s (Fast)** | **FAIL (Notes)** | ❌ Wiped |
| **S3: Translate** | **GPU** | **Clean** | **~0.4s (Safe)** | **Elite** | 🛡️ ACTIVE |
| **S5: Synthesis** | GPU | Polluted | ~45.0s | Poor | ❌ Wiped |
| **S5: Synthesis** | GPU | Clean | **~8.0s (Target)** | High | ⏳ PENDING |

## 📊 Observations

### 1. The "Fast but Wrong" CPU Trap
In the initial "Polluted" run on the CPU, the Translation stage was actually faster (~0.2s/seg) because the model was small and your CPU (3.89 GHz) handled the logic well. **However**, it produced "Robot Notes" which caused the TTS stage to balloon to **45+ seconds per segment.**

### 2. The "GPU Turbo" Impact (Active)
Currently, in **Stage 3 (GPU)**, your 1050 Ti is handling the Llama-3 model. While the per-segment speed is slightly slower than the CPU due to VRAM transfer, the **quality is 100% higher** because of our new strict prompt formatting.

### 3. The "Numpy 2.4" Risk Factor
> [!WARNING]
> **Observation Node**: We are currently running on **Numpy 2.4.4**. 
> *   **Stage 3 (Translate)** is handling it fine.
> *   **Stage 5 (TTS)** is where we expect a potential "Pant" moment (Failure). If the sound synthesis fails to start, it is because this version of Numpy has broken the `torchaudio` math libraries. 

## Summary
The system is currently powering through the GPU re-translation of the 1-hour movie. We are "letting it play out" as requested to see if the Voice Engine can survive the new Numpy version.

**Reminder Logged: Move to Numpy 1.26.4 if TTS Pants out.**
