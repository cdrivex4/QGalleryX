# Folder Scanning Diagnostic Report

## Current Scanning Configuration

### ImageModel.cpp (Main App)
**Line 203-216:**
```cpp
QDirIterator it(cleanPath,
    QStringList() << "*.jpg" << "*.jpeg" << "*.png" << "*.mp4"
                  << /* ... 40+ extensions ... */,
    QDir::Files,
    QDirIterator::Subdirectories);
```

### ScrollBenchImageModel.cpp
**Line 256-257:**
```cpp
QDirIterator it(cleanPath, filters, 
    QDir::Files | QDir::Readable,
    QDirIterator::Subdirectories);
```

---

## ✅ What IS Being Scanned:

- **Recursive**: YES (`QDirIterator::Subdirectories`)
- **Supported formats**: 40+ extensions (images, videos, RAW)
- **File types**: Regular files only (`QDir::Files`)

---

## ❌ What Might Be MISSED:

###  1. **Hidden Files/Folders**
**Status**: SKIPPED (not in flags)

Hidden folders (e.g., `.thumbnails`, `$RECYCLE.BIN`) are **ignored** by default.

**Fix**: Add `QDir::Hidden` to flags:
```cpp
QDir::Files | QDir::Hidden  // Include hidden files
```

### 2. **System Folders**
**Status**: SKIPPED (not in flags)

System folders (e.g., `System Volume Information`) are **ignored**.

**Fix**: Add `QDir::System` (risky):
```cpp
QDir::Files | QDir::System  // Include system files (careful!)
```

### 3. **Symlinks**
**Status**: **NOT FOLLOWED** (default behavior)

Symbolic links to folders are **not** traversed.

**Fix**: Enable symlink following (risky - can cause loops):
```cpp
QDirIterator::FollowSymlinks  // Follow symlinks (careful of loops!)
```

### 4. **Permission Issues**
**Status**: ScrollBench adds `QDir::Readable` filter

Files/folders without read permission are **skipped** in ScrollBench but **attempted** in main app.

**Impact**: 
- Main app might crash on unreadable files
- ScrollBench silently skips them

### 5. **Case-Sensitive Extensions**
**Status**: **WILL MISS** uppercase extensions

Filters use lowercase: `*.jpg`, `*.png`, etc.

**Missing**: `*.JPG`, `*.PNG`, `*.JPEG` (common on cameras!)

**Fix**: Use case-insensitive filter (QDir has no built-in support):
```cpp
// Must manually add uppercase variants:
filters << "*.jpg" << "*.JPG" << "*.jpeg" << "*.JPEG" << "*.png" << "*.PNG"
```

Or use `QRegularExpression` with case-insensitive matching.

### 6. **Network Path Timeouts**
**Status**: No timeout handling

UNC paths (`\\server\share`) might:
- Hang on slow networks
- Partially fail on disconnected shares
- Skip inaccessible subfolders silently

---

## Most Likely Culprits:

### 🔴 **UPPERCASE Extensions** (Very Common!)
Cameras often save files as:
- `IMG_0001.JPG` (not `.jpg`)
- `VID_2023.MP4` (not `.mp4`)
- `DSC_0001.ARW` (not `.arw`)

**These are ALL being skipped!**

### 🟡 **Hidden Folders**
Some software (e.g., Lightroom, Google Photos backup) stores images in:
- `.picasa`
- `.thumbnails`
- `__MACOSX`

---

## Recommended Fixes:

### Priority 1: Fix Case-Sensitivity (CRITICAL)

**Before:**
```cpp
filters << "*.jpg" << "*.jpeg" << "*.png" << "*.mp4";
```

**After:**
```cpp
QStringList filters;
// Add both lowercase and uppercase variants
QStringList baseExtensions = {"jpg", "jpeg", "png", "mp4", "mkv", "avi", 
    "mov", "arw", "cr2", "dng", "nef", "webp", "heic", "tiff", "bmp"};
    
for (const QString &ext : baseExtensions) {
    filters << QString("*.%1").arg(ext);           // lowercase
    filters << QString("*.%1").arg(ext.toUpper()); // UPPERCASE
}
```

### Priority 2: Add Hidden Files Support (Optional)

```cpp
QDir::Files | QDir::Hidden  // If you want hidden files
```

### Priority 3: Add Diagnostic Logging

```cpp
qDebug() << "Scanning:" << cleanPath;
qDebug() << "Subdirectories found:" << QDir(cleanPath).entryList(QDir::Dirs | QDir::NoDotAndDotDot);

// After scanning:
qDebug() << "Total files found:" << totalFound;
qDebug() << "Sample paths:" << (found files list);
```

---

## Test Cases to Verify:

1. **Create test files:**
   ```
   test_folder/
     ├── image.jpg      (lowercase - should work)
     ├── IMAGE.JPG      (uppercase - currently MISSING!)
     ├── video.mp4      (lowercase - should work)
     ├── VIDEO.MP4      (uppercase - currently MISSING!)
     └── subfolder/
         └── photo.PNG  (uppercase - currently MISSING!)
   ```

2. **Run scan and check logs** for count

3. **Expected**: All 5 files found  
   **Actual (current)**: Only 2 files found (lowercase only)

---

## Summary:

**Root cause**: Likely **case-sensitive file extension matching**

Most cameras save files as `.JPG` (uppercase), not `.jpg` (lowercase).

**Quick fix**: Add uppercase variants to filters  
**Better fix**: Use case-insensitive matching throughout
