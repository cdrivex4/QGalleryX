# Known Issues & Limitations (v2.1.0)

## ⚠️ Critical / High Priority

### 1. Memory Usage under Heavy Load
-   **Issue**: The application does not currently have a strict hard limit on RAM usage. The previous 2.5GB enforcement was reverted due to causing UI freezes (Mutex contention).
-   **Impact**: On systems with limited RAM (<8GB), opening thousands of high-res images quickly *might* lead to high memory consumption.
-   **Workaround**: Clean the cache manually via restart if needed, or rely on OS paging.
-   **Status**: **Open**. Needs a background Garbage Collector thread instead of synchronous enforcement.

### 2. Video Playback
-   **Issue**: Video playback logic is largely placeholder. While the *thumbnail generation* is robust (D3D11 accelerated), the actual *player* view is minimal.
-   **Impact**: Videos may not play or only show the first frame in the full viewer.
-   **Status**: **Planned** for v3.0.

## 🔸 Moderate Priority

### 3. Network Performance
-   **Issue**: While network paths (`\\Server\Share`) are supported and working, listing directories with thousands of files over WiFi can be slow.
-   **Impact**: Initial loading of a large network folder might temporarily block the UI or show a loading spinner.
-   **Status**: **Known**. Future optimization: Pre-fetch meta-data.

### 4. Build Script Paths
-   **Issue**: `build.ps1` contains hardcoded paths to Qt 6.9.3 (e.g., `D:\Qt\6.9.3\...`).
-   **Impact**: Building on a different machine requires editing the script variables.
-   **Status**: **WontFix** for this sprint; intended for the specific development environment.

## ✅ Recent Fixes (v2.1.0)
-   **Fixed**: "Missing DLL" errors when running from network shares (Added MinGW runtime deployment).
-   **Fixed**: Application freeze when loading PNGs/JPEGs (Removed `CoInitialize` from worker threads).
-   **Fixed**: UI Unresponsiveness during heavy load (Improved Re-queue logic and removed synchronous memory check).
