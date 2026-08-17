// Update wiring_diagrams.md with deep-dive connections from AsyncImageProvider
// ... (keeping previous content)

#### **4. The Image Loading Pipeline (The "Pixel Engine")**
This diagram illustrates the lifecycle of an image request, from a QML `Image` component to its final delivery from the cache or disk.

```mermaid
graph TD
    subgraph UI_Request [QML Request]
        Img[QML Image Component] -->|requests id/size| AIP[AsyncImageProvider]
    end

    subgraph Cache_Check [Step 1: Fast Path]
        AIP -->|Check Key| RAM[QCache - RAM]
        AIP -->|Check Key| Disk[FileCacheManager - Disk]
    end

    subgraph Heavy_Processing [Step 2: Slow Path / Threaded]
        Disk -- "If Miss" --> TS[TaskScheduler]
        TS -->|CPU_BOUND Task| RAW[LibRaw Decoder]
        TS -->|CPU_BOUND Task| VID[VideoThumbnailer]
        TS -->|CPU_BOUND Task| STD[QImageReader - Standard]
    end

    subgraph Delivery [Step 3: Return to UI]
        RAW -->|Decode Result| Response[AsyncImageResponse]
        VID -->|Decode Result| Response
        STD -->|Decode Result| Response
        Response -->|handleDone via QueuedConnection| Img
    end
```

#### **5. The Precache & Virtualization Loop**
(Already exists in file...)
