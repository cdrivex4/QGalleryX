# Hardware Acceleration & Dynamic Resource Orchestration Plan

This plan outlines the architecture for fixing the FFmpeg hardware acceleration failure, implementing intelligent startup environment profiling, and creating dynamic memory/IO guardrails to prevent crashing on low-spec hardware.

## Phase 0: FFmpeg Security Audit & Core Update (Pre-Requisite)
FFmpeg recently had several significant CVEs disclosed (including heap buffer overflows and potential Remote Code Execution vulnerabilities). Before we optimize the hardware acceleration, we must update the core FFmpeg binaries.

*   **SWOT Analysis for Updating FFmpeg:**
    *   **Strengths:** Closes severe RCE and memory corruption vulnerabilities. Provides access to newer hardware decoding APIs and better HEIC support.
    *   **Weaknesses:** Updating FFmpeg across major versions (e.g., 5.x -> 7.x) almost guarantees broken APIs in our C++ wrapper. Structs, memory management functions, and hardware context negotiation methods often change.
    *   **Opportunities:** A forced update allows us to rewrite `VideoThumbnailer.cpp` using the modern `av_hwdevice_ctx_create` pipeline cleanly, resolving our `d3d11` issues simultaneously.
    *   **Threats:** If the API changes significantly, it could temporarily break all video thumbnail generation until the new pipeline is fully debugged.

## Phase 1: Startup Environment & Capabilities Profiling
Before initializing our heavy processing queues, we need to understand the constraints of the host machine.

1. **Hardware Snapshot Logging:**
   - Extend `SystemMonitor` (or create a dedicated `EnvironmentProfiler`) to capture system specs at startup.
   - **Metrics to capture:** CPU Model/Cores, Total System RAM, OS Version, and available GPU(s).
   - Log this snapshot immediately to `logs/application.log` and `logs/crash.log` to provide context for debugging remote machines.

2. **FFmpeg Hardware Capabilities Probe:**
   - Write a probe function that iterates through supported `AVHWDeviceType` options in the host's FFmpeg build.
   - Detect and log availability of specific hardware decoders:
     - `AV_HWDEVICE_TYPE_CUDA` (NVIDIA)
     - `AV_HWDEVICE_TYPE_QSV` (Intel QuickSync)
     - `AV_HWDEVICE_TYPE_D3D11VA` (Direct3D 11 - Modern Windows)
     - `AV_HWDEVICE_TYPE_DXVA2` (DirectX Video Acceleration - Legacy Windows)

## Phase 2: Robust GPU Hardware Acceleration Fallback Chain
Currently, `VideoThumbnailer` fails because it expects a specific `d3d11` configuration. We must make this resilient.

1. **Dynamic Decoder Selection:**
   - During FFmpeg context initialization, use the results from the capabilities probe to select the best available decoder.
   - **Priority Chain:** NVDEC/CUDA -> QSV -> D3D11VA -> DXVA2 -> Software (CPU) Fallback.
2. **Proper Context Management:**
   - Ensure `AVBufferRef *hw_device_ctx` is correctly initialized via `av_hwdevice_ctx_create()`.
   - Implement strict cleanup procedures to release the hardware context when the video stream closes, preventing VRAM leaks (critical for shared-memory integrated GPUs like the i3-7100).

## Phase 3: Memory Budgets & VRAM Guardrails
To support computers with 4GB to 32GB+ RAM without crashing, memory allocation must be dynamic.

1. **Calculate Dynamic Budgets:**
   - Base the application's RAM budget on the detected Total RAM.
   - *Example:* On an 8GB system, cap the `AsyncImageProvider` cache and video frame queue at 1GB. On a 32GB system, allow up to 4GB.
2. **Active Memory Pressure Sensor:**
   - Continuously monitor Available RAM via `SystemMonitor`.
   - If Available RAM drops below a critical threshold (e.g., < 10% or < 500MB):
     - Trigger an emergency `trim()` on internal caches.
     - Temporarily pause background prefetching in `TaskScheduler` until memory recovers.
3. **VRAM Consideration:**
   - Integrated GPUs (like the i3's HD 630) share system memory. By strictly monitoring system RAM, we inherently protect against overflowing integrated VRAM.

## Phase 4: Dynamic IO Orchestration
Bind the hardware capabilities with storage latency to dictate queue depths.

1. **Storage Context Awareness:**
   - Differentiate between Network/HDD vs SSD storage mediums.
2. **Dynamic Queue Scaling:**
   - If reading from a high-latency source (Network/HDD), reduce the `MAX_CONCURRENT_TASKS` in `TaskScheduler` to prevent disk thrashing and thread blocking.
   - If reading from an SSD, increase concurrency to saturate the hardware decoders.

---

### Execution Order
1. **Implement Phase 1:** Get the environment data printing to the logs so we know exactly what we are dealing with on any machine.
2. **Implement Phase 2:** Fix FFmpeg in `VideoThumbnailer.cpp` to negotiate the fallback chain properly and stop hitting the CPU.
3. **Implement Phase 3 & 4:** Tie the memory limits and IO queues to the data we collect in Phase 1.
