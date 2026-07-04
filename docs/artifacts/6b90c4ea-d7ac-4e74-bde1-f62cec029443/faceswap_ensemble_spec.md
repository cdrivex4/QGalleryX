# FaceSwap Temporal Coherency: Ensemble Architecture Spec

## 1. Executive Summary
This document outlines the architectural rewrite required to transform the current "single-frame reference" FaceSwap engine into a **Temporal Coherency Ensemble Pipeline**. The objective is to achieve Hollywood-grade face tracking (handling extreme profiles, closed eyes, and heavy shadows) while strictly operating within a **4GB VRAM constraint** on consumer hardware.

To achieve this without blowing up the GPU, we will use a **Layered Ensemble Approach**. Instead of running heavy Neural Network trackers (like DeepSORT) which consume massive VRAM, we will cascade lightweight heuristic algorithms (Optical Flow, Multi-Image KNN Pooling, and Pose-Aware Math) to seamlessly track identity across a video.

---

## 2. The Layered Ensemble Engine (How It Works)

During the `_run_faceswap_core` loop, every single frame will pass through a 4-layer validation cascade. If Layer 1 fails, it falls back to Layer 2, and so on.

### Layer 1: The Multi-Reference KNN Pool (Zero-Cost VRAM)
Instead of 1 target face, the user provides 3 to 5 reference shots (Front, Left, Right, Closed Eyes).
* **Pre-Computation:** Before the video starts, InsightFace extracts the 512-D embeddings for ALL provided references. These are stored in a simple Python list in system RAM (0 GPU cost).
* **Execution:** For every face found in the video, we calculate the similarity against the *entire pool*. If it matches *any* of the angles > 0.6 threshold, it swaps.

### Layer 2: Pose-Aware Dynamic Thresholding (Pure Math)
If the face doesn't match the KNN pool > 0.6, we check the Euler angles (yaw/pitch) generated natively by InsightFace's detector.
* **Execution:** If `abs(face.pose[1]) > 45` (face is turned sideways), the system recognizes this is a weak profile shot and dynamically drops the similarity threshold for the KNN Pool from `0.6` to `0.4`.

### Layer 3: Lucas-Kanade Optical Flow Bounding-Box Tracking (CPU-Bound)
If Layer 1 and Layer 2 fail (e.g., the actor looks completely backward, resulting in a similarity of 0.1), we rely on physics.
* **Execution:** We maintain a history of the bounding box `[x1, y1, x2, y2]` from the *previous* frame where a successful swap occurred. Using OpenCV's `calcOpticalFlowPyrLK` (which runs entirely on the CPU), we track the physical pixels of that bounding box into the current frame. If the tracked box heavily overlaps (IoU > 0.8) with a detected face, we *force* the swap, completely bypassing the embedding math.

### Layer 4: Rolling Anchor State (Memory)
* **Execution:** If the Optical Flow successfully forces a swap on a strange angle, we temporarily inject that strange angle's embedding into the KNN Pool as a "Temporary Local Anchor". This allows the system to seamlessly track the actor as they turn completely around, and drops the anchor when the scene cuts.

---

## 3. Required Code Rewrites & Wiring

### Phase 1: Gradio UI Overhaul (`app/main_ui.py`)
* **Component Change:** Replace the single `gr.Image(label="Target Reference")` with a `gr.Gallery()` or multiple `gr.Image()` blocks to accept up to 5 reference frames.
* **Payload Change:** Update the job submission JSON payload.
  * *Old:* `"target_reference_path": "path/to/img.png"`
  * *New:* `"target_reference_paths": ["path/1.png", "path/2.png", ...]`

### Phase 2: Data Extraction Pre-Pass (`app/stages/faceswap.py`)
* **Function Rewrite:** Rewrite `get_source_face()` into `get_source_face_pool()`.
* **Logic:** Iterate through all paths in `target_reference_paths`. Extract the `normed_embedding` for each. Return a `List[Face]`.
* **VRAM Safety:** Flush VRAM (`torch.cuda.empty_cache()`) immediately after extracting the pool, before the main video loop begins.

### Phase 3: The Core Evaluation Loop (`app/stages/faceswap.py`)
Inside `_run_faceswap_core`, locate the frame iteration loop:
```python
for face in faces:
    # --- ENSEMBLE CASCADE GOES HERE ---
```
* **Implement Layer 1 (KNN):**
  * Iterate over the `source_face_pool`. Find `max_similarity`.
* **Implement Layer 2 (Pose):**
  * `if abs(face.pose[1]) > 45: current_threshold = base_threshold - 0.2`
* **Implement Layer 3 (Optical Flow):**
  * Store `prev_gray_frame` and `prev_target_bbox` at the end of every successful loop.
  * If `max_similarity < current_threshold`, trigger `cv2.calcOpticalFlowPyrLK` comparing `prev_gray_frame` to `current_gray_frame`. Calculate the Intersection over Union (IoU) of the tracked points vs the current `face.bbox`. If `IoU > 0.75`, force swap.

### Phase 4: Job State & Resume Logic (`app/fs_worker.py`)
* Ensure that the `fs_jobs/` JSON format correctly serializes lists for `target_reference_paths`.
* The `test_faceswap.py` suite must be updated to mock a list of reference paths to ensure the new ensemble logic stays under the 150MB disk footprint constraint.

---

## 4. Hardware SWOT Analysis (4GB VRAM Focus)

* **Strengths:** 
  * Avoids loading DeepSORT/YOLO into VRAM. OpenCV Optical Flow is pure CPU math, keeping our precious 4GB VRAM completely dedicated to InSwapper and GFPGAN.
  * No pre-scanning required. The video streams linearly.
* **Weaknesses:**
  * Optical Flow tracking breaks instantly on scene cuts (camera angle changes).
* **Mitigation (The Fix):** 
  * Because Optical Flow breaks on scene cuts, the pipeline automatically falls back to Layer 1 (KNN Pool). As long as the user provided a decent reference image for the new scene's angle, the cascade immediately catches it and resumes tracking.

## 5. Next Steps for Implementation
If this architecture is approved by review, the rollout sequence should be:
1. Update `main_ui.py` to support multi-image upload and pass arrays instead of strings.
2. Rewrite the distance/similarity math in `faceswap.py` to support array pools.
3. Inject the OpenCV Optical Flow CPU tracker into the core loop.
4. Run `test_faceswap.py` to verify no memory leaks occur from storing previous frame arrays.
