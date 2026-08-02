# Media Player Overhaul & Native AV1 FFmpeg Backend

- **Native AV1 / FFmpeg Playback:** Added `qputenv("QT_MEDIA_BACKEND", "ffmpeg")` in `main.cpp`. Bypasses Windows Media Foundation system codecs and uses our bundled FFmpeg engine for native AV1, VP9, WebM, and MKV video playback.
- **Flat Vector Media Icons (`MediaIcon.qml`):** Created a dedicated Canvas icon component rendering solid flat white Play, Pause, Speaker Enabled, Speaker Muted, and Rotate icons. Eliminates OS text/emoji rendering bugs.
- **Volume Slider Hover Fix:** Introduced a non-blocking `combinedHoverArea` covering both the speaker button and popup slider region so mouse movement into the popup slider never triggers premature menu auto-close.
- **Slider Track Styling:** Custom styled both volume and video timeline sliders so active progress (elapsed time / volume level) is filled in solid white (`#ffffff`), while inactive track is dark grey (`#40ffffff`).
- **Rotation Reference Fix:** Added explicit `id: videoContainer` in `PhotoViewer.qml` to fix rotation property scoping.
