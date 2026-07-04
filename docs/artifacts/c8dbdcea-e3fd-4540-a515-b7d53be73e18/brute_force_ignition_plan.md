# The "Brute Force" Ignition Plan

Our "smart" way of asking the computer to look for the GPU files in different folders is being ignored. This is why you see the "AI Core Booting" hang—the software is standing in front of your 1050 Ti but can't find the key to start it.

I am going to "Brute Force" the connection.

## User Review Required

> [!CAUTION]
> **Environment Hygiene**: I will be copying the NVIDIA DLLs directly into the core folder of your AI library (`site-packages/llama_cpp/lib`). This is a "surgical" fix that bypasses Windows' complicated path system. It is very effective but slightly "messy" for the virtual environment.

## Proposed Changes

---

### [Component] Hardware Linkage (Brute Force)

#### [EXECUTE] DLL Relocation
*   Copy `cudart64_12.dll` from the `archive/` folder into `env/Lib/site-packages/llama_cpp/lib/`.
*   Copy `cublas64_12.dll` and `cublasLt64_12.dll` into the same folder.
*   **The Result**: The AI engine will no longer have to "look" for its dependencies; they will be sitting right next to it. 

---

### [Component] Final Activation

#### [RESTART] The 1-hour Movie
*   Restart `launch.bat`.
*   **Verification**: I will monitor for the **VRAM spike to 2.5 GB**. 
*   **Reminder**: If the Voice Engine (TTS) fails later due to the Numpy version, we will perform the finalize downgrade then (as previously discussed).

## Open Questions

1.  **Permission to proceed with the Brute Force copy?** This is the most reliable way to get a 1050 Ti out of its "CPU-only" cage.
