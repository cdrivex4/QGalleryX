# 🔍 UI Wiring Audit — main_ui.py Deep Walk

> **Scope:** Line-by-line walkthrough of [main_ui.py](file:///c:/just-dub-it2/app/main_ui.py) + all connection points to [pipeline.py](file:///c:/just-dub-it2/app/pipeline.py), [config.py](file:///c:/just-dub-it2/app/utils/config.py), and stage files.

---

## 🐛 Issues Found: 16

### UI-01: 🔴 Missing `subprocess` import — `perform_folder_open` and `run_selective_preview` crash
**Lines:** [273](file:///c:/just-dub-it2/app/main_ui.py#L273), [287](file:///c:/just-dub-it2/app/main_ui.py#L287), [313](file:///c:/just-dub-it2/app/main_ui.py#L313), [335-336](file:///c:/just-dub-it2/app/main_ui.py#L335-L336)

`main_ui.py` never imports `subprocess` at the top level. Multiple functions use it:
- `perform_folder_open()` calls `subprocess.run(f'explorer /select,...')`
- `run_selective_preview()` does `import subprocess` locally (line 287) — **this works**
- `update_sliders()` calls `subprocess.run(["ffprobe"...])` — **but subprocess is NOT imported at this scope**

`update_sliders` is defined at line 331 INSIDE the `gr.Blocks` context. It doesn't do a local import. When a user uploads a video, this crashes silently.

---

### UI-02: 🔴 `gpu_diarizing` status missing from UI label_map and weights
**Lines:** [65-76](file:///c:/just-dub-it2/app/main_ui.py#L65-L76), [83-93](file:///c:/just-dub-it2/app/main_ui.py#L83-L93), [109](file:///c:/just-dub-it2/app/main_ui.py#L109)

Pipeline sets `job.status = "gpu_diarizing"` in [pipeline.py:312](file:///c:/just-dub-it2/app/pipeline.py#L312). The UI has **no entry** for this status in:
- `weights` dict (line 65) → progress bar shows 0% during entire diarization
- `label_map` dict (line 83) → displays raw `GPU_DIARIZING` instead of a friendly label
- `stage_order` list (line 109) → stage counter shows blank

---

### UI-03: 🔴 `gpu_faceswap` status missing from UI — same issue
**Lines:** [65-76](file:///c:/just-dub-it2/app/main_ui.py#L65-L76), [83-93](file:///c:/just-dub-it2/app/main_ui.py#L83-L93)

Pipeline sets `job.status = "gpu_faceswap"` in [pipeline.py:193](file:///c:/just-dub-it2/app/pipeline.py#L193). Face Lab jobs show a raw status string with 0% progress.

---

### UI-04: 🔴 Download link is broken — `file/{path}` is not a valid Gradio URL
**Line:** [162](file:///c:/just-dub-it2/app/main_ui.py#L162)

```html
<a class='dl-link' href='file/{job.output_path}' ...>💾</a>
```

Gradio serves files via `/file=` not `/file/`. Also, paths with backslashes (Windows) break as URLs. Should be `/file={encoded_path}`.

---

### UI-05: 🟡 `settings.json` path is CWD-relative — fragile
**File:** [config.py:8](file:///c:/just-dub-it2/app/utils/config.py#L8)

```python
SETTINGS_PATH = "settings.json"
```

The CWD depends on how the app is launched. `launch.bat` does `cd /d %~dp0` (project root), then runs `env\Scripts\python.exe app\main_ui.py`. Python's CWD will be the project root. This *works* but if anyone runs `main_ui.py` from the `app/` directory, settings silently fail.

---

### UI-06: 🟡 Mark Start/End buttons use `gr.State()` as dummy input — always returns `None`
**Lines:** [347-358](file:///c:/just-dub-it2/app/main_ui.py#L347-L358)

```python
mark_start_btn.click(
    fn=set_start,
    inputs=[gr.State()],  # Dummy — always None
    outputs=[prev_start],
    js="() => { return getGradioVideoTime('main_video_player'); }"
)
```

The JS function returns the video's currentTime, but `set_start(t)` receives the value from `gr.State()` which is `None`, NOT the JS return value. In Gradio, the `js` parameter runs client-side and its return value replaces the inputs — **but only if the number of inputs matches the JS return values.** This should work because there's 1 input and 1 JS return value. However, `set_start` just does `return t` — if `t` is `None` from the State, the start time resets to None instead of the video position.

The actual Gradio behavior: the JS return value **replaces** the input values before they reach the Python function. So `set_start` receives the JS return (currentTime). **This should work IF the JS actually returns the right value.** But `getGradioVideoTime` tries to find `#main_video_player video` — in Gradio 6.0, the video element may not have the standard `#elem_id video` CSS structure. Need to verify.

---

### UI-07: 🟡 `set_dur` logic is wrong — calculates duration from two inputs but only gets one from JS
**Lines:** [345](file:///c:/just-dub-it2/app/main_ui.py#L345), [354-358](file:///c:/just-dub-it2/app/main_ui.py#L354-L358)

```python
def set_dur(t, start): return max(0.1, t - start)

mark_end_btn.click(
    fn=set_dur,
    inputs=[gr.State(), prev_start],  # 2 inputs
    outputs=[prev_dur],
    js="() => { return getGradioVideoTime('main_video_player'); }"  # Returns 1 value
)
```

JS returns 1 value but the function expects 2 inputs (`t` and `start`). The JS return replaces `gr.State()` (first input) with currentTime. `prev_start` (second input) comes from Gradio normally. So `set_dur(currentTime, start_value)` → `currentTime - start_value` → this is the correct duration. **This actually works as designed.**

---

### UI-08: 🟡 Failed jobs don't show error message anywhere in the UI
**Lines:** [58-172](file:///c:/just-dub-it2/app/main_ui.py#L58-L172)

When `job.status == "failed"`, the table shows a red "FAILED" badge, but `job.error` (which contains the actual exception message) is **never rendered** in the HTML. The user sees "FAILED" with no clue why.

---

### UI-09: 🟡 Preview player has no auto-refresh mechanism
**Lines:** [259-260](file:///c:/just-dub-it2/app/main_ui.py#L259-L260)

The `preview_player` and `preview_player_state` are created but **never connected** to any periodic check. The `playPreview()` JS function directly sets the `<video>` src, which bypasses Gradio's state management. This means:
- The preview player works via raw DOM manipulation (fragile)
- Refreshing the page loses the preview
- No Gradio state tracking

---

### UI-10: 🟡 `video_state` doesn't update properly — Gradio video components return dicts
**Line:** [342](file:///c:/just-dub-it2/app/main_ui.py#L342)

```python
video_in.change(fn=lambda x: x, inputs=[video_in], outputs=[video_state])
```

In Gradio 6.0, `gr.Video` returns a dict `{"video": {"path": "..."}}` not a plain string path. The `run_selective_preview` function at line 303 checks `os.path.exists(source_video)` — this will fail if `source_video` is a dict instead of a path string.

---

### UI-11: 🟡 Job resume logic auto-enqueues on startup — uncontrolled
**Lines:** [462-474](file:///c:/just-dub-it2/app/main_ui.py#L462-L474)

```python
for ws_id in all_ids:
    score = get_ws_score(ws_id)
    if score >= 5:
        ...
        manager.add_job("resumed.mp4", job_id=ws_id)
```

On every server restart, ALL workspace folders with score ≥ 5 get auto-enqueued. This means:
- Half-finished jobs restart automatically (good)
- But previously failed jobs ALSO restart (bad — infinite retry loop)
- The video path `"resumed.mp4"` doesn't exist and `shutil.copy` fails silently
- The `source_basename` is set to `"resumed"` instead of the original filename

---

### UI-12: 🟢 Gradio 6.0 CSS deprecation warning
**Line:** [220](file:///c:/just-dub-it2/app/main_ui.py#L220)

```python
with gr.Blocks(title="Offline AI Studio v2.6.5") as demo:
```

The crash log shows: `UserWarning: The parameters have been moved from the Blocks constructor to the launch() method in Gradio 6.0: css.`
The `css` parameter was already removed from Blocks (good), but `title` may also need to move.

---

### UI-13: 🟢 `config.get()` at UI build time — called during module load
**Lines:** [376-380](file:///c:/just-dub-it2/app/main_ui.py#L376-L380)

```python
value=config.get("translate_device")
```

This is called when the UI is *built*, not when the tab is *opened*. If settings change between app restarts, the UI shows stale values until the page is refreshed.

---

### UI-14: 🟢 No Settings controls for `target_language` or `low_vram_mode`
**Lines:** [372-396](file:///c:/just-dub-it2/app/main_ui.py#L372-L396)

The config has `target_language` and `low_vram_mode` but the Settings tab has no UI controls for them. Users can't change the target language from the dashboard.

---

### UI-15: 🟢 `save_settings` doesn't save `low_vram_mode`, `target_language`, or `translate_disable_cuda_graphs`
**Lines:** [389-393](file:///c:/just-dub-it2/app/main_ui.py#L389-L393)

Only 3 of 7 config keys are saved. The others silently retain their defaults forever.

---

### UI-16: 🟢 `clear_workspace` deletes workspaces with digit-only names but AUDIT_ workspaces survive
**Lines:** [129-137](file:///c:/just-dub-it2/app/pipeline.py#L129-L137)

```python
if os.path.isdir(ws_path) and item.isdigit() and item not in active:
```

Auditor workspaces like `AUDIT_1748777123` don't match `isdigit()` and never get cleaned up.

---

## Summary Table

| ID | Severity | Category | Description |
|----|----------|----------|-------------|
| UI-01 | 🔴 Critical | Missing import | `subprocess` not imported — `update_sliders()` crashes on video upload |
| UI-02 | 🔴 Critical | Missing mapping | `gpu_diarizing` not in UI weights/labels/stage_order |
| UI-03 | 🔴 Critical | Missing mapping | `gpu_faceswap` not in UI weights/labels |
| UI-04 | 🔴 Critical | Broken link | Download button uses wrong URL format (`file/` → should be `/file=`) |
| UI-05 | 🟡 Moderate | Path fragility | `settings.json` path is CWD-relative |
| UI-06 | 🟡 Moderate | Wiring issue | Mark Start/End buttons depend on Gradio JS interop (verify) |
| UI-07 | ✅ OK | Wiring | `set_dur` works correctly as designed |
| UI-08 | 🟡 Moderate | Missing info | Failed jobs show no error message |
| UI-09 | 🟡 Moderate | Dead wiring | Preview player state never used |
| UI-10 | 🟡 Moderate | Type mismatch | `video_state` may contain dict instead of path string |
| UI-11 | 🟡 Moderate | Auto-resume | Failed/crashed jobs auto-enqueue forever on restart |
| UI-12 | 🟢 Low | Deprecation | Gradio 6.0 constructor parameter migration |
| UI-13 | 🟢 Low | Stale state | Settings values cached at UI build time |
| UI-14 | 🟢 Low | Missing control | No UI for `target_language` or `low_vram_mode` |
| UI-15 | 🟢 Low | Incomplete save | Only 3/7 config keys saved |
| UI-16 | 🟢 Low | Cleanup gap | AUDIT_ workspaces never cleaned up |
