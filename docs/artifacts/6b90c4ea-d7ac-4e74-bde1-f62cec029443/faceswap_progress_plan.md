# FaceSwap Progress, ETA & Resume — Detailed Implementation Plan

> [!IMPORTANT]
> This plan touches **3 files** in a specific order. Each improvement builds on the previous.
> Files: `app/stages/faceswap.py` · `app/fs_worker.py` · `app/main_ui.py`

---

## Improvement 1 — Phase-Weighted Progress Bar

### The Problem
`faceswap.py` calls `progress_callback(i + 1, total, "Swapping frames")` only inside the frame-swap loop. Frame extraction (FFmpeg) and re-encoding (FFmpeg) are silent `subprocess.run()` calls with no wiring to the callback, making the UI freeze at 0% for the entire extraction phase.

### The Fix: `app/stages/faceswap.py`

**Step 1 — Use ffprobe to get total frame count BEFORE extraction.**

Before calling FFmpeg to extract frames, we call ffprobe to get the exact frame count:

```python
# ADD this helper at the top of faceswap.py ~line 20
def _get_frame_count(video_path: str) -> int:
    """Fast frame count via ffprobe stream metadata."""
    from utils.ffmpeg_utils import ffprobe_path, get_env_with_ffmpeg
    cmd = [
        ffprobe_path(), "-v", "error", "-select_streams", "v:0",
        "-count_frames", "-show_entries", "stream=nb_read_frames",
        "-of", "default=noprint_wrappers=1:nokey=1", video_path
    ]
    result = subprocess.run(cmd, capture_output=True, text=True,
                            env=get_env_with_ffmpeg())
    try:
        return int(result.stdout.strip())
    except ValueError:
        return 0
```

This runs once upfront in ~1 second and gives us `total_frames` — the denominator for all ETA math.

**Step 2 — Define phase weights as module-level constants.**

```python
# ADD at top of faceswap.py after imports
PHASE_WEIGHTS = {
    "extracting":  (0.0,  0.05),   # 0%  → 5%
    "swapping":    (0.05, 0.70),   # 5%  → 70%
    "enhancing":   (0.70, 0.90),   # 70% → 90%
    "encoding":    (0.90, 1.00),   # 90% → 100%
}
```

**Step 3 — Replace the flat callback with a phase-aware wrapper.**

Inside `_run_faceswap_core`, add a wrapper that maps each phase's local 0-100 range into the global weighted range before calling the real callback:

```python
# ADD inside _run_faceswap_core, before any subprocess calls
def weighted_cb(local_done, local_total, phase_key):
    if not progress_callback:
        return
    w_start, w_end = PHASE_WEIGHTS.get(phase_key, (0, 1))
    ratio = (local_done / local_total) if local_total > 0 else 0
    global_pct = (w_start + ratio * (w_end - w_start)) * 100
    progress_callback(global_pct, 100, phase_key)
```

**Step 4 — Wire the extraction phase by counting files as they land on disk.**

FFmpeg writes frames sequentially as `000001.png`, `000002.png`, etc. Run FFmpeg in a background thread, and in the main thread poll the directory:

```python
# REPLACE the existing subprocess.run() extraction block with:
import threading

total_frames = _get_frame_count(input_video_path)  # Get total upfront

extract_done = threading.Event()

def _do_extract():
    subprocess.run([
        ffmpeg_path(), "-y", "-i", input_video_path,
        str(frames_dir / "%06d.png")
    ], check=True, capture_output=True)
    extract_done.set()

extract_thread = threading.Thread(target=_do_extract, daemon=True)
extract_thread.start()

# Poll extracted frames and fire weighted callback
while not extract_done.is_set():
    n_done = len(list(frames_dir.glob("*.png")))
    if total_frames > 0:
        weighted_cb(n_done, total_frames, "extracting")
    time.sleep(0.5)
weighted_cb(total_frames, total_frames, "extracting")  # Clamp to 5%
extract_thread.join()
```

**Step 5 — Wire re-encoding phase similarly.**

FFmpeg re-encoding also has a progress mode via `-progress pipe:1`:

```python
# REPLACE the final subprocess.run() re-encode block with:
encode_proc = subprocess.Popen([
    ffmpeg_path(), "-y",
    "-framerate", str(fps),
    "-i", str(out_frames_dir / "%06d.png"),
    "-i", input_video_path,
    "-map", "0:v", "-map", "1:a",
    "-c:v", "libx264", "-crf", "18", "-preset", "slow",
    "-pix_fmt", "yuv420p", "-c:a", "copy",
    "-progress", "pipe:1",   # <-- KEY: stream progress to stdout
    output_video_path
], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True)

frames_encoded = 0
for line in encode_proc.stdout:
    # ffmpeg -progress outputs "frame=NNN"
    if line.startswith("frame="):
        try:
            frames_encoded = int(line.split("=")[1].strip())
            weighted_cb(frames_encoded, total_frames, "encoding")
        except ValueError:
            pass
encode_proc.wait()
```

---

## Improvement 2 — Live Rolling-Average ETA

### The Problem
The current ETA in `main_ui.py` calculates `speed = progress_percent / elapsed`. This is a *lifetime average* — at the start of a job when progress is 0.1%, a 1-second elapsed time gives a wildly wrong ETA of thousands of hours. It needs a minimum of `>10 elapsed` and `>1%` to even show, meaning the user waits 2+ minutes before seeing any estimate.

### The Fix: `app/fs_worker.py` — Enrich the status payload

The worker already updates the status dict via `pb_callback`. We need to track a frame-level timestamp history inside `fs_worker.py` to compute a rolling FPS:

```python
# REPLACE the pb_callback inside main_loop() with:

frame_timestamps = []   # rolling deque of (timestamp, frame_number)

def pb_callback(current_pct, total_pct, phase):
    now = time.time()
    status["progress_percent"] = current_pct
    status["sub_status"] = f"{phase.replace('_', ' ').title()}"
    status["phase"] = phase

    # Track frame throughput for ETA — only during swap/enhance phases
    if phase in ("swapping", "enhancing"):
        frame_timestamps.append((now, current_pct))
        # Keep only the last 20 data points (rolling window)
        if len(frame_timestamps) > 20:
            frame_timestamps.pop(0)
        
        # Calculate rolling speed (% per second)
        if len(frame_timestamps) >= 2:
            dt = frame_timestamps[-1][0] - frame_timestamps[0][0]
            dp = frame_timestamps[-1][1] - frame_timestamps[0][1]
            if dt > 0 and dp > 0:
                pct_per_sec = dp / dt
                remaining_pct = 100.0 - current_pct
                status["etr_seconds"] = remaining_pct / pct_per_sec
    
    update_status(job_id, status)
```

The key differences from the current approach:
- Uses a **20-point rolling window** not a lifetime average
- Starts producing an ETA after just ~20 callback ticks (a few seconds of real work)
- Attaches `etr_seconds` directly to the status JSON so the UI reads it without doing any math

### The Fix: `app/main_ui.py` — Read pre-computed ETA

In `get_fs_status_table()`, the `MockJob` object already reads the status JSON. Add one more field:

```python
# In the MockJob construction loop (~line 216), ADD:
j.etr_seconds = sdata.get("etr_seconds", None)
j.phase = sdata.get("phase", "")
```

Then in the `p_html` template (~line 270), replace the hardcoded stage label with:

```python
# REPLACE the static stage_label with:
PHASE_LABELS = {
    "extracting": "📽️ Extracting Frames",
    "swapping":   "🎭 Swapping Faces (GPU)",
    "enhancing":  "✨ Enhancing with GFPGAN",
    "encoding":   "🎬 Re-encoding Video",
    "": "⚙️ Initializing",
}
stage_label = PHASE_LABELS.get(getattr(job, 'phase', ''), "⚙️ Processing")
```

And replace the ETA calculation (~line 259-262):

```python
# REPLACE the entire etr block with:
etr = getattr(job, 'etr_seconds', None)
```

The ETA text that already exists in the template at line 281 picks this up automatically — no further changes needed there.

---

## Improvement 3 — Frame-Level Resume / Checkpoint

### The Problem
If the worker crashes during frame swapping (e.g. VRAM OOM, power blip, manual restart), it renames the job file back to pending and restarts from frame 1, redoing all completed GPU work. The frames that were already written to `workspace/fs_<id>/faceswap_out/` still exist on disk but are thrown away.

### The Fix: `app/stages/faceswap.py` — Skip already-done frames

The swap loop iterates over `frame_paths` sorted by name (`000001.png`, `000002.png`...). The output folder `faceswap_out/` has the exact same naming convention. All we need to do is check what already exists:

```python
# REPLACE the frame swap loop header:
#   for i, fpath in enumerate(frame_paths):
# WITH:

completed_frames = set(f.name for f in out_frames_dir.glob("*.png"))
resume_count = len(completed_frames)

if resume_count > 0:
    logger.info(f"♻️ Resuming from frame {resume_count}/{total} "
                f"({resume_count/total*100:.1f}% already done)")

for i, fpath in enumerate(frame_paths):
    # SKIP frames already on disk
    if fpath.name in completed_frames:
        # Still fire the progress callback so the bar reflects real state
        if progress_callback:
            weighted_cb(i + 1, total, "swapping")
        continue
    
    frame = cv2.imread(str(fpath))
    # ... rest of existing swap logic unchanged ...
```

### The Fix: `app/fs_worker.py` — Detect partially-done jobs

When the worker picks up a job, it should check the workspace to see if a partial run was interrupted. If `faceswap_out/` already has frames, it should update the status to show resume state instead of "Booting...":

```python
# ADD after reading job_spec (~line 71), before setting initial status:

ws_path = job_spec["workspace_dir"]
out_dir = os.path.join(ws_path, "faceswap_out")
existing_frames = len([f for f in os.listdir(out_dir) if f.endswith(".png")]) \
    if os.path.exists(out_dir) else 0

initial_sub = f"Resuming from frame {existing_frames}..." \
    if existing_frames > 0 else "Booting InsightFace..."

status = {
    "id": job_id,
    "status": "gpu_faceswap",
    "progress_percent": 0.0,
    "sub_status": initial_sub,
    "phase": "swapping" if existing_frames > 0 else "",
    "start_time": time.time(),
    "end_time": None,
    "error": None,
    "output_path": None
}
```

### The Fix: `app/fs_worker.py` — Re-queue failed jobs as resumable

Currently, when a job fails it is renamed to `failed_<id>.json` and abandoned. Instead, we should rename it back to the queue with a `retry_` prefix so it is picked up again automatically:

```python
# REPLACE the except block (~line 120-126) with:

except Exception as e:
    import traceback
    tb = traceback.format_exc()
    logger.error(f"FaceSwap Job {job_id} FAILED:\n{tb}")
    
    status["status"] = "failed"
    status["error"] = str(e)
    status["end_time"] = time.time()
    update_status(job_id, status)
    
    # Check if partially done — if >5% frames exist, mark as resumable
    ws_path = job_spec["workspace_dir"]
    out_dir = os.path.join(ws_path, "faceswap_out")
    done_frames = len([f for f in os.listdir(out_dir) if f.endswith(".png")]) \
        if os.path.exists(out_dir) else 0
    total_frames_approx = job_spec.get("total_frames", 1)
    
    if done_frames > 0:
        logger.info(f"Job {job_id} has {done_frames} frames saved. "
                    f"Marking as resumable.")
        # Move back to pending queue — faceswap.py will skip done frames
        os.rename(processing_path, job_path)  # Back to un-prefixed = re-queued
        status["status"] = "queued"
        status["sub_status"] = f"Will resume from frame {done_frames}"
        update_status(job_id, status)
    else:
        os.rename(processing_path, 
                  os.path.join(FS_JOBS_DIR, f"failed_{job_file}"))
```

---

## Summary of All File Changes

| File | What Changes |
|---|---|
| `app/stages/faceswap.py` | Add `_get_frame_count()`, `PHASE_WEIGHTS`, `weighted_cb()`, threaded extraction poller, ffmpeg `-progress` pipe for encoding, frame-resume skip logic |
| `app/fs_worker.py` | Enrich `pb_callback` with rolling `etr_seconds`, add resume detection at job pickup, replace hard-fail with smart re-queue |
| `app/main_ui.py` | Read `etr_seconds` and `phase` from MockJob, swap static stage label for `PHASE_LABELS` dict, remove the now-redundant ETA math |

> [!TIP]
> The three improvements are **stackable** — Improvement 1 alone already makes the UI dramatically better. Improvement 2 is a small follow-on to fs_worker.py. Improvement 3 is the most complex but is fully self-contained in faceswap.py and fs_worker.py with no UI changes required.
