# Samsung Gallery Clone - Architecture Documentation

## System Architecture

### High-Level Overview

The application follows a Qt-based architecture with C++ backend logic and QML frontend interface. The system is divided into three main layers:

1. **C++ Backend Layer** - Core logic, data models, and image processing
2. **QML Frontend Layer** - User interface and presentation logic  
3. **Integration Layer** - Qt's signal-slot mechanism connecting backend to frontend

### Component Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    QML Frontend Layer                       │
├─────────────────────────────────────────────────────────────┤
│  Main.qml          │  GalleryView.qml   │  PhotoViewer.qml  │
│  (Application)     │  (Grid View)       │  (Image Viewer)   │
│                    │                    │                   │
│  AlbumsView.qml    │  BottomBar.qml     │  StatsOverlay.qml │
│  (Albums)          │  (Navigation)      │  (Performance)    │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                   Qt Integration Layer                      │
├─────────────────────────────────────────────────────────────┤
│  QML Context Properties  │  QML Type Registration  │        │
│  (appSettings)          │  (ImageModel, etc.)     │        │
│                         │                         │        │
│  Image Provider         │  Signal-Slot Connections │        │
│  (AsyncImageProvider)   │  (Backend-Frontend)     │        │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                   C++ Backend Layer                         │
├─────────────────────────────────────────────────────────────┤
│  ImageModel.cpp/h      │  AlbumModel.cpp/h      │        │
│  (Image Data)          │  (Album Data)          │        │
│                         │                         │        │
│  SettingsHelper.cpp/h  │  AsyncImageProvider.cpp/h│        │
│  (Configuration)       │  (Async Loading)       │        │
└─────────────────────────────────────────────────────────────┘
```

### Core Components

#### 1. Main Application (`src/main.cpp`)
- **Purpose**: Application entry point and setup
- **Key Functions**:
  - Graphics API configuration (Direct3D11, Vulkan, OpenGL, Software)
  - QML type registration
  - Context property setup
  - Image provider registration

#### 2. ImageModel (`src/ImageModel.cpp/h`)
- **Purpose**: Data model for image files
- **Key Features**:
  - Recursive directory scanning
  - Background thread processing
  - Date-based grouping
  - EXIF data extraction
  - Image cropping functionality

#### 3. AsyncImageProvider (`src/AsyncImageProvider.cpp/h`)
- **Purpose**: Asynchronous image loading and caching
- **Key Features**:
  - Thread pool for concurrent loading
  - Memory-efficient caching
  - Cancelable operations
  - Performance monitoring

#### 4. SettingsHelper (`src/SettingsHelper.cpp/h`)
- **Purpose**: Application settings and system information
- **Key Features**:
  - Graphics API selection
  - Performance tuning settings
  - System resource monitoring
  - Cache management

#### 5. QML Frontend Components
- **Main.qml**: Main application window with tab navigation
- **GalleryView.qml**: Grid-based image browser with zoom/pinch support
- **PhotoViewer.qml**: Full-screen image viewer with navigation and editing
- **AlbumsView.qml**: Album organization interface (placeholder)
- **BottomBar.qml**: Tab navigation bar
- **StatsOverlay.qml**: Performance monitoring overlay

### Data Flow Architecture

```
User Interaction → QML Event → Signal → C++ Slot → Processing → Result → Signal → QML Update
```

### Key Design Patterns

1. **Model-View-Controller**: QML views, C++ models, Qt's built-in controller
2. **Async Pattern**: Background processing for file operations
3. **Provider Pattern**: AsyncImageProvider for image loading
4. **Observer Pattern**: Qt's signal-slot mechanism for UI updates
5. **Factory Pattern**: QML type registration system

### Performance Optimizations

1. **Asynchronous Loading**: All file I/O operations run in background threads
2. **Caching**: Multi-level caching (memory, disk, GPU)
3. **Lazy Loading**: Images loaded on-demand as user scrolls
4. **GPU Acceleration**: Hardware-accelerated rendering
5. **Thread Pool**: Concurrent image processing

### Memory Management

- Qt's parent-child ownership model
- Shared pointers for cancelable operations
- QCache for efficient memory usage
- RAII patterns in C++ components