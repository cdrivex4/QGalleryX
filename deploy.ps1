$ErrorActionPreference = "Stop"

$BUILD_DIR = "build"
$DIST_DIR = "Dist/QGalleryX"
$QT_BIN_DIR = "D:\Qt\6.9.3\mingw_64\bin"
$WINDEPLOYQT = "D:\Qt\6.9.3\mingw_64\bin\windeployqt.exe"
$FFMPEG_BIN = "3rdparty/ffmpeg/bin"

Write-Host "--- Starting Deployment ---" -ForegroundColor Cyan

# 1. Clean Dist Dir
if (Test-Path $DIST_DIR) {
    Remove-Item -Recurse -Force $DIST_DIR -ErrorAction SilentlyContinue
}
New-Item -ItemType Directory -Force -Path $DIST_DIR | Out-Null

# 2. Copy Executable
Write-Host "Copying executable..."
$targetExe = "$DIST_DIR/QGalleryX.exe"
if (Test-Path $targetExe) {
    try {
        Copy-Item "$BUILD_DIR/QGalleryX.exe" -Destination $targetExe -Force
    } catch {
        $oldExe = "$DIST_DIR/QGalleryX.exe.old"
        Remove-Item $oldExe -Force -ErrorAction SilentlyContinue
        Move-Item $targetExe $oldExe -Force -ErrorAction SilentlyContinue
        Copy-Item "$BUILD_DIR/QGalleryX.exe" -Destination $targetExe -Force
    }
} else {
    Copy-Item "$BUILD_DIR/QGalleryX.exe" -Destination $DIST_DIR -Force
}

# 3. Run windeployqt
Write-Host "Running windeployqt..."
& $WINDEPLOYQT --qmldir "resources/qml_legacy" --dir $DIST_DIR "$DIST_DIR/QGalleryX.exe" --release --no-translations --compiler-runtime

# 4. Copy FFmpeg DLLs
Write-Host "Copying FFmpeg DLLs..."
Get-ChildItem "$FFMPEG_BIN/*.dll" | Copy-Item -Destination $DIST_DIR

# 5. Copy LibRaw DLLs? (Static, so no)

Write-Host "--- Deployment Complete ---" -ForegroundColor Green
Write-Host "Output: $DIST_DIR"
