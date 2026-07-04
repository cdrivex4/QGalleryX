# 🔬 UI Architecture Deep Dive & Recovery Plan

## Root Cause: Why Everything Broke

> [!CAUTION]
> **The environment is catastrophically drifted.** When I ran `pip install insightface` earlier, it triggered a **dependency cascade** that silently upgraded core packages far beyond their pinned versions. The `requirements.txt` pins are no longer enforced.

### Actual vs. Expected Package Versions

| Package | `requirements.txt` Pin | Actually Installed | Breaking? |
|---------|----------------------|-------------------|-----------|
| **gradio** | `4.44.0` | **`6.11.0`** | 🔴 **CRITICAL** — completely different API |
| **torch** | `2.3.1+cu121` | **`2.4.1+cu121`** | 🟡 Minor — mostly compatible |
| **scipy** | `1.13.0` | **`1.17.1`** | 🟡 Minor — mostly compatible |
| **onnxruntime-gpu** | `1.17.1` | `1.17.1` ✅ | ✅ Fine (after fix) |
| **insightface** | *(missing)* | `1.0.1` ✅ | ✅ New — working |

**The Gradio jump from 4.44 → 6.11 is the single biggest cause of every symptom you're seeing:**
- Buttons not responding → Gradio 6 changed component data models
- File uploads failing → `gr.Video` returns `FileData` objects, not dicts
- UI unresponsive → Gradio 6 changed the internal queue/SSE architecture
- `gr.update()` behaving differently → dict-based in v4, object-based in v6

---

## Current Architecture (As-Is)

```mermaid
graph TB
    subgraph "Single Python Process"
        UI["Gradio Web Server<br/>(main_ui.py)"]
        PM["PipelineManager<br/>(pipeline.py)"]
        W["Worker Thread<br/>(daemon)"]
        Q["Queue<br/>(threading.Queue)"]
    end

    subgraph "GPU Stages"
        S1["Demucs<br/>Stem Split"]
        S2["Whisper<br/>Transcribe"]
        S3["LLM<br/>Translate"]
        S4["XTTS<br/>TTS"]
        S5["MuseTalk<br/>Lipsync"]
        FS["InsightFace<br/>FaceSwap"]
    end

    User -->|HTTP :7860| UI
    UI -->|add_job()| PM
    PM --> Q
    Q -->|queue.get()| W
    W --> S1 --> S2 --> S3 --> S4 --> S5
    W --> FS
    UI -->|gr.Timer(5s)| PM
    PM -->|job.status / progress_percent| UI
```

### Architecture Problems

| Problem | Impact | Severity |
|---------|--------|----------|
| **Single process** — UI + AI worker share Python GIL | GPU-heavy stages starve UI thread | 🔴 High |
| **Polling-based updates** — `gr.Timer(5s)` rebuilds full HTML table | Wasteful; generates large DOM diffs every 5s even when idle | 🟡 Medium |
| **Global mutable cache** — `_LAST_STATUS_HTML` comparison for dedup | `gr.update()` semantics changed between Gradio 4 and 6 | 🔴 High |
| **HTML status board** — Server-rendered HTML with inline JS | Buttons use DOM hacks (`querySelector`, `dispatchEvent`) to proxy clicks to hidden Gradio components | 🟡 Medium |
| **No error boundary** — Worker crash = silent failure | User sees frozen progress bar, no error toast | 🟡 Medium |

---

## 5 Recovery Options

---

### Option 1: 🔧 "Pin & Repair" (Fix the Environment)

**What:** Downgrade Gradio back to 4.44.0, revert all code changes I made, keep insightface.

**Wiring:**
```
requirements.txt (pinned) → pip install --force-reinstall → Gradio 4.44.0
main_ui.py → revert extract_path / fs_stat_box changes
faceswap.py → keep as-is (was already working with Gradio 4)
```

**Implementation:**
1. `pip install gradio==4.44.0` (force downgrade)
2. `pip install torch==2.3.1+cu121 --index-url https://download.pytorch.org/whl/cu121` 
3. Revert `main_ui.py` to the version before my changes (git checkout or manual)
4. Keep `insightface` + `buffalo_l` models (they work independently)

#### SWOT

| | |
|---|---|
| **Strengths** | Fastest fix (~15 min). Returns to known-working state. Zero risk to pipeline logic. |
| **Weaknesses** | Doesn't solve the underlying GIL contention issue. Face Lab still needs its own status board. Gradio 4 is EOL. |
| **Opportunities** | Buys time to plan a proper upgrade later. Can layer improvements incrementally. |
| **Threats** | Future `pip install` of any package could cascade again. Gradio 4 will stop getting security patches. |

**Effort:** ⭐ (1-2 hours)
**Risk:** ⭐ (Very Low)

---

### Option 2: 🔄 "Gradio 6 Migration" (Embrace the Upgrade)

**What:** Keep Gradio 6.11 but properly migrate all UI code to use its new API.

**Wiring:**
```
main_ui.py → Rewrite video/image handlers for FileData objects
           → Replace gr.update() dict returns with component updates
           → Use gr.Timer with proper Gradio 6 event system
           → Use native gr.Progress() for real-time progress (new in v5+)
pipeline.py → No changes needed (backend is framework-agnostic)
```

**Key Gradio 6 Changes to Adopt:**
- `gr.Video` returns `FileData` objects with `.path` attribute (not dicts)
- `gr.update()` is deprecated — return component values directly
- `gr.Timer` works differently with the new reactive event system
- `gr.Progress()` provides native progress bars without custom HTML
- SSE (Server-Sent Events) replace long-polling for real-time updates

#### SWOT

| | |
|---|---|
| **Strengths** | Modern stack. Native progress bars. Better SSE performance. Future-proof. |
| **Weaknesses** | Largest code change (~200 lines in `main_ui.py`). Testing surface is wide. May uncover new incompatibilities with other pinned packages. |
| **Opportunities** | Gradio 6 has better file handling, native streaming, and improved queue management — all directly relevant to your use case. |
| **Threats** | Other pinned packages (demucs, coqui-tts) may conflict with Gradio 6's dependency tree. Unknown unknowns. |

**Effort:** ⭐⭐⭐ (4-6 hours)
**Risk:** ⭐⭐⭐ (Medium-High)

---

### Option 3: 🧩 "Process Isolation" (Separate UI from Pipeline)

**What:** Run the pipeline worker as a **separate Python process** communicating via a lightweight JSON file or SQLite database. The Gradio UI only reads status, never shares a GIL with AI work.

**Wiring:**
```
┌─────────────────────┐     ┌──────────────────────┐
│  Process A: Gradio  │     │  Process B: Worker   │
│  (main_ui.py)       │◄───►│  (worker.py)         │
│  - Serves UI        │     │  - Runs AI stages    │
│  - Reads status.json│     │  - Writes status.json│
│  - Enqueues jobs    │     │  - Reads jobs.json   │
└─────────────────────┘     └──────────────────────┘
         ▲                           │
         │    shared filesystem      │
         └───── jobs.json ◄──────────┘
                status.json
```

**Implementation:**
1. Extract `PipelineManager._worker()` into standalone `worker.py`
2. Jobs communicated via `workspace/jobs.json` (write from UI, read from worker)
3. Status communicated via `workspace/status.json` (write from worker, read from UI)
4. `launch.bat` starts both processes
5. UI polls `status.json` instead of shared memory

#### SWOT

| | |
|---|---|
| **Strengths** | **Completely eliminates GIL contention.** UI is always responsive regardless of GPU load. Each process can be restarted independently. Worker crashes don't kill the UI. |
| **Weaknesses** | Significant refactor. File-based IPC has ~100ms latency (fine for 5s polling). Need to handle both processes in `launch.bat`. |
| **Opportunities** | Opens path to running worker on a different machine (network IPC). Can add multiple workers for parallel jobs. |
| **Threats** | File locking on Windows can be tricky. Need to handle partial writes. More moving parts = more failure modes. |

**Effort:** ⭐⭐⭐⭐ (8-12 hours)
**Risk:** ⭐⭐⭐ (Medium)

---

### Option 4: 🌐 "FastAPI + Vanilla JS" (Replace Gradio Entirely)

**What:** Replace Gradio with a lightweight FastAPI backend + a single `index.html` with vanilla JavaScript. The HTML is fully static; all updates come via `/api/status` polling or WebSocket.

**Wiring:**
```
┌──────────────────────┐
│  FastAPI Server      │
│  /api/status         │◄──── JS fetch() every 3s
│  /api/upload         │◄──── <input type="file">
│  /api/faceswap       │◄──── fetch() POST
│  /api/download/{id}  │
│                      │
│  PipelineManager     │
│  (same as today)     │
└──────────────────────┘
         ▲
         │
┌────────┴─────────────┐
│  Static index.html   │
│  - Vanilla JS        │
│  - CSS (your current │
│    design system)    │
│  - No framework      │
└──────────────────────┘
```

**Implementation:**
1. Create `server.py` with FastAPI routes
2. Create `static/index.html` with tabs, forms, progress bars
3. Move status rendering to client-side JS (JSON → DOM)
4. Keep `pipeline.py` and all stages untouched
5. File uploads via standard `multipart/form-data`

#### SWOT

| | |
|---|---|
| **Strengths** | **Total control over UI behavior.** No Gradio version conflicts ever again. Lightweight — FastAPI + uvicorn is ~2MB vs Gradio's ~50MB. Sub-second responsiveness. Client-side rendering is instant. |
| **Weaknesses** | Must rebuild the entire UI from scratch. Lose Gradio's built-in video player, image editor, and file handling. More initial work. |
| **Opportunities** | Can use WebSocket for true real-time progress (no polling). Can add a mobile-friendly layout. Can package as Electron app later. |
| **Threats** | Maintaining a custom UI long-term requires more effort than using a framework. Need to handle CORS, file serving, etc. manually. |

**Effort:** ⭐⭐⭐⭐⭐ (16-24 hours)
**Risk:** ⭐⭐ (Low-Medium — well-understood tech)

---

### Option 5: 🏗️ "Hybrid" (Pin Gradio 4 + Process Isolation for FaceSwap Only)

**What:** Best of Options 1 and 3. Pin Gradio back to 4.44.0 for the main UI (which was already working), but isolate the Face Swap pipeline into a separate worker process so it can't destabilize the UI.

**Wiring:**
```
┌───────────────────────────────────────────┐
│  Main Process (Gradio 4.44.0)             │
│  ┌─────────────┐  ┌────────────────────┐  │
│  │ Production   │  │ Face Lab Tab       │  │
│  │ Hub Tab      │  │ - Upload face      │  │
│  │ (working)    │  │ - Select video     │  │
│  └──────┬───────┘  │ - Start job ──────►│──┼──► writes job to
│         │          └────────────────────┘  │    workspace/fs_jobs/
│  PipelineManager                          │
│  (translation jobs only)                  │
└───────────────────────────────────────────┘
                                               ┌──────────────────┐
                                               │ FaceSwap Worker  │
                                               │ (fs_worker.py)   │
                                               │ - Watches fs_jobs│
                                               │ - Runs InsightFace│
                                               │ - Writes status  │
                                               └──────────────────┘
```

**Implementation:**
1. `pip install gradio==4.44.0` (downgrade)
2. Remove `fs_stat_box` and my broken caching changes
3. Create `fs_worker.py` — a standalone script that watches `workspace/fs_jobs/` 
4. Face Lab tab writes a job spec JSON, worker picks it up
5. Face Lab tab polls `workspace/fs_jobs/{id}/status.json` for progress
6. `launch.bat` starts both `main_ui.py` and `fs_worker.py`

#### SWOT

| | |
|---|---|
| **Strengths** | Restores the working Production Hub immediately. FaceSwap gets its own process (no GIL fights). Incremental — doesn't touch existing pipeline. |
| **Weaknesses** | Two-process management. Face Lab UI still limited by Gradio 4's polling model. Slightly more complex deployment. |
| **Opportunities** | Can extend the worker pattern to other stages later (e.g., lipsync). Natural stepping stone to full process isolation (Option 3). |
| **Threats** | Maintaining two different IPC patterns (in-process for translation, file-based for faceswap) adds cognitive overhead. |

**Effort:** ⭐⭐⭐ (4-6 hours)
**Risk:** ⭐⭐ (Low)

---

## Recommendation Matrix

| Option | Effort | Risk | Responsiveness Fix | Future-Proof | Preserves Existing |
|--------|--------|------|-------------------|-------------|-------------------|
| 1. Pin & Repair | ⭐ | ⭐ | ❌ Partial | ❌ | ✅ Best |
| 2. Gradio 6 Migration | ⭐⭐⭐ | ⭐⭐⭐ | ✅ Good | ✅ Best | 🟡 Medium |
| 3. Process Isolation | ⭐⭐⭐⭐ | ⭐⭐⭐ | ✅ Best | ✅ Good | 🟡 Medium |
| 4. FastAPI + Vanilla JS | ⭐⭐⭐⭐⭐ | ⭐⭐ | ✅ Best | ✅ Good | 🟡 Medium |
| **5. Hybrid (Recommended)** | **⭐⭐⭐** | **⭐⭐** | **✅ Good** | **✅ Good** | **✅ Good** |

> [!IMPORTANT]
> **My recommendation is Option 5 (Hybrid)**, but **starting with Option 1 as the immediate first step.** The env must be repaired first regardless of which path you choose. That takes 15 minutes and gets you back to a working state. Then we can layer the FaceSwap process isolation on top.

## Immediate Next Step (Required for ALL Options)

The very first thing we must do, regardless of which option you pick:

```bash
# Fix the environment — force Gradio back to 4.44.0
env\Scripts\pip.exe install gradio==4.44.0 --force-reinstall --no-deps
env\Scripts\pip.exe install torch==2.3.1+cu121 torchaudio==2.3.1+cu121 torchvision==0.18.1+cu121 --index-url https://download.pytorch.org/whl/cu121
```

Then revert `main_ui.py` to remove my broken changes (the `fs_stat_box`, `extract_path`, caching modifications).

**Which option would you like to go with?**
