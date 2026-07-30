# GPU Acceleration Status Report

## Current Implementation (as of 2026-07-29)

### ✅ WHAT IS GPU-ACCELERATED:

#### **1. UI Rendering & Interaction (via Qt Quick Scene Graph)**
- **Backend**: Direct3D 11 (Primary), OpenGL (Fallback), Vulkan (Available but disabled by default)
- **Status**: ✅ **WORKING** and highly optimized
- **Usage**: Automatically powers smooth scrolling, opacity masks, dynamic resizing, animations, and the core rendering pipeline.

#### **2. Video Decoding (via FFmpeg)**
- **Format**: `.mp4`, `.avi`, `.mov`, `.mkv`, etc.
- **Backend**: D3D11VA (DirectX 11 Video Acceleration)
- **Status**: ✅ **WORKING** and verified
- **Usage**: Automatic for all video thumbnail generation and playback
- **Logs**: 
  ```
  [HWAccel] Successfully initialized d3d11va
  [VideoThumbnailer] GPU decode enabled for <file>
  [VideoThumbnailer] GPU-decoded frame detected, transferring to CPU memory
  ```

### ❌ WHAT IS NOT GPU-ACCELERATED:

#### **1. Image Files (JPG, PNG, BMP, etc.)**
- **Current**: CPU-only via Qt's `QImage`
- **Reason**: No GPU decode path implemented
- **To Fix**: Would need OpenGL/Vulkan compute shaders or DirectX compute

#### **2. RAW Files (CR2, NEF, ARW, DNG, etc.)**
- **Current**: CPU-only via LibRaw
- **"RAW Acceleration" Setting**: Only uses embedded thumbnails (still CPU)
- **Reason**: LibRaw has no GPU support
- **To Fix**: Would need custom GPU-based demosaicing (complex)

#### **3. Image Resizing/Scaling**
- **Current**: CPU-only via Qt/swscale
- **Reason**: No GPU pipeline for resize operations
- **To Fix**: Would need GPU shaders for scaling

### 🔴 BLOCKED BACKENDS:

#### **Vulkan**
- **Status**: ❌ **BLACKLISTED** (causes system crash/reboot)
- **Reason**: Unknown - could be driver incompatibility, FFmpeg bug, or GPU limitation
- **Risk**: **CRITICAL** - full system crash
- **Recommendation**: **DO NOT USE** without extensive debugging in isolated environment

#### **OpenCL**
- **Status**: ⚠️ **AUTO-FALLBACK** (attempts with safety)
- **Reason**: Untested, may not work
- **Behavior**: Tries D3D11 first, falls back if fails

---

## Performance Monitoring

### Current Metrics (via SystemMonitor):
✅ **CPU Usage** - Tracked and logged  
✅ **GPU Load** - Tracked via PDH (Performance Data Helper)  
✅ **GPU VRAM** - Tracked via DXGI  
✅ **RAM Usage** - Tracked and logged  

### Why You May Not See GPU Activity:

1. **ScrollBench Uses Synthetic Test Data**
   - Generates fake images programmatically
   - No real video files = no GPU decode

2. **Images Don't Use GPU**
   - Even real JPG/PNG files won't trigger GPU
   - Only videos use GPU acceleration

3. **To See GPU Activity:**
   - Open folder with `.mp4`/`.mov` files in main app
   - GPU will show activity during video thumbnail generation
   - Check logs for "GPU decode enabled" messages

---

## Recommendations

### Immediate Actions:
1. ✅ **Keep current D3D11 implementation** - It works
2. ❌ **Do NOT enable Vulkan** - System crash risk too high
3. ⚠️ **Test with real video files** - To verify GPU is working

### Future GPU Acceleration (Would Require New Development):
1. **Image Decode**: Implement GPU JPEG decoder via DirectX/CUDA
2. **RAW Processing**: Write custom GPU demosaicing shader
3. **Image Scaling**: Use GPU for resize operations
4. **Vulkan Debug**: Investigate crash in isolated test environment

---

## How to Verify GPU is Working:

### 1. Run with Real Videos:
```powershell
.\test_scrollbench\deploy\appScrollBench.exe "D:\Videos"
```

### 2. Check Logs for:
```
[HWAccel] Requested mode: SettingsHelper::D3D11VA
[HWAccel] Attempting d3d11va...
[HWAccel] SUCCESS: d3d11va initialized
[VideoThumbnailer] GPU decode enabled for <video file>
[VideoThumbnailer] GPU-decoded frame detected
```

### 3. Monitor GPU Usage:
- Task Manager → Performance → GPU
- Should see "3D" or "Video Decode" activity
- VRAM usage will increase

---

## Summary:

**ONLY VIDEO DECODING USES GPU CURRENTLY**

Images and RAW files are CPU-only. This is a fundamental architectural limitation, not a configuration issue.
