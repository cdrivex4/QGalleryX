# Interactive Markers & Queue Optimization

We are addressing two points of friction: the manual entry of timestamps and the perceived "slowness" when managing the job queue.

## Proposed Changes

### [MODIFY] [main_ui.py](file:///c:/just-dub-it2/app/main_ui.py)
**Objective 1**: The "Video Marker" Bridge.
**Objective**: "Interactive Scene Preview" UI.
- **Fix the Player**: Resolve the "broken" state of the `gr.Video` component. I will ensure the file paths are correctly cached so the browser doesn't lose the source acquisition.
- **Reuse Mechanism**: I will link the **Start Point** and **Stop Point** Number inputs closer to the player. 
- **Integrated Preview**: Instead of just a download link, I will add a **Live Preview Player** dedicated to showing the Phase 5 "Early Preview". This allows you to watch the dub result immediately without opening VLC.
- **Workflow**: 
    1. Upload/Select video in the acquisition player.
    2. Scrub the player to find your scene.
    3. Input your desired timestamps.
    4. Hit **"🎬 Preview Selective Scene"**.
    5. Watch the results in the **Live Preview Player** right below the ledger.

### [MODIFY] [pipeline.py](file:///c:/just-dub-it2/app/pipeline.py)
**Objective**: Early Mux & Selective Stopping (Verified).
- **Auto-Preview**: Every job already generates a `preview_dub.mp4` immediately after the audio mix.
- **Selective Stop**: If the job is a "Preview Selection" task, the pipeline stops after Phase 5.

## User Review Required
> [!IMPORTANT]
> To enable "setting the start and stop time index" directly *from* the player, I'll provide **Quick Sync** buttons.
> 
> My plan is to add a "📍 Capture Current Time" button if Gradio's internal state allows it, otherwise, I'll keep the Number inputs highly visible and attached to the player for easy manual entry. 
> 
> Does this "Unified Preview Hub" approach satisfy the requirement to reuse the existing mechanism?
