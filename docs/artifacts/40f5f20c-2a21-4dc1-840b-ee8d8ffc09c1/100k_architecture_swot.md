# 100k Image Rendering Architecture: SWOT & Alternatives

## 1. SWOT Analysis of Initial Suggestions

### Option A: Monolithic C++ Grid Renderer (Custom `QQuickItem` / `QSGNode`)
*Instead of QML `GridView` creating delegates, we write a C++ item that uses `QSGGeometryNode` to draw the entire grid in a single, massively batched GPU operation.*

| Strengths | Weaknesses |
| :--- | :--- |
| **Absolute Performance:** Bypasses QML object instantiation entirely. One draw call can render thousands of quads. <br> **Memory Control:** We manage the textures and vertex data directly in C++, ensuring zero garbage collection pauses. | **Complexity:** Re-inventing the wheel. We lose all built-in QML features (animations, styling, layout margins). <br> **Interaction:** Hit-testing (finding which image was clicked) must be manually calculated using 2D math. |

| Opportunities | Threats |
| :--- | :--- |
| **D3D11/Vulkan Integration:** Can bypass Qt's RHI and write custom shaders if needed (e.g., for hardware-accelerated DNG demosaicing directly on the grid). | **Development Time:** High risk of getting bogged down in low-level OpenGL/QSG math instead of shipping features. |

---

### Option B: Spatial Indexing (QuadTree/R-Tree) + Minimal QML
*C++ maintains the 100,000 items in a QuadTree. As the user scrolls, C++ rapidly calculates intersections and only exposes the exact visible subset to QML.*

| Strengths | Weaknesses |
| :--- | :--- |
| **Scalability:** The math to find visible items in a QuadTree is $O(\log n)$. It can handle millions of items in memory. <br> **QML Preservation:** We still get to use QML delegates and animations for the visible items. | **Rapid Scroll Thrashing:** If the user grabs the scrollbar and yanks it, the tree must calculate intersections and QML must instantiate/destroy delegates extremely rapidly, causing spikes. |

| Opportunities | Threats |
| :--- | :--- |
| **Semantic Zoom Clustering:** A QuadTree naturally clusters data. When zoomed out, a parent node can represent a "month" of photos, allowing us to swap individual images for aggregated textures. | **Main Thread Blocking:** If the intersection math or QML delegate binding takes longer than 16ms, the UI stutters. |

---

## 2. Three Alternative Architectures

### Alternative 1: Texture Atlasing (The "Video Game" Approach)
Instead of handing QML 100,000 individual 3KB textures, the C++ backend dynamically "bakes" visible clusters of thumbnails into a single massive texture atlas (e.g., a 4K texture containing an 8x8 grid of photos).
*   **Pros:** Drastically reduces texture bindings and draw calls. QML only renders a few large `Image` items.
*   **Cons:** High VRAM usage. Baking textures on the fly requires fast GPU memory transfers.

### Alternative 2: Instanced Rendering via Qt3D / Custom OpenGL Widget
Ditch QML for the actual gallery grid. Use `QOpenGLWidget` (or Vulkan equivalent) and write a raw shader that uses **Hardware Instancing** (rendering the exact same quad 100,000 times, just passing an array of coordinates and texture IDs).
*   **Pros:** The undisputed king of raw performance. This is how particle systems render 1 million particles at 60 FPS.
*   **Cons:** A complete paradigm shift. UI overlays (selection checkboxes, video duration badges) become much harder to implement over a raw 3D context.

### Alternative 3: Data Decimation & Level of Detail (LOD)
If 100,000 images are on screen at once, each image is smaller than a pixel. We stop trying to render images at that zoom level. Instead, we render statistical heatmaps or dominant color blocks. We only load actual textures when the user zooms in to a manageable subset (e.g., < 1,000 visible).
*   **Pros:** Solves the problem by acknowledging the limits of human perception and screen resolution. Extremely fast.
*   **Cons:** Doesn't technically "render 100k images." Might feel like a compromise if the user expects to see microscopic thumbnails.

---

## 3. Dependency & System Impact Analysis

Whichever path we choose will cause a ripple effect across the codebase. Here is what we must consider to protect the frozen legacy app and ensure stability:

1.  **Isolation (Protecting `appSamsungGallery`):**
    *   **Impact:** If we rewrite the `ImageModel` or `AsyncImageProvider` to support these massive scales, we will break the legacy app.
    *   **Mitigation:** All new data structures (e.g., QuadTrees, Atlas Generators, custom QSG nodes) **must** be created exclusively inside the `test_scrollbench/src/` directory. The legacy app should not link against them.

2.  **`AsyncImageProvider` Overhaul:**
    *   **Impact:** Currently, QML *pulls* images from the provider via `image://async/`. If we use a custom C++ renderer or atlasing, C++ will need to *push* textures directly to the GPU.
    *   **Mitigation:** We will likely need to write a new `BatchImageProvider` in ScrollBench that pre-loads textures into GPU memory, bypassing Qt's standard image provider scheme entirely.

3.  **Interaction & Selection:**
    *   **Impact:** ScrollBench currently has a robust `Ctrl+Click` / Shift-Select system based on QML `MouseArea`. If we use a C++ monolithic renderer or raw OpenGL, QML won't know where the individual images are.
    *   **Mitigation:** We will need to implement a "Raycasting" or 2D Hit-Test function in C++ that takes the `(x, y)` mouse coordinates from QML and mathematically determines which of the 100,000 items was clicked.

4.  **TaskScheduler Thread Pools:**
    *   **Impact:** Decoding 100,000 images, even at tiny resolutions, will flood the `CPU_BOUND` thread pool.
    *   **Mitigation:** We must implement "Cancellable Batches." If the user scrolls past 50,000 images in one swipe, the `TaskScheduler` must be able to instantly drop those decode requests rather than processing them.
