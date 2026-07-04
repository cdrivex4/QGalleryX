# Share & Resize + Media Type Support

## Current Focus: Share Dialog (Phase 3.2)

Wire up Share button to open dialog with resize presets.

### ShareDialog.qml

```qml
Dialog {
    id: shareDialog
    title: "Share " + imageModel.selectedCount + " images"
    modal: true
    
    ColumnLayout {
        spacing: 15
        
        Button {
            text: "Resize for Email (Small)"
            Layout.fillWidth: true
            onClicked: {
                // Apply preset: 1024x768, 80% quality
                console.log("Email small preset")
                shareDialog.close()
            }
        }
        
        Button {
            text: "Resize for Email (Manual)"
            Layout.fillWidth: true
            onClicked: {
                // Open advanced editor (Phase 3.3)
                console.log("Open resize editor")
                shareDialog.close()
            }
        }
        
        Button {
            text: "Share Original Size"
            Layout.fillWidth: true
            onClicked: {
                // Share without modifications
                console.log("Share original")
                shareDialog.close()
            }
        }
        
        Button {
            text: "Cancel"
            Layout.fillWidth: true
            onClicked: shareDialog.close()
        }
    }
}
```

---

## Future: Media Type Support

### Video & RAW Thumbnail Generation

**Requirements:**
- FFmpeg for video thumbnails
- LibRaw for RAW formats (.CR2, .NEF, .ARF, etc.)
- Thumbnail caching system
- Background processing

### Performance Overlay Toggles

Add to PerformanceOverlay.qml:

```qml
ColumnLayout {
    Text {
        text: "Media Type Filters"
        font.bold: true
    }
    
    Switch {
        text: "Show Images"
        checked: true
        onToggled: imageModel.setShowImages(checked)
    }
    
    Switch {
        text: "Show Videos"
        checked: true
        onToggled: imageModel.setShowVideos(checked)
    }
    
    Switch {
        text: "Show RAW Formats"
        checked: true
        onToggled: imageModel.setShowRaw(checked)
    }
}
```

### Model Updates

```cpp
// ScrollBenchImageModel.h
Q_PROPERTY(bool showImages READ showImages WRITE setShowImages NOTIFY filterChanged)
Q_PROPERTY(bool showVideos READ showVideos WRITE setShowVideos NOTIFY filterChanged)
Q_PROPERTY(bool showRaw READ showRaw WRITE setShowRaw NOTIFY filterChanged)

Q_INVOKABLE void setShowImages(bool show);
Q_INVOKABLE void setShowVideos(bool show);
Q_INVOKABLE void setShowRaw(bool show);

signals:
    void filterChanged();

private:
    bool m_showImages = true;
    bool m_showVideos = true;
    bool m_showRaw = true;
```

### ImageItem Type Detection

```cpp
struct ImageItem {
    QString path;
    QString fileName;
    MediaType type;  // Image, Video, Raw
    bool isLoaded = false;
    bool isSelected = false;
};

enum MediaType {
    Image,
    Video,
    Raw
};
```

---

## Implementation Order

### ✅ Phase 3.1: Selection Mode Buttons (DONE)
- Select All, Select None, Invert
- Share icon

### 🔄 Phase 3.2: Basic Share Dialog (CURRENT)
- Create ShareDialog.qml
- Wire up share button
- Add preset options
- **Note: Only processes images, not videos**

### Phase 3.3: Advanced Resize Editor
- Split-screen preview
- Quality/compression sliders
- Size calculator

### Phase 3.4: Image Processing
- C++ resize backend
- Quality/compression
- Preview generation

### Phase 4: Video/RAW Support
- FFmpeg integration for video thumbnails
- LibRaw for RAW formats
- Media type detection
- Performance overlay toggles
- Filter implementation

---

## Notes

> [!IMPORTANT]
> Share dialog will only process **images**. Videos will be excluded from share operations.

> [!NOTE]
> Media type toggles will filter the grid view. Videos and RAW files will still be selectable but won't be processed by resize/share operations.
