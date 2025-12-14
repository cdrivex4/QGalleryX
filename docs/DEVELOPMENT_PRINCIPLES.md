# Development Principles & Best Practices

## Overview

This document outlines the core development principles, coding standards, and architectural patterns used in the Antigravity project. These guidelines ensure code quality, maintainability, memory safety, and optimal performance. **Use this as a reference for future projects.**

---

## 🎯 Core Development Principles

### 1. RAII (Resource Acquisition Is Initialization)

**Definition**: Resources are acquired during object initialization and released during destruction, ensuring no leaks even in exceptional circumstances.

**Implementation in This Project**:
- **FFmpeg Resource Management**: The `VideoThumbnailer` class uses RAII wrappers (`FFmpegCleanup` nested class) to manage complex C-style FFmpeg resources
- **Direct3D11 Context Management**: Hardware acceleration contexts are properly wrapped and released
- **Qt Smart Pointers**: Use `QScopedPointer`, `QSharedPointer`, and `std::unique_ptr` for automatic memory management
- **File Handles**: Qt's RAII classes (`QFile`, `QDir`) ensure proper cleanup

**Example Pattern**:
```cpp
class VideoThumbnailer {
    struct FFmpegCleanup {
        AVFormatContext* formatCtx = nullptr;
        AVCodecContext* codecCtx = nullptr;
        
        ~FFmpegCleanup() {
            if (codecCtx) avcodec_free_context(&codecCtx);
            if (formatCtx) avformat_close_input(&formatCtx);
        }
    };
};
```

**Benefits**:
- No memory leaks, even during early returns or exceptions
- Automatic cleanup reduces cognitive load
- Thread-safe resource management

---

### 2. Modular Design & Separation of Concerns

**Definition**: Components are loosely coupled, each with a single, well-defined responsibility.

**Implementation in This Project**:

#### Component Isolation:
- **TaskScheduler**: Generic thread pool manager - knows nothing about images or videos
- **AsyncImageProvider**: Image loading service - doesn't know about UI components
- **ImageModel**: File system abstraction - isolated from QML
- **VideoThumbnailer**: Video processing logic - standalone utility
- **SystemMonitor**: Hardware stats provider - no business logic coupling

#### Benefits:
- **Testability**: Components can be unit-tested in isolation
- **Refactoring Safety**: Changes to one component don't cascade
- **Code Reusability**: Classes like `TaskScheduler` can be used in other Qt projects
- **Maintainability**: Clear boundaries make debugging easier

**Anti-Pattern to Avoid**:
```cpp
// BAD: Mixing concerns
class ImageLoader {
    void loadImage() {
        // File I/O
        // Image decoding
        // UI updates (QML signal emission)
        // Database updates
        // Network requests
    }
};
```

**Correct Pattern**:
```cpp
// GOOD: Single responsibility
class ImageDecoder { /* Only handles decoding */ };
class CacheManager { /* Only handles caching */ };
class UIBridge : public QObject { /* Only handles QML signals */ };
```

---

### 3. Code Reusability & Library Wrapping

**Definition**: Wrap third-party C libraries in modern C++ classes rather than scattering raw API calls throughout the codebase.

**Implementation in This Project**:
- **FFmpeg Wrapper**: `VideoThumbnailer` encapsulates all FFmpeg complexity
  - Hardware acceleration setup (D3D11VA)
  - Black frame detection
  - Thumbnail extraction
  - Error handling and fallback logic
  
- **LibRaw Wrapper**: RAW image decoding abstracted in `AsyncImageProvider`
  - Embedded preview extraction
  - Full decode pipeline
  - Memory-efficient processing

- **DirectX Wrapper**: `SystemMonitor` wraps DXGI and PDH APIs
  - GPU usage monitoring
  - VRAM tracking
  - CPU and memory stats

**Benefits**:
- **Centralized Error Handling**: One place to fix bugs
- **Testing**: Mock the wrapper instead of the entire library
- **API Contract**: Internal API is cleaner than raw C functions
- **Future Flexibility**: Easy to swap implementations

---

### 4. Memory Safety

**Definition**: Prevent memory leaks, dangling pointers, buffer overflows, and use-after-free errors through modern C++ practices.

**Practices Used**:

#### Smart Pointers:
```cpp
// GOOD: Automatic cleanup
QScopedPointer<ImageModel> model(new ImageModel());

// GOOD: Shared ownership
QSharedPointer<QImage> cachedImage = cache.get(id);

// BAD: Manual memory management
ImageModel* model = new ImageModel();
delete model; // Easy to forget or miss in error paths
```

#### Container Safety:
- Use Qt containers (`QVector`, `QList`, `QHash`) with automatic memory management
- Prefer `const` references to avoid unnecessary copies
- Use `std::move` for large objects to avoid deep copies

#### Thread Safety:
- **Mutex Protection**: Shared resources like FFmpeg hardware contexts protected by `QMutex`
- **Atomic Operations**: Use `QAtomicInt` for simple counters
- **Semaphores**: Limit concurrent GPU operations to prevent driver crashes

**Example from TaskScheduler**:
```cpp
class TaskScheduler {
    QMutex m_queueMutex;
    QSemaphore m_hwSemaphore{2}; // Max 2 concurrent GPU tasks
    
    void scheduleTask(Task task) {
        QMutexLocker lock(&m_queueMutex); // RAII lock
        m_queue.enqueue(task);
    }
};
```

---

### 5. Thread Safety & Concurrency

**Definition**: Ensure correct behavior when code executes concurrently across multiple threads.

**Threading Model in This Project**:

#### Main Thread (GUI Thread):
- Runs Qt event loop
- Handles QML property bindings
- Updates UI models (`ImageModel`, `AlbumModel`)
- **Must never block** - all I/O offloaded to workers

#### Render Thread (Qt Scene Graph):
- Managed internally by Qt Quick
- Renders the QML scene
- Synchronized with main thread before each frame

#### Worker Threads (QThreadPool):
- **CPU Tasks**: Image decoding, RAW processing, video thumbnails
- **I/O Tasks**: Directory scanning, file metadata reading
- Managed by `TaskScheduler` with priority queues

#### Thread Safety Patterns:
1. **Mutex for Shared State**:
   ```cpp
   QMutexLocker lock(&m_hwContextMutex);
   // Safe to access shared hardware context
   ```

2. **Signal/Slot for Thread Communication**:
   ```cpp
   // Worker thread emits signal
   emit imageLoaded(image);
   
   // Main thread receives via queued connection (Qt handles marshalling)
   connect(worker, &Worker::imageLoaded, this, &UI::displayImage, Qt::QueuedConnection);
   ```

3. **Lock-Free Queues** (where applicable):
   - Qt's `QQueue` with mutex protection
   - Atomic reference counting for cache

**Concurrency Limits**:
- **Hardware Video Decoding**: Limited to 2 concurrent operations (semaphore) to prevent GPU driver hangs
- **Thread Pool Size**: Dynamically scaled based on CPU cores
- **I/O vs CPU Separation**: Prevents disk-bound tasks from starving CPU-bound tasks

---

### 6. Clean Room Code & Documentation

**Definition**: Write code that is self-documenting, properly commented, and easy to understand without prior context.

**Practices**:

#### Self-Documenting Code:
```cpp
// GOOD: Clear naming
bool isBlackFrame(const AVFrame* frame) {
    const int threshold = 25; // Luma values below this are "dark"
    // ...
}

// BAD: Unclear naming
bool chk(AVFrame* f) {
    int t = 25;
    // ...
}
```

#### Comment What, Not How:
```cpp
// GOOD: Explains intent
// Skip dark intro sequences (common in movies) by seeking to 10%
if (isBlackFrame(frame)) {
    seekToPercent(0.10);
}

// BAD: Redundant comment
// Call seekToPercent with 0.10
seekToPercent(0.10);
```

#### Header Documentation:
```cpp
/**
 * @brief Generates video thumbnails using hardware-accelerated FFmpeg.
 * 
 * Features:
 * - D3D11VA hardware acceleration with software fallback
 * - Black frame detection to skip dark intros
 * - Automatic retry at 10%, 25% if initial frame is dark
 * 
 * Thread Safety: Safe to call from multiple threads (mutex-protected HW context)
 */
class VideoThumbnailer { /* ... */ };
```

#### Inline Comments for Complex Logic:
- Use comments to explain **why** decisions were made
- Reference external resources (FFmpeg docs, StackOverflow, bug trackers)
- Mark known limitations or workarounds

---

## 🏗️ Architectural Patterns

### Layered Architecture

```
┌──────────────────────────────────────┐
│         QML Frontend Layer           │  User Interface
├──────────────────────────────────────┤
│       Qt Integration Layer           │  Context Properties, Type Registration
├──────────────────────────────────────┤
│        C++ Backend Layer             │  Business Logic, Data Models
├──────────────────────────────────────┤
│      Third-Party Libraries           │  FFmpeg, LibRaw, DirectX
└──────────────────────────────────────┘
```

**Communication Rules**:
- QML → C++ via context properties and registered types
- C++ → QML via signals (`Q_SIGNALS`)
- No direct QML access from C++ (use signals/slots)

### Asynchronous Processing Pipeline

```
User Action → Main Thread → TaskScheduler → Worker Thread → Signal → Main Thread → UI Update
```

**Key Points**:
- All blocking operations (file I/O, decoding) happen on worker threads
- Results marshalled back to main thread via Qt's signal/slot mechanism
- UI remains responsive during heavy operations

---

## 🔒 Security & Stability

### Input Validation
- **Path Sanitization**: Use `QDir::cleanPath()` and `QFileInfo` for all user-provided paths
- **UNC Path Support**: Use `QUrl::toLocalFile()` for correct `\\Server\Share` handling
- **File Extension Validation**: Whitelist known image/video formats

### Error Handling
- **Graceful Degradation**: Software fallback if hardware acceleration fails
- **User-Visible Errors**: Log technical details, show user-friendly messages
- **No Silent Failures**: Always log errors, even if recoverable

### Resource Limits
- **Cache Size Limits**: Configurable max memory (64MB - 2048MB)
- **Concurrency Limits**: Prevent GPU overload with semaphores
- **Timeout Mechanisms**: Detect and abort stuck operations

---

## 📋 Code Review Checklist

Before committing code, verify:

- [ ] **RAII Used**: All resources wrapped in RAII classes
- [ ] **No Raw Pointers**: Use smart pointers or Qt parent-child ownership
- [ ] **Thread Safety**: Shared mutable state protected by mutex
- [ ] **Null Checks**: Pointers validated before dereferencing
- [ ] **Error Handling**: All failure paths handled or logged
- [ ] **Comments**: Complex logic explained
- [ ] **Separation of Concerns**: Single responsibility per class
- [ ] **Qt Conventions**: Follow Qt naming (camelCase methods, m_memberVars)
- [ ] **Memory Leaks**: Run with memory profiler (Valgrind, Dr. Memory)
- [ ] **Performance**: No blocking calls on main thread

---

## 🚀 Performance Guidelines

### General Rules
1. **Measure Before Optimizing**: Use `StatsOverlay` and profilers
2. **Async by Default**: Offload I/O and CPU-intensive work
3. **Cache Intelligently**: LRU cache for recently used images
4. **Prioritize User Actions**: Full-size viewer images load before thumbnails

### Specific Optimizations in This Project
- **D3D11 Hardware Decoding**: 10x faster than software for video thumbnails
- **RAW Embedded Previews**: Load JPEG preview instead of full decode (40x faster)
- **Priority Queues**: Viewer requests jump ahead of gallery thumbnails
- **Lazy Loading**: Images loaded on-demand, not all at once

---

## 📚 Required Reading for New Developers

1. **Qt Documentation**:
   - [Qt Object Model](https://doc.qt.io/qt-6/object.html)
   - [Signals & Slots](https://doc.qt.io/qt-6/signalsandslots.html)
   - [Thread Basics](https://doc.qt.io/qt-6/thread-basics.html)

2. **C++ Best Practices**:
   - [CppCoreGuidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
   - RAII and smart pointers (std::unique_ptr, QScopedPointer)

3. **Project-Specific**:
   - `docs/ARCHITECTURE.md` - Component relationships
   - `docs/THREAD_HIERARCHY.md` - Threading model
   - `docs/FEATURES.md` - Feature specifications

---

## 🎓 Lessons Learned from This Project

### What Worked Well
✅ Separating test app (`main_test.cpp`) from production app allowed safe experimentation  
✅ RAII wrappers eliminated FFmpeg memory leaks  
✅ TaskScheduler abstraction made it easy to add priority queues later  
✅ Comprehensive logging (`LogManager`) made debugging network issues trivial  

### What to Improve
⚠️ Initial design had blocking I/O on main thread (QDir::exists on network share) - fixed by offloading to worker  
⚠️ Early version didn't limit GPU concurrency - led to driver hangs - fixed with semaphore  
⚠️ Hardcoded paths in defaults - should force folder picker on first run  

---

## 🔧 Tools & Workflow

### Build System
- **CMake 3.16+**: Cross-platform build configuration
- **PowerShell Scripts**: `build.ps1` for automated builds, `deploy.ps1` for packaging
- **Qt 6.4+**: Modern Qt framework with QML

### Development Tools
- **Visual Studio 2022**: Primary IDE (MSVC compiler)
- **Qt Creator**: Alternative IDE with better QML support
- **Git**: Version control
- **CTest**: Unit test runner

### Debugging
- **Qt Creator Debugger**: Step through C++ and QML
- **qDebug() with LogManager**: Thread-safe logging to file
- **StatsOverlay**: Runtime performance monitoring (FPS, CPU, memory, GPU)

### Testing
- **Unit Tests**: `tst_imagemodel.cpp`, `tst_scheduler.cpp` (Qt Test framework)
- **Integration Tests**: Manual testing via test app (`appSamsungGalleryTest.exe`)
- **Performance Tests**: `StatsOverlay` for runtime metrics

---

## 🌟 Summary

This project demonstrates modern C++ best practices:
- **Safety**: RAII and smart pointers prevent leaks
- **Performance**: Async processing and hardware acceleration
- **Maintainability**: Modular design and clear separation
- **Reliability**: Thread-safe concurrency and error handling

**Apply these principles to future projects** for clean, maintainable, high-performance code.

---

**Last Updated**: 2025-12-07  
**Author**: Antigravity Development Team  
**Project**: Samsung Gallery Clone (Antigravity Media Viewer)
