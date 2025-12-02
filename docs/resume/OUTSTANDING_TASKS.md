# Outstanding Tasks & Roadmap

## 🐛 BUG FIXES (Quick Wins)

### BUG-001: Startup Null Reference Warning
**File**: `resources/qml/Main.qml` lines 274-275  
**Error**: `TypeError: Cannot read property 'isApiSupported' of null`  
**Cause**: QML accesses `appSettings` before context property is fully initialized  
**Impact**: Minor - app recovers, but console shows warnings  
**Fix**: Add null check:
```qml
// Old (line 274)
text: modelData.name + ": " + (appSettings.isApiSupported(modelData.value) ? "✅" : "❌")

// New
text: modelData.name + ": " + (appSettings && appSettings.isApiSupported(modelData.value) ? "✅" : "❌")
```
**Estimated Time**: 5 minutes  
**Testing**: Launch app, check no console errors

---

### BUG-002: Hardcoded Test Paths
**Files**: Multiple locations  
**Issue**: References to non-existent paths like "I:/MY SDCards/dir0064.chk"  
**Impact**: Users see errors if they don't have those paths  
**Fix**: 
1. Search codebase for hardcoded paths
2. Replace with proper folder selection dialogs
3. Save last used path in settings

**Estimated Time**: 30 minutes  
**Testing**: Fresh install, no default paths

---

### BUG-003: Crash Log Pollution
**Location**: Project root directory  
**Issue**: 28+ crash log files from development/testing  
**Impact**: Clutters workspace, confusing  
**Fix**: 
- Move logs to `logs/` subdirectory
- Update logging code to use new path
- Add logs/ to .gitignore

**Estimated Time**: 15 minutes  
**Testing**: Generate new crash, verify path

---

## 🎨 FEATURE IMPLEMENTATION (Core)

### FEAT-001: Albums View
**Status**: Placeholder only  
**File**: `resources/qml/AlbumsView.qml`  
**Current**: "Albums Feature Coming Soon" text  
**Requirements**:
1. Display folders as albums
2. Show album thumbnails (first 4 images in grid)
3. Click album to view its contents
4. Use existing AlbumModel from C++

**Subtasks**:
- [ ] Design album grid layout
- [ ] Create AlbumCard.qml component
- [ ] Connect to AlbumModel
- [ ] Implement album detail view
- [ ] Add "back to albums" navigation
- [ ] Handle empty albums

**Estimated Time**: 4-6 hours  
**Complexity**: Medium  
**Dependencies**: AlbumModel.cpp/h (already exists)

---

### FEAT-002: Video Playback
**Status**: Placeholder thumbnails only  
**Current**: Video files show play icon but don't play  
**Requirements**:
1. Integrate QtMultimedia VideoOutput
2. Add video player controls (play/pause/seek)
3. Generate video thumbnails
4. Handle video formats (mp4, mkv, avi, mov)

**Decision Point**: Implement or Remove?
- **Implement**: Full featured gallery (+complexity)
- **Remove**: Keep it simple photo gallery (-scope creep)

**If Implementing**:
- [ ] Create VideoPlayer.qml component
- [ ] Add media controls overlay
- [ ] Integrate with PhotoViewer
- [ ] Test video format support
- [ ] Add video-specific settings

**Estimated Time**: 6-8 hours  
**Complexity**: High  
**Dependencies**: Qt6::Multimedia

---

### FEAT-003: Stories Feature
**Status**: Not implemented  
**Location**: `Main.qml` Tab 2  
**Current**: Placeholder text  

**Decision Needed**: 
- What should "Stories" do? (Instagram-like? Timeline? Photo memories?)
- Is this feature actually wanted?
- Can we repurpose this tab for something else?

**Recommendation**: Defer or remove until use case is clear

---

## ⚡ PERFORMANCE & POLISH

### PERF-001: GPU Monitoring
**Status**: Placeholder values  
**File**: `src/SystemMonitor.cpp`  
**Current**: Returns static "Unknown GPU"  
**Fix**: Implement real GPU detection using:
- Windows: DXGI API
- Cross-platform: QOpenGLContext

**Estimated Time**: 2-3 hours  
**Complexity**: Medium

---

### PERF-002: Memory Leak Detection
**Issue**: Long-running sessions might leak memory  
**Current**: No formal testing  
**Fix**:
- Run with memory profiler (valgrind on Linux, PerfView on Windows)
- Check AsyncImageProvider cache eviction
- Verify QML object destruction

**Estimated Time**: 2-4 hours  
**Complexity**: Medium-High

---

### POLISH-001: Build Script Consistency
**Issue**: `build.ps1` references Qt 6.9.3, CMake requires 6.4+  
**Impact**: Confusing for new developers  
**Fix**: Parameterize Qt path in build.ps1, add detection logic

**Estimated Time**: 30 minutes

---

### POLISH-002: User Documentation
**Status**: Missing  
**Need**: 
- User manual (how to use the app)
- Installation guide
- Troubleshooting guide
- FAQ

**Estimated Time**: 3-4 hours

---

## 🔬 TESTING & QUALITY

### TEST-001: Expand Unit Tests
**Current**: Only `tst_imagemodel.cpp` exists  
**Need**:
- AlbumModel tests
- GroupedProxyModel tests
- SettingsHelper tests
- SystemMonitor tests

**Estimated Time**: 4-6 hours

---

### TEST-002: QML Component Tests
**Current**: No QML tests  
**Need**: Tests for critical QML components  
**Tools**: Qt Test QML module

**Estimated Time**: 6-8 hours

---

## 🌍 CROSS-PLATFORM

### PLATFORM-001: Linux Support
**Current**: Windows-only (psapi.lib, Direct3D)  
**Blockers**:
- SystemMonitor uses Windows APIs
- Graphics API selection assumes Windows

**Estimated Time**: 8-12 hours  
**Complexity**: High

---

### PLATFORM-002: macOS Support
**Similar issues as Linux**  
**Additional**: Need to handle macOS bundle structure

**Estimated Time**: 10-15 hours  
**Complexity**: High

---

## 📊 PRIORITY MATRIX

### Do First (High Impact, Low Effort)
1. ✅ BUG-001: Startup null reference (5 min)
2. ✅ BUG-002: Hardcoded paths (30 min)
3. ✅ BUG-003: Crash log cleanup (15 min)
4. ✅ POLISH-001: Build script consistency (30 min)

### Do Next (High Impact, Medium Effort)
5. ⏳ FEAT-001: Albums view (4-6 hours)
6. ⏳ PERF-001: GPU monitoring (2-3 hours)

### Consider Later (Medium Impact)
7. ⏳ FEAT-002: Video playback (decide first!)
8. ⏳ TEST-001: Expand unit tests
9. ⏳ POLISH-002: User documentation

### Future/Maybe (Low Priority)
10. 🔮 FEAT-003: Stories feature
11. 🔮 PLATFORM-001/002: Cross-platform
12. 🔮 TEST-002: QML tests

---

## 📅 SUGGESTED SPRINT PLAN

### Week 1: Bug Fixes & Stability
- [x] Verify current build works
- [ ] Fix all BUG-xxx items
- [ ] Polish build system
- Result: Clean, stable baseline

### Week 2: Albums Feature
- [ ] Design album UI
- [ ] Implement AlbumView.qml
- [ ] Test with real data
- Result: Core feature complete

### Week 3: Polish & Performance
- [ ] GPU monitoring
- [ ] Memory profiling
- [ ] Performance optimization
- Result: Production-ready quality

### Week 4: Documentation & Testing
- [ ] Write user manual
- [ ] Expand test coverage
- [ ] Create demo videos/screenshots
- Result: Shippable product

---

## 🎯 DEFINITION OF DONE

For each task to be considered complete:
- [ ] Code implemented and committed
- [ ] No new compiler warnings
- [ ] No new runtime errors
- [ ] Manual testing passed
- [ ] Documentation updated (if user-facing)
- [ ] Reviewed by another developer (or AI!)

---

## 📝 NOTES FOR NEXT SESSION

1. **Start Fresh**: If AI session gets sluggish, start new conversation
2. **One Task at a Time**: Don't jump between features
3. **Test Before Commit**: Build → Test → Commit, always
4. **Document as You Go**: Update PROGRESS.md with each session
5. **Respect Working Code**: If it ain't broke, don't fix it!

---

**Last Updated**: 2025-12-02  
**Project Status**: ✅ Stable & Functional  
**Ready for Development**: Yes
