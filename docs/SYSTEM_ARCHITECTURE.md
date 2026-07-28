# System Architecture Overview

## Purpose
This document visualizes the application's architecture, threading model, scan strategies, and data flow to help understand how different components interact and make informed architectural decisions.

---

## 🏗️ High-Level System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         QML UI Layer                            │
│  (GalleryViewTiles.qml, AlbumView.qml, ShareDialog.qml)       │
└────────────────┬────────────────────────────────┬───────────────┘
                 │                                 │
                 ▼                                 ▼
┌────────────────────────────┐    ┌──────────────────────────────┐
│     AlbumModel             │    │  ScrollBenchImageModel       │
│  (Folder enumeration)      │    │  (Image/Video enumeration)   │
│  - Generation ID cancel    │    │  - Generation ID cancel      │
│  - Network-aware scanning  │    │  - Network-aware scanning    │
│  - Progressive updates     │    │  - Progressive updates       │
└─────────┬──────────────────┘    └─────────┬────────────────────┘
          │                                  │
          │                                  ▼
          │                    ┌──────────────────────────────┐
          │                    │   AsyncImageProvider         │
          │                    │  (Thumbnail generation)      │
          │                    │  - LIFO queue                │
          │                    │  - Visible range priority    │
          │                    └─────────┬────────────────────┘
          │                              │
          ▼                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    TaskScheduler (Thread Pool)                  │
│  ┌──────────────────────┐         ┌──────────────────────┐     │
│  │  IO-Bound Threads    │         │  CPU-Bound Threads   │     │
│  │  - File scanning     │         │  - Image decode      │     │
│  │  - Network I/O       │         │  - RAW processing    │     │
│  │  (2 threads)         │         │  (2 threads)         │     │
│  └──────────────────────┘         └──────────────────────┘     │
└─────────────────────────────────────────────────────────────────┘
```

---

## 🔄 Scan Strategy Decision Tree

### Path Type Detection

```
User opens directory
         │
         ▼
┌────────────────────┐
│ Normalize path     │
│ (QUrl → QString)   │
└────────┬───────────┘
         │
         ▼
┌─────────────────────────────────────────┐
│ Is path network?                        │
│ • UNC path (\\server\share)?            │
│ • Mapped drive with network device?     │
└──────┬─────────────────────┬────────────┘
       │ YES                  │ NO
       ▼                      ▼
  [Network Path]        [Local Path]
```

### Network Path Strategy

```
Network Path Detected
         │
         ▼
┌─────────────────────────────────────────┐
│ Disable Incremental Updates             │
│ (Set isNetworkPath = true)              │
└────────┬────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────┐
│ Use Fallback Scanner (QDirIterator)     │
│ • Recursive subdirectory scan           │
│ • NO UI updates during scan             │
│ • Collect ALL results in memory         │
└────────┬────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────┐
│ Single Model Update                     │
│ • Sort collected data                   │
│ • beginResetModel()                     │
│ • Populate model                        │
│ • endResetModel()                       │
└────────┬────────────────────────────────┘
         │
         ▼
     [Complete]
     
WHY: Network I/O is slow and unpredictable. Multiple model 
     updates cause UI thrashing and race conditions.
```

### Local Path Strategy

```
Local Path Detected
         │
         ▼
┌─────────────────────────────────────────┐
│ Is drive letter-based? (C:\, D:\)       │
└────────┬─────────────────────┬──────────┘
         │ YES                 │ NO
         ▼                     ▼
   [Try Fast MFT]      [Use Fallback]
         │
         ▼
┌──────────────────────────────────────────┐
│ FastVolumeScanner (Direct MFT read)      │
│ • 100,000+ files/second                  │
│ • Full volume enumeration                │
│ • Filter by extension                    │
└────────┬─────────────────────────────────┘
         │
    ┌────┴─────┐
    │ SUCCESS? │
    └────┬─────┘
         │ YES                 NO
         ▼                     │
┌────────────────────┐         │
│ Progressive Updates│◄────────┘
│ • Every 50 items   │    [Fallback to QDirIterator]
│ • Every 200 items  │
│ • Every 1000 items │
└────────┬───────────┘
         │
         ▼
    [Complete]

WHY: MFT scanning is instant for local drives. Progressive 
     updates provide immediate user feedback.
```

---

## 📊 Data Flow: From Scan to Display

### Run
```powershell
.\Release\QGalleryXBench.exe
```

### Network Diagnostics (Watchdog)
If running over a network share where execution is unstable or silent crashes occur:
```powershell
.\Release\QGalleryXBenchNet.exe
```
**Note:** `QGalleryXBenchNet.exe` is a temporary debug watchdog. It launches the main app, captures exit codes, holds the terminal open on crash, and dumps `logs/crash.log` contents. This is NOT the main application and should be cleaned up after network diagnostics are completed.

### AlbumModel (Folder Scan)

```
User Navigates to Path
         │
         ▼
scanAlbums(path) called
         │
         ├─→ Increment m_scanGeneration (atomic int)
         │   • Invalidates previous scans
         │
         ├─→ Emit isLoadingChanged(true)
         │
         └─→ QtConcurrent::run([...]) {
                    │
                    ▼
          ┌─────────────────────────┐
          │ Path Type Detection     │
          │ isNetworkPath = ?       │
          └────────┬────────────────┘
                   │
             ┌─────┴─────┐
             ▼           ▼
        [Network]    [Local]
             │           │
             │           ├─→ Try FastVolumeScanner
             │           │   └─→ IF SUCCESS:
             │           │       Incremental updates every 200 albums
             │           │
             │           └─→ IF FAIL: Fallback
             │
             └─→ QDirIterator (Recursive)
                 • Enumerate all subdirectories
                 • Count files per directory
                 • NO incremental updates
                 │
                 ▼
          Final Update to UI
          • Sort albums
          • Single beginResetModel/endResetModel
          • Emit isLoadingChanged(false)
         }
```

### ScrollBenchImageModel (Image Scan)

```
scanDirectory(path) called
         │
         ├─→ Increment m_scanGeneration
         │
         ├─→ beginResetModel() + Clear existing data
         │
         └─→ TaskScheduler::addTask(IO_BOUND) {
                    │
                    ▼
          ┌─────────────────────────┐
          │ Path Type Detection     │
          │ isNetworkPath = ?       │
          └────────┬────────────────┘
                   │
             ┌─────┴─────┐
             ▼           ▼
        [Network]    [Local]
             │           │
             │           ├─→ Try FastVolumeScanner
             │           │   └─→ Incremental updates:
             │           │       • 50 items (if < 200 total)
             │           │       • 200 items (if < 1000 total)
             │           │       • 1000 items thereafter
             │           │
             │           └─→ Fallback: QDirIterator
             │               └─→ Updates at 10, 50, 200, 1000, 
             │                   then every 2000 items
             │
             └─→ Collect ALL items
                 • NO incremental updates
                 │
                 ▼
          QMetaObject::invokeMethod(this, [...]() {
              • Check generation ID
              • Sort by date (descending)
              • Detect bursts (< 2s apart)
              • Single model update
          })
         }
         │
         ▼
   ┌─────────────────────────────────┐
   │ Lazy Thumbnail Loading          │
   │ (Viewport Culling Enabled)      │
   └────────┬────────────────────────┘
            │
            ▼
   updateVisibleRange()
            │
            ├─→ Calculate buffered range (±50 items)
            ├─→ Set VisibleRangeManager paths
            └─→ Request thumbnails for visible items
                         │
                         ▼
              ┌─────────────────────────┐
              │ AsyncImageProvider      │
              │ • LIFO queue            │
              │ • Priority: visible     │
              │ • Decode in background  │
              └────────┬────────────────┘
                       │
                       ▼
              TaskScheduler (CPU_BOUND)
                       │
                       ▼
              Thumbnail delivered to UI
```

---

## 🧵 Threading Model

### Thread Types

| Thread Type | Count | Purpose | Examples |
|------------|-------|---------|----------|
| **Main (UI)** | 1 | QML rendering, model updates, user input | All `QMetaObject::invokeMethod` calls |
| **IO-Bound** | 2 | File system operations, network I/O | `scanDirectory`, `scanAlbums` |
| **CPU-Bound** | 2 | Image decoding, RAW processing | `AsyncImageProvider` decode |
| **QtConcurrent** | N | Folder scanning (AlbumModel) | `QtConcurrent::run` |

### Thread Safety

```
┌─────────────────────────────────────────────────────────────────┐
│                     Thread Safety Mechanisms                    │
├─────────────────────────────────────────────────────────────────┤
│ 1. Atomic Generation IDs                                        │
│    • std::atomic<int> m_scanGeneration                          │
│    • Increment on new scan → invalidates old scans              │
│    • Check before EVERY model update                            │
│                                                                 │
│ 2. QMetaObject::invokeMethod                                   │
│    • ALL model changes on main thread                           │
│    • Qt::QueuedConnection for async                             │
│                                                                 │
│ 3. Copy-on-Write for Data Transfer                            │
│    • QVector<ImageItem> batch = currentData;                   │
│    • Pass copies to main thread                                 │
│    • Avoids concurrent modification                             │
│                                                                 │
│ 4. TaskScheduler Queue Management                             │
│    • LIFO for visible items (newest = highest priority)        │
│    • Thread pool prevents oversubscription                      │
└─────────────────────────────────────────────────────────────────┘
```

---

## 🚀 Performance Optimizations

### 1. Viewport Culling

```
GridView visible range: Items 50-100
         │
         ▼
VisibleRangeManager updated
         │
         ▼
AsyncImageProvider
         │
         ├─→ HIGH PRIORITY: Items 0-150 (buffer ±50)
         │   └─→ Added to LIFO queue (processed first)
         │
         └─→ LOW PRIORITY: Items 151+
             └─→ Existing requests remain, new requests ignored
```

**Benefit:** Only decode thumbnails for visible+buffered items, not all 10,000+ images.

### 2. LIFO Task Queue

```
User scrolls down rapidly
    │
    ├─→ Requests for items 0-50 (added to queue)
    ├─→ Requests for items 100-150 (added to queue)
    ├─→ Requests for items 500-550 (added to queue, MOST RECENT)
    │
    ▼
TaskScheduler processes LIFO
    │
    └─→ Items 500-550 decoded FIRST (what user sees NOW)
        Items 100-150 decoded second
        Items 0-50 decoded last (if still needed)
```

**Benefit:** Always decode what's currently visible, not what was requested 5 seconds ago.

### 3. Generation ID Cancellation

```
Scan A starts (Gen 1)
    │
    ├─→ Still scanning network share...
    │
User switches to different folder
    │
    └─→ Scan B starts (Gen 2, m_scanGeneration++)
            │
            ▼
    Scan A eventually finishes
    • Checks generation ID
    • 1 != 2, discards results
    • NO model update

Scan B finishes
    • Checks generation ID
    • 2 == 2, applies results
    • Model updated
```

**Benefit:** No race conditions, no stale data, instant cancellation.

---

## 🔧 Key Configuration Points

### Scan Batch Sizes

| Location | Batch Size | Reason |
|----------|-----------|--------|
| **Local MFT (Fast)** | 50, 200, 1000 | Frequent early feedback, less frequent after initial load |
| **Local Fallback** | 10, 50, 200, 1000, then 2000 | Very frequent for slow scans |
| **Network** | NONE (single update) | Avoid UI thrashing on slow connections |
| **Albums** | 200 (Fast), 50 (Fallback) | Folders update less frequently than files |

### Buffer Sizes

| Component | Buffer | Purpose |
|-----------|--------|---------|
| **VisibleRangeManager** | ±50 items | Pre-load thumbnails before scrolling into view |
| **Burst Detection** | 2000ms | Group photos taken within 2 seconds |
| **Update Timer** | 16ms (~60fps) | Batch multiple thumbnail completions |

---

## 🐛 Common Issues & Solutions

### Issue: UI freezes during scan
**Cause:** Incremental updates on network path  
**Solution:** Network detection disables incremental updates  
**Check:** Look for `[NetworkScan]` logs

### Issue: Thumbnails don't load
**Cause:** Viewport culling too aggressive or AsyncImageProvider stalled  
**Solution:** Check `VisibleRangeManager` paths, verify TaskScheduler threads  
**Check:** `stagedRequestCount()` in TelemetryMonitor

### Issue: Old scan data appears after switching folders
**Cause:** Generation ID not checked  
**Solution:** Every model update MUST check `myGen != m_scanGeneration.load()`  
**Check:** Verify all `QMetaObject::invokeMethod` lambdas

### Issue: Race condition with rapid folder switching
**Cause:** Multiple scans running simultaneously  
**Solution:** Generation ID atomically invalidates previous scans  
**Check:** `m_scanGeneration` increments on each `scanDirectory`/`scanAlbums` call

---

## 📝 Decision Guidelines

### When to add incremental updates?
- ✅ Local drive with fast enumeration
- ✅ User needs immediate feedback (large libraries)
- ❌ Network paths (UI thrashing)
- ❌ Operations < 100ms (overhead > benefit)

### When to use FastVolumeScanner?
- ✅ Local fixed drives (C:\, D:\)
- ✅ User has 1000+ files
- ❌ Network shares
- ❌ Removable media (USB can be slow MFT)

### When to use TaskScheduler vs QtConcurrent?
- **TaskScheduler:** Image processing, I/O-bound file operations (controlled thread pool)
- **QtConcurrent:** One-off background tasks like folder scanning (Qt manages threads)

---

## 🎯 Future Improvements

1. **Adaptive Batch Sizes:** Measure scan speed, adjust update frequency dynamically
2. **Persistent Cache:** Store folder contents to disk for instant reload
3. **Directory Watching:** Detect file system changes, incremental re-scan
4. **Network Performance Hints:** Use QNetworkInformation to detect slow connections
### Maintenance & Debug Tools (Temporary)

- **QGalleryXBenchNet.exe**: A lightweight Win32 watchdog wrapper for `QGalleryXBench.exe`. 
    - **Purpose**: Diagnostic capture for network-share execution where silent crashes or .ps1 blockages occur.
    - **Cleanup Policy**: This is a non-production debug tool and should be removed once network stability is verified.
    - **Logic**: Uses `CreateProcess` and `WaitForSingleObject` to persist the console on non-zero exits.
5. **Background Pre-scanning:** Scan subdirectories in advance during idle time

---

**Last Updated:** 2026-02-08  
**Author:** System Architecture Documentation  
**Status:** ✅ Implemented and Verified (Build Successful)
