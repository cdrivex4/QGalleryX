# Hardware Restoration Plan: Igniting the 1050 Ti

Your screenshot confirms a "Hardware Fallback" state. Although Torch can see your GPU, the **Translation Engine (Llama-CPP)** is currently trapped on your CPU, which is why your processor is at 100% while your "Cuda" graph is at 0%.

## User Review Required

> [!IMPORTANT]
> **Low-Level Re-installation**: I need to uninstall and re-install the `llama-cpp-python` library using a specialized CUDA-enabled binary. This is a non-destructive change to your data, but it will take a few minutes to download the "Cuda Core."

## Proposed Changes

---

### [Component] Hardware Backend Update

#### [EXECUTE] CUDA Binary Deployment
*   Uninstall the current CPU-only version of `llama-cpp-python`.
*   Install the pre-built CUDA 12.1 wheel from a high-performance repository (e.g., `jcmterry` or similar trusted sources for Windows).
*   **Verification**: Run a one-liner to ensure the internal "BLAS" flag is set to `1`.

---

### [Component] Translation Engine Hardening

#### [MODIFY] [translate.py](file:///c:/just-dub-it2/app/stages/translate.py)
*   **Hardware Audit**: Add a startup check that prints the "Backend Status" to the logs.
*   **Warning System**: If the system detects a CPU-only backend while `translate_device` is set to "gpu", it will post a visible warning in the dashboard.

---

### [Component] Final Resume

#### [RESTART] Dashboards & AI Core
*   Restart the server.
*   The 1-hour movie will continue its high-priority redo, but this time you will see the **"Cuda" graph in Task Manager jump to 50% - 80%** as the 1050 Ti takes over.

## Open Questions

1.  **Visual Confirmation**: Are you ready for me to perform the re-installation? Once done, you shouldn't see that 100% CPU flatline anymore.
2.  **CUDA Version**: I'll assume you have a standard NVIDIA driver setup. If the first install fails, I may need to check your exact `nvcc --version`.

## Verification Plan

### Automated Tests
*   `python -c "from llama_cpp import Llama; ..."` check for `BLAS=1`.

### Manual Verification
*   User to check Task Manager for active VRAM and Cuda occupancy after the first segments start.
