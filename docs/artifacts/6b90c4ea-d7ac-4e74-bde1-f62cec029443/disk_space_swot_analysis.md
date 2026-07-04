# 💾 FaceSwap Disk Space Optimization — SWOT Analysis

## Current State (Measured)

| Metric | Value |
|---|---|
| **Video** | `Copy.mp4`, 86 seconds @ 25fps |
| **Total Frames** | 5,744 |
| **Avg Frame Size** | ~164 KB (PNG, 720p) |
| **Input Frames Folder** | `faceswap_frames/` — **952 MB** |
| **Output Frames Folder** | `faceswap_out/` — **~952 MB** (when complete) |
| **Peak Disk Usage** | **~1.9 GB** (both folders simultaneously on disk) |
| **Your Requested Cap** | **500 MB combined total** |

> [!WARNING]
> At 720p, every minute of 25fps video generates ~250 MB of PNG frames. A 10-minute video would consume **~5 GB** just for extraction. Without intervention, longer videos will fill your 235 GB free space fast.

---

## Strategy 1: Sliding Window Batched Processing

**Concept:** Extract → Swap → Encode in fixed-size batches (e.g., 500 frames at a time). Delete each batch's frames as soon as they're encoded into a partial `.ts` segment. After all batches complete, FFmpeg concatenates the segments into the final video.

```
[Extract batch 1] → [Swap batch 1] → [Encode segment_001.ts] → [DELETE batch 1 frames]
                    [Extract batch 2] → [Swap batch 2] → [Encode segment_002.ts] → [DELETE batch 2 frames]
                                        ...
                    [FFmpeg concat all .ts segments] → final_output.mp4
```

### SWOT

| | |
|---|---|
| **Strengths** | ✅ Disk usage capped to `batch_size × avg_frame_size × 2` (input + output). With 500 frames × 164KB × 2 = **~160 MB peak** — well under the 500 MB cap. ✅ Resume is trivial: skip batches whose `.ts` segment already exists. ✅ Works with every codec, every resolution, no architectural changes to the face-swap core. ✅ Each batch is independently verifiable. |
| **Weaknesses** | ❌ Encoding introduces multiple FFmpeg start/stop cycles (model load overhead per batch). ❌ `.ts` concat can introduce 1-frame glitches at segment boundaries if GOP alignment is wrong. ❌ GFPGAN enhancement must also be batched, adding complexity. ❌ More code to write and test (batch orchestrator, segment tracker, concat logic). |
| **Opportunities** | 💡 Batch size could be auto-calculated from free disk space: `batch = (free_space × 0.8) / (avg_frame_size × 2)`. 💡 Parallelism: could extract batch N+1 while swapping batch N. 💡 Progress becomes very granular: "Batch 3/12 — Swapping frame 245/500". |
| **Threats** | ⚠️ If the concat step fails or is interrupted, all `.ts` segments must be preserved until final output is written. ⚠️ Windows file handle limits could cause issues if hundreds of segments accumulate. ⚠️ Video quality may degrade slightly across segment boundaries with lossy codecs. |

| Metric | Rating |
|---|---|
| **Implementation Complexity** | 🟡 Medium (3-4 hours) |
| **Disk Savings** | 🟢 **~85%** (952 MB → ~160 MB peak) |
| **Quality Impact** | 🟡 Minor risk at segment boundaries |
| **Resume Support** | 🟢 Excellent |
| **Compatibility** | 🟢 Works with all existing code |

---

## Strategy 2: FFmpeg Pipe Streaming (Zero-Disk Extraction)

**Concept:** Eliminate the extraction folder entirely. Pipe raw frames from FFmpeg directly into Python via `stdout`, process each frame in memory, and pipe the result back into a second FFmpeg process via `stdin` for real-time encoding.

```
[FFmpeg decode] --stdout--> [Python: swap face in RAM] --stdin--> [FFmpeg encode] → output.mp4
```

### SWOT

| | |
|---|---|
| **Strengths** | ✅ **Zero disk usage for frames.** Only the final output video touches the filesystem. ✅ Fastest possible throughput — no disk I/O bottleneck at all. ✅ Elegant, Unix-philosophy pipeline. |
| **Weaknesses** | ❌ **No resume capability.** If the process crashes at frame 4000/5744, you start from scratch. ❌ GFPGAN enhancement cannot easily be inserted into a streaming pipeline (it needs the full frame in memory and returns asynchronously). ❌ Progress tracking becomes harder — you must count frames in the pipe. ❌ Error handling is brutal: if the encode pipe breaks, the decode pipe keeps flooding RAM. ❌ Debugging is nearly impossible — you can't inspect intermediate frames. ❌ **Your 4GB VRAM GTX 1050 Ti** would need to hold InsightFace + the pipe buffer simultaneously. |
| **Opportunities** | 💡 Could checkpoint every N frames by occasionally writing a "bookmark" frame to disk. 💡 Could use a ring buffer to allow 1-frame look-back for temporal smoothing. |
| **Threats** | ⚠️ A single Python GIL stall (garbage collection, VRAM swap) causes the FFmpeg pipe to block, potentially causing FFmpeg to timeout and kill the stream. ⚠️ Raw frame pipes use ~5.5 MB/frame in RAM (1280×720×3 bytes uncompressed). At 25fps that's 137 MB/s of RAM bandwidth. ⚠️ Windows named pipes have historically been flaky with large binary payloads. |

| Metric | Rating |
|---|---|
| **Implementation Complexity** | 🔴 High (6-8 hours) |
| **Disk Savings** | 🟢 **~100%** (zero intermediate frames) |
| **Quality Impact** | 🟢 None (lossless pipe) |
| **Resume Support** | 🔴 None without major rework |
| **Compatibility** | 🔴 Requires rewrite of faceswap.py core loop |

---

## Strategy 3: Selective-Only Extraction (Smart Skip)

**Concept:** Don't extract frames that don't contain the target face. Do a fast pre-scan of the video using a lightweight face detector (or scene-change detection), build a "face map" of which frame ranges actually contain the target, extract and swap only those frames, and copy unmodified frames directly from the source video during re-encoding.

```
[Pre-scan video] → face_map = {frame 100-340, 500-890, ...}
[Extract ONLY face_map frames] → [Swap those] → [Re-encode: swapped frames + original video for gaps]
```

### SWOT

| | |
|---|---|
| **Strengths** | ✅ If the target face appears in only 40% of frames, disk usage drops by 60%. ✅ Swap time also drops proportionally — fewer frames to process. ✅ Conceptually elegant: "only touch what needs touching." |
| **Weaknesses** | ❌ **Pre-scan adds a full video decode pass** before any work begins (doubles wall-clock time for the analysis phase). ❌ Re-encoding requires complex FFmpeg filter chains (`select`, `overlay`, `concat`) to merge swapped and original frames at the correct indices. ❌ If face detection has false negatives during pre-scan, those frames will have the wrong face in the output. ❌ The "face map" assumption breaks down for videos where the target face is in 90%+ of frames (e.g., interviews, vlogs). |
| **Opportunities** | 💡 The face map could be cached and reused for re-runs with different source faces. 💡 Could combine with Strategy 1 (batch only the face-containing ranges). 💡 Pre-scan could use the same InsightFace model at lower resolution for speed. |
| **Threats** | ⚠️ False negatives in pre-scan → visible face flicker (original face appears for 1-2 frames mid-swap). ⚠️ The FFmpeg filter chain for selective frame replacement is notoriously fragile and poorly documented. ⚠️ Frame-accurate seeking in FFmpeg is imprecise with variable-bitrate source videos. |

| Metric | Rating |
|---|---|
| **Implementation Complexity** | 🔴 High (8-10 hours) |
| **Disk Savings** | 🟡 **Variable** (0-60% depending on face screen time) |
| **Quality Impact** | 🟡 Risk of face-flicker on false negatives |
| **Resume Support** | 🟡 Possible but complex |
| **Compatibility** | 🔴 Major rework of encode stage |

---

## Strategy 4: JPEG Intermediate Format

**Concept:** Replace PNG (lossless, ~164 KB/frame) with high-quality JPEG (lossy, ~30-50 KB/frame at quality 95). This is a one-line change in FFmpeg extraction (`%06d.jpg` instead of `%06d.png`) and `cv2.imwrite` quality parameter.

```
Current:  5,744 × 164 KB (PNG) = 952 MB
Proposed: 5,744 ×  40 KB (JPEG Q95) = 230 MB
```

### SWOT

| | |
|---|---|
| **Strengths** | ✅ **Trivially easy to implement** — literally change file extensions and add quality params. ✅ Immediate ~75% reduction: 952 MB → ~230 MB for input, ~460 MB total. ✅ No architectural changes whatsoever. ✅ Resume, progress, batching — everything works identically. ✅ Battle-tested: every video editor in the world uses JPEG intermediates. |
| **Weaknesses** | ❌ **Lossy compression introduces artifacts**, especially around face edges, hair, and skin tones — exactly the areas the face-swap AI needs to be pixel-perfect. ❌ Each generation of JPEG re-compression degrades quality (extract → save as JPEG → load → swap → save as JPEG → load for GFPGAN → save again). That's **3 lossy compressions.** ❌ JPEG doesn't support alpha channels if we ever need transparency for masking. |
| **Opportunities** | 💡 Could use WebP (lossy at Q95) instead — 30% smaller than JPEG with fewer artifacts. 💡 Could use JPEG for extraction only and PNG for swap output (hybrid approach). 💡 Quality parameter could be user-configurable in Settings tab. |
| **Threats** | ⚠️ Face-swap quality degradation may be invisible at 720p but catastrophic at 4K. ⚠️ GFPGAN was trained on PNG inputs — JPEG artifacts may confuse the enhancement model. ⚠️ Users who compare output quality frame-by-frame will notice the generational loss. |

| Metric | Rating |
|---|---|
| **Implementation Complexity** | 🟢 Trivial (30 minutes) |
| **Disk Savings** | 🟢 **~75%** (952 MB → ~230 MB) |
| **Quality Impact** | 🔴 Cumulative lossy degradation across 3 save cycles |
| **Resume Support** | 🟢 Unchanged |
| **Compatibility** | 🟢 Drop-in replacement |

---

## Strategy 5: Hybrid Concat Encoding (The Recommended Approach)

**Concept:** Combine the best of Strategy 1 + Strategy 3. Process frames in batches, but instead of extracting ALL frames upfront, use FFmpeg's frame-accurate seeking to extract only one batch at a time. Swap each batch, encode it immediately into a lossless intermediate `.mkv` segment using FFV1 (or near-lossless H.264 CRF 0), delete the frames, then concat all segments.

**Key innovation:** The input frame folder and output frame folder are never larger than one batch. Frames that don't contain a detected face are passed through without modification (no swap, no quality loss), but they're still part of the batch pipeline so frame ordering is preserved perfectly.

```
for batch in range(0, total_frames, BATCH_SIZE):
    [FFmpeg seek → extract batch to faceswap_frames/]     ← ~80 MB
    [Swap faces in batch → write to faceswap_out/]         ← ~80 MB
    [Encode batch → segment_NNN.mkv (lossless)]            ← ~15 MB
    [DELETE faceswap_frames/* and faceswap_out/*]           ← back to 0
    
[FFmpeg concat all .mkv segments + original audio] → final_output.mp4
```

### SWOT

| | |
|---|---|
| **Strengths** | ✅ **Peak disk = 1 batch of input + 1 batch of output + accumulated segments.** With 500-frame batches: `(500 × 164KB × 2) + segments ≈ 200 MB` — comfortably under 500 MB cap. ✅ Lossless intermediate encoding (FFV1/CRF 0) means zero quality loss. ✅ Resume is rock-solid: skip batches whose `.mkv` segment already exists. ✅ No-face frames pass through untouched — no wasted compute. ✅ Works with GFPGAN (batch the enhancement within each window). ✅ Progress is naturally granular: "Batch 3/12". ✅ Auto-tunable batch size based on free disk space. |
| **Weaknesses** | ❌ Most complex implementation of the five strategies. ❌ FFmpeg frame-accurate seeking for batch extraction requires careful `-ss` / `-frames:v` math. ❌ Lossless `.mkv` segments are still ~2-3× larger than the final H.264 output (but still far smaller than raw PNGs). ❌ Final concat step must handle audio sync carefully. |
| **Opportunities** | 💡 Batch size auto-calculator: `batch = min(500, int((free_disk * 0.4) / (avg_frame_bytes * 2)))`. 💡 Could display per-batch ETA and rolling average. 💡 Could pipeline: extract batch N+1 while encoding batch N. 💡 The segment-based approach enables future parallel processing (multiple GPUs, one batch each). |
| **Threats** | ⚠️ FFmpeg CRF 0 encoding is CPU-heavy — may slow down on your system even with 50% thread cap. ⚠️ If the concat step is interrupted before writing the final output, all segments must be preserved. ⚠️ Some edge-case videos (variable frame rate) may cause frame-count mismatches between batches. |

| Metric | Rating |
|---|---|
| **Implementation Complexity** | 🟡 Medium-High (5-6 hours) |
| **Disk Savings** | 🟢 **~90%** (952 MB → ~100-200 MB peak) |
| **Quality Impact** | 🟢 Zero (lossless intermediates) |
| **Resume Support** | 🟢 Excellent (per-batch checkpointing) |
| **Compatibility** | 🟡 Requires refactor of faceswap.py core loop |

---

## Comparison Matrix

| Strategy | Disk Savings | Quality | Resume | Complexity | Best For |
|---|---|---|---|---|---|
| **1. Sliding Window** | 🟢 85% | 🟡 Minor | 🟢 Yes | 🟡 Medium | Quick win, good enough |
| **2. FFmpeg Pipe** | 🟢 100% | 🟢 None | 🔴 No | 🔴 High | Short videos, no crash risk |
| **3. Selective Skip** | 🟡 Variable | 🟡 Flicker risk | 🟡 Partial | 🔴 High | Videos with sparse face time |
| **4. JPEG Format** | 🟢 75% | 🔴 Lossy × 3 | 🟢 Yes | 🟢 Trivial | "Good enough" quality tolerance |
| **5. Hybrid Concat** | 🟢 90% | 🟢 None | 🟢 Yes | 🟡 Med-High | **Production-grade solution** |

## 🏆 Recommendation

> [!IMPORTANT]
> **Strategy 5 (Hybrid Concat Encoding)** is the recommended approach. It is the only strategy that simultaneously achieves all three of your requirements:
> 1. **500 MB disk cap** — peak usage ~160-200 MB
> 2. **Zero quality loss** — lossless intermediates
> 3. **Full resume support** — per-batch segment checkpointing
>
> Strategy 4 (JPEG) could be layered on top as an optional "low disk mode" toggle in Settings for users who prioritize speed over quality.

## Next Steps

Which strategy (or combination) would you like me to implement? I can start immediately while your current job finishes processing.
