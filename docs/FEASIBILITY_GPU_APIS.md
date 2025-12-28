# Feasibility Assessment: GPU API Support in ScrollBench

## Investigation Results

### What the Main App Actually Has:

**Lines 46-56 in `src/main.cpp`:**
```cpp
SettingsHelper tempHelper;
int api = tempHelper.selectedApi();

if (api == 1)
    QQuickWindow::setGraphicsAPI(QSGRendererInterface::Direct3D11);
else if (api == 2)
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Vulkan);
else if (api == 3)
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
else if (api == 4)
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);
```

### ❗ CRITICAL FINDING:

**This is NOT for image/video decode acceleration!**

This code sets the **Qt Scene Graph (QSG) rendering backend** - which controls how QML UI elements are rendered:
- **Direct3D11**: QML rendered via DirectX
- **Vulkan**: QML rendered via Vulkan  
- **OpenGL**: QML rendered via OpenGL
- **Software**: CPU-based QML rendering

**This has ZERO impact on:**
- Image decode (JPG/PNG/RAW)
- Video decode  
- Image processing

---

## What We Actually Found:

### GPU Acceleration Architecture:

| Component | Main App | ScrollBench | Purpose |
|-----------|----------|-------------|---------|
| **QSG Graphics API** | ✅ Configurable (D3D11/Vulkan/OpenGL) | ❌ Uses default | QML UI rendering |
| **VideoThumbnailer (FFmpeg)** | ✅ D3D11VA | ✅ D3D11VA | Video decode |
| **AsyncImageProvider** | ✅ CPU only | ✅ CPU only | Image decode |
| **LibRaw (RAW files)** | ✅ CPU only | ✅ CPU only | RAW decode |
| **HardwareAccelerationManager** | ✅ Shared | ✅ Shared | Video decode backend |

---

## Feasibility Assessment:

### 1. **Port QSG Graphics API Selection to ScrollBench**

**Difficulty**: ⭐ Very Easy  
**Time**: 5 minutes  
**Value**: Low (only affects QML rendering, not image processing)

**Implementation:**
```cpp
// Add to main_scrollbench.cpp before QGuiApplication creation:
SettingsHelper tempHelper;
int api = tempHelper.selectedApi();
if (api == 1)
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Direct3D11);
// ... etc
```

**Impact:**
- Faster/smoother QML UI rendering  
- **Does NOT accelerate image/video processing**
- May help with large grid scrolling performance

---

### 2. **Add GPU Image Decode (What You Actually Want)**

**Difficulty**: ⭐⭐⭐⭐⭐ Very Hard  
**Time**: Days to weeks  
**Value**: High (significant performance boost)

**Why It's Hard:**
1. **No existing GPU JPEG decoder in Qt/FFmpeg**
   - Would need custom DirectX/CUDA implementation
   - Or use nvJPEG (NVIDIA only)

2. **RAW files extremely complex**
   - LibRaw has no GPU support
   - Custom demosaicing shader needed
   - Different algorithm per camera

3. **Image pipeline refactor required**
   - Replace `QImage` loading
   - Implement GPU texture upload
   - Handle color space conversion on GPU

**Possible Approaches:**

#### Option A: NVIDIA nvJPEG (NVIDIA GPUs only)
- ✅ Industry-standard, proven
- ✅ Handles JPEG decode on GPU
- ❌ Requires CUDA
- ❌ NVIDIA only (locks out AMD/Intel)
- **Feasibility**: Medium (if you accept NVIDIA-only)

#### Option B: DirectX Compute Shaders
- ✅ Works on all DirectX 11+ GPUs
- ✅ Already have D3D11 context
- ❌ Have to write JPEG decoder from scratch
- ❌ Complex implementation
- **Feasibility**: Low (too much work)

#### Option C: Vulkan Compute (if we fix crashes)
- ✅ Cross-vendor (NVIDIA/AMD/Intel)
- ✅ Modern, performant
- ❌ Vulkan crashes your system currently
- ❌ Still need custom decoder
- **Feasibility**: Very Low (crash risk + complexity)

#### Option D: Hybrid Approach
- Use GPU for video (already working)
- Use GPU for QML rendering (easy to add)
- **Keep images CPU-based** (Qt is already optimized)
- **Feasibility**: ⭐⭐⭐⭐⭐ Recommended

---

## Recommendations:

### ✅ DO THIS (Quick Wins):

1. **Add QSG API selection to ScrollBench** (5 min)
   - Improves QML rendering
   - Very low risk

2. **Verify D3D11VA video decode is working** (Already done)
   - Test with real video files
   - Confirm GPU usage in Task Manager

3. **Optimize CPU-based image loading**
   - AsyncImageProvider already has caching
   - TaskScheduler already manages threads
   - This is probably "fast enough"

### ❌ DON'T DO THIS (Not Worth It):

1. **GPU JPEG decode**
   - Massive effort
   - Marginal benefit (Qt QImage is already fast)
   - Would only help with very large JPEGs

2. **GPU RAW processing**
   - Extremely complex
   - Diminishing returns
   - LibRaw CPU path is mature and tested

3. **Fix Vulkan for QML**
   - High crash risk
   - D3D11 already works fine
   - Not worth debugging time

---

## Bottom Line:

**The main app does NOT have GPU image decode.**

Both apps use:
- ✅ GPU for video (D3D11VA via FFmpeg) - **Working**
- ❌ CPU for images/RAW - **By design, not a bug**

The only difference is QSG graphics API selection (QML UI rendering), which is trivial to port but has minimal impact.

**My recommendation**: Accept current architecture, focus on other features.
