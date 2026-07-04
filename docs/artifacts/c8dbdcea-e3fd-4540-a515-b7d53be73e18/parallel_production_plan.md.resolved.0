# Parallel Production Plan (Asynchronous I/O Refactor)

The "Spiky" behavior is caused by a serial execution barrier: the GPU waits for the CPU/Disk to finish saving before starting the next batch. I will refactor the pipeline to use an asynchronous "Hand-off" pattern.

## User Review Required

> [!IMPORTANT]
> **Pipeline Decoupling**: By moving `cv2.imwrite` and `get_image` to a background thread pool, we allow the GPU to start the next 12-frame chunk immediately. 
> **Throughput vs Latency**: This will significantly increase the total frames-per-second, though it will keep your CPU usage high (which is good—it means it's working properly alongside the GPU).

## Proposed Changes

---

### [Component] MuseTalk Inference Script

#### [MODIFY] [inference.py](file:///c:/just-dub-it2/tools/MuseTalk/scripts/inference.py)
*   **Threaded Executor**: Initialize a `ThreadPoolExecutor` with 4 workers.
*   **Asynchronous Save Loop**: 
    1.  Collect the raw AI output (recon) from the GPU.
    2.  Instead of a serial loop for `get_image` and `imwrite`, submit these tasks to the executor.
    3.  Move immediately to the next `unet_obj.model(...)` call.
*   **Synchronization**: Add a final `.shutdown(wait=True)` at the end of the chunk to ensure all frames are written before FFmpeg starts.

---

### [Component] Memory Safety

#### [MODIFY] [inference.py](file:///c:/just-dub-it2/tools/MuseTalk/scripts/inference.py)
*   **VRAM Flushing**: Ensure that the background threads don't accidentally hold onto large GPU tensors, preventing memory fragmentation on the 1050 Ti.

## Open Questions

1.  **Do you want to limit the number of threads?** (I recommend 4 to keep enough CPU headroom for the OS).
