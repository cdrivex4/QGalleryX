# Landmark Acceleration Plan (High-Throughput Refactor)

The current "Extracting landmarks" phase is hitting a "RAM Wall" and "Serial Stall." I will refactor the MuseTalk preprocessing utility to use streaming I/O and parallel CUDA batches.

## User Review Required

> [!IMPORTANT]
> **RAM Thrashing Fix**: By moving to a "Generator" pattern, we stop Windows from swapping to your hard drive. This is the biggest speed boost possible for your setup.
> **Batch Size 8**: Your 1050 Ti (4GB) is perfectly sized to handle a batch of 8 frames.

## Proposed Changes

---

### [Component] MuseTalk Preprocessing Utility

#### [MODIFY] [preprocessing.py](file:///c:/just-dub-it2/tools/MuseTalk/musetalk/utils/preprocessing.py)
*   **Lazy Image Generator**: Create a `get_img_generator(img_list, batch_size)` function that yields image batches from disk.
*   **Refactor `get_landmark_and_bbox`**: 
    1.  Remove the call to `read_imgs(img_list)`.
    2.  Update the loop to process batches of **12 frames** in parallel.
    3.  Pass the full batch to `fa.get_detections_for_batch()` and `model()` to parallelize the CUDA work.
*   **Explicit Memory Disposal**: Call `del` and `gc.collect()` at the end of each batch to ensure the 1050 Ti remains clear.

---

### [Component] AI Core Initialization

#### [MODIFY] [preprocessing.py](file:///c:/just-dub-it2/tools/MuseTalk/musetalk/utils/preprocessing.py)
*   **ONNX CUDA Force**: Confirmed `onnxruntime-gpu` is available. I will force the `CUDAExecutionProvider` to hardware-accelerate the Face Parsing.

## Open Questions

1.  **Do you have 'onnxruntime-gpu' installed?** (If not, the ONNX part will remain on CPU, but the Batching fix will still provide a 5x boost).
