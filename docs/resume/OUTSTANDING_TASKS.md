# Outstanding Tasks & Known Issues

**Last Updated:** February 10, 2026

## 🔴 High Priority

### Concurrency Leak & Stall Recovery (Feb 10, 2026)
**Status:** Identified, pending implementation  
**Issue:** `AsyncImageProvider` has a double-increment bug in `activeWeight` and lacks a timer to trigger `checkStalls()`.  
**Impact:** UI may freeze or starve when loading from slow/stalled network shares.  
**Files:** `src/AsyncImageProvider.cpp`

### DNG Proprietary Compression Support ⏸️ DEFERRED
**Status:** Paused for future optimization  
**Issue:** DNG files with `PhotometricInterpretation=32803` take 120+ seconds to load  
**Root Cause:** LibRaw CPU-only processing, no GPU acceleration for Bayer demosaicing  
**Current Workaround:** Users should use corresponding JPG files  
**Future Solutions:**
1. Show embedded JPEG preview immediately, background-load full RAW
2. Implement GPU-accelerated demosaicing (custom shaders)
3. Cache pre-processed DNG files to disk
4. Detect problematic DNGs and skip with user notification

**Files Involved:**
- `src/AsyncImageProvider.cpp` (lines 378-415) - LibRaw processing
- `test_scrollbench/qml/PhotoViewerScrollBench.qml` - RAW loading strategy

**Performance Impact:**
- Current: 120+ seconds load time, FPS drops to 27
- Target: <3 seconds (preview) + background processing

---

## 🟡 Medium Priority

### Video Playback Polish
**Status:** Functional but needs refinement  
**Items:**
- Implement hardware-accelerated decoding validation
- Add codec support detection (H.265, AV1, VP9)
- Test HEVC rotation metadata handling
- Validate audio sync on all video formats

### Album View Integration
**Status:** Incomplete  
**Files:** `test_scrollbench/qml/AlbumsViewScrollBench.qml`  
**Needs:**
- Wire up `ScrollBenchImageModel` correctly
- Test grouping and filtering
- Verify performance with large albums

---

## 🟢 Low Priority / Future Enhancements

### Format Support Expansion
**Items:**
- Test all 170+ formats in compatibility matrix
- Validate HEIC support on Windows
- Test edge cases for exotic formats (MNG, TGA, etc.)
- Document any unsupported format edge cases

### Performance Optimization
**Items:**
- Profile LibRaw processing pipeline
- Investigate multi-threaded Bayer demosaicing
- Optimize memory usage during RAW processing
- Reduce VRAM spikes during rapid zooming

### UI Polish
**Items:**
- Loading spinner for slow RAW files
- Progress indicator for background processing
- Better error messages for unsupported formats
- "Processing..." overlay for DNG files

---

## ✅ Recently Completed

### Media Loading & Type Detection (Dec 25, 2024)
- ✅ Centralized file type detection (`FileTypeRouter`)
- ✅ Fixed video/RAW badges in grid and semantic views
- ✅ Corrected role ID mappings across all QML views
- ✅ Photo Viewer image loading for standard formats
- ✅ Comprehensive format compatibility documentation (170+ formats)

### Previous Sessions
See `SESSION_HISTORY.md` for details on earlier work.

---

## 🐛 Known Issues

### Minor Bugs
1. **Date Scrubber year markers** - May not display for very large collections (>10k items)
2. **Semantic view grouping** - Week grouping may show incorrect boundaries
3. **Thumbnail memory** - Occasional VRAM spike when scrolling rapidly

### Build Warnings
- Qt policy QTP0004 warnings (cosmetic, can be ignored)
- clangd lint errors on FileTypeRouter (IDE only, compiles fine)

---

## 📋 Testing Checklist

When revisiting DNG support:
- [ ] Test standard DNG (non-proprietary) - should load in 2-3 seconds
- [ ] Test Apple ProRAW DNG - verify preview extraction
- [ ] Test Android mobile DNG - validate color accuracy
- [ ] Test proprietary compression DNG - measure load time
- [ ] Profile CPU usage during demosaicing
- [ ] Test FPS impact on various hardware
- [ ] Validate memory leak during repeated DNG loads

---

## 🔗 Related Documentation

- **Format Compatibility:** `/docs/FORMAT_COMPATIBILITY.md`
- **Session History:** `/docs/resume/SESSION_2024-12-25.md`
- **Code Architecture:** `/docs/ARCHITECTURE.md`
- **Threading Model:** `/docs/THREAD_HIERARCHY.md`

---

**Note:** This document is living - update as tasks are completed or new issues discovered.
