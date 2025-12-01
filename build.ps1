$ErrorActionPreference = "Stop"
$env:PATH = "D:\Qt\6.9.3\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;D:\Qt\Tools\CMake_64\bin;D:\Qt\Tools\Ninja;$env:PATH"

# Ensure we are in the project root
Set-Location $PSScriptRoot

# Configuration
$BuildDir = "build"
$ExeName = "appSamsungGallery.exe"
$ExePath = Join-Path $BuildDir $ExeName

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "   SamsungGallery Build System" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan

# Step 1: Kill running instances
Write-Host "[1/4] Checking for running instances..." -ForegroundColor Yellow
try {
    taskkill /F /IM "appSamsungGallery.exe" 2>$null
    taskkill /F /IM "appSamsungGalleryTest.exe" 2>$null
}
catch {}

# Step 2: Ensure QML/MOC freshness (Critical for UI changes)
# We remove the autogen folder to force Qt to re-parse QML and Signals
if (Test-Path "$BuildDir/appSamsungGallery_autogen") {
    Remove-Item -Recurse -Force "$BuildDir/appSamsungGallery_autogen" | Out-Null
}
if (Test-Path "$BuildDir/appSamsungGalleryTest_autogen") {
    Remove-Item -Recurse -Force "$BuildDir/appSamsungGalleryTest_autogen" | Out-Null
}
Write-Host "[2/4] Cleaned autogen folders." -ForegroundColor Yellow

# Step 3: Configure & Build
Write-Host "[3/4] Building (Release)..." -ForegroundColor Yellow
try {
    # Configure (creates build dir if missing)
    cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release -S . -B $BuildDir
    
    # Build
    cmake --build $BuildDir -j 4
}
catch {
    Write-Host "BUILD FAILED!" -ForegroundColor Red
    exit 1
}

# Step 4: Report
$ExePath = Join-Path $BuildDir "appSamsungGallery.exe"
$TestExePath = Join-Path $BuildDir "appSamsungGalleryTest.exe"

if (Test-Path $ExePath) {
    Write-Host "==========================================" -ForegroundColor Green
    Write-Host " BUILD SUCCESSFUL" -ForegroundColor Green
    Write-Host " Stable: $(Resolve-Path $ExePath)" -ForegroundColor Magenta
    if (Test-Path $TestExePath) {
        Write-Host " Test:   $(Resolve-Path $TestExePath)" -ForegroundColor Cyan
    }
    Write-Host "==========================================" -ForegroundColor Green
}
else {
    Write-Host "Build finished but binary not found!" -ForegroundColor Red
    exit 1
}
