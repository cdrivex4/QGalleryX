param (
    [switch]$Clean = $false
)

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

# Step 0: Verify Dependencies
Write-Host "[0/4] Verifying Dependencies..." -ForegroundColor Yellow
$FFmpegBin = Join-Path $PSScriptRoot "3rdparty/ffmpeg/bin"
$RequiredDlls = @("avcodec-62.dll", "avformat-62.dll", "avutil-60.dll", "swscale-9.dll")

foreach ($dll in $RequiredDlls) {
    if (!(Test-Path (Join-Path $FFmpegBin $dll))) {
        Write-Host "ERROR: Missing dependency: $dll" -ForegroundColor Red
        Write-Host "  Expected at: $(Join-Path $FFmpegBin $dll)" -ForegroundColor Red
        Write-Host "  Please ensure all 3rdparty libraries are present." -ForegroundColor Red
        Write-Host "  If you cloned the repo, you might need to manually download large binaries if they were excluded." -ForegroundColor Red
        exit 1
    }
}
Write-Host "  FFmpeg binaries found." -ForegroundColor Gray

# Step 1: Kill running instances (Robust)
Write-Host "[1/4] Checking for running instances..." -ForegroundColor Yellow
$pNames = @("appSamsungGallery", "appSamsungGalleryTest")
foreach ($pName in $pNames) {
    if (Get-Process -Name $pName -ErrorAction SilentlyContinue) {
        Write-Host "  Killing $pName..." -ForegroundColor Gray
        Stop-Process -Name $pName -Force -ErrorAction SilentlyContinue
        
        # Wait for it to actually die
        $retries = 10
        while ((Get-Process -Name $pName -ErrorAction SilentlyContinue) -and ($retries -gt 0)) {
            Start-Sleep -Milliseconds 500
            $retries--
        }
        
        if (Get-Process -Name $pName -ErrorAction SilentlyContinue) {
            Write-Host "  FAILED to kill $pName. File might be locked." -ForegroundColor Red
            taskkill /F /IM "$pName.exe" | Out-Null # Last resort
        }
    }
}

# Step 2: Clean if requested or ensure freshness
# Remove old binary to ensure we never run stale code if build fails
if (Test-Path $ExePath) {
    Remove-Item -Force $ExePath -ErrorAction SilentlyContinue
}

if ($Clean) {
    Write-Host "[2/4] Full Clean requested. Removing build directory..." -ForegroundColor Magenta
    if (Test-Path $BuildDir) {
        Remove-Item -Recurse -Force $BuildDir | Out-Null
    }
}
else {
    # We remove the autogen folder to force Qt to re-parse QML and Signals
    if (Test-Path "$BuildDir/appSamsungGallery_autogen") {
        Remove-Item -Recurse -Force "$BuildDir/appSamsungGallery_autogen" | Out-Null
    }
    Write-Host "[2/4] Cleaned autogen folders & removed old binary." -ForegroundColor Yellow
}

# Step 3: Configure & Build
Write-Host "[3/4] Building (Release)..." -ForegroundColor Yellow
try {
    # Create build dir if it doesn't exist
    if (!(Test-Path $BuildDir)) {
        New-Item -ItemType Directory -Path $BuildDir | Out-Null
    }

    # Configure
    Write-Host "  -> Configuring..." -ForegroundColor Gray
    cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release -S . -B $BuildDir
    if ($LASTEXITCODE -ne 0) { throw "CMake Configuration Failed" }
    
    # Build
    Write-Host "  -> Compiling..." -ForegroundColor Gray
    cmake --build $BuildDir
    if ($LASTEXITCODE -ne 0) { throw "Build Failed" }

    # Deploy Qt dependencies (only checking if plugins missing logic could be added, but forced for now)
    Write-Host "  -> Deploying Qt Dependencies..." -ForegroundColor Gray
    
    # We construct the command manually to ensure arguments are passed correctly
    $WindeployQt = "D:\Qt\6.9.3\mingw_64\bin\windeployqt.exe"
    
    # Standard deploy for QML
    & $WindeployQt --qmldir $PSScriptRoot/resources/qml --dir $BuildDir $BuildDir/appSamsungGallery.exe --compiler-runtime --no-opengl-sw
    
    # We explicitly ensure imageformats are copied
    # windeployqt usually handles this if it detects QtGui, but being explicit is safer

}
catch {
    Write-Host "BUILD FAILED!" -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Red
    exit 1
}

# Step 4: Report
$ExePath = Join-Path $BuildDir "appSamsungGallery.exe"
$TestExePath = Join-Path $BuildDir "appSamsungGalleryTest.exe"

if (Test-Path $ExePath) {
    Write-Host "==========================================" -ForegroundColor Green
    Write-Host " BUILD SUCCESSFUL" -ForegroundColor Green
    Write-Host " Stable:      $(Resolve-Path $ExePath)" -ForegroundColor Magenta
    
    # Check for ScrollBench
    # It might be in build/test_scrollbench/appScrollBench.exe or build/appScrollBench.exe depending on CMake
    $ScrollBenchExePath = Join-Path $BuildDir "appScrollBench.exe"
    if (!(Test-Path $ScrollBenchExePath)) {
        $ScrollBenchExePath = Join-Path $BuildDir "test_scrollbench/appScrollBench.exe"
    }

    if (Test-Path $ScrollBenchExePath) {
        Write-Host " ScrollBench: $(Resolve-Path $ScrollBenchExePath)" -ForegroundColor Cyan
    }

    if (Test-Path $TestExePath) {
        Write-Host " Test:        $(Resolve-Path $TestExePath)" -ForegroundColor Cyan
    }
    Write-Host "==========================================" -ForegroundColor Green
}
else {
    Write-Host "Build finished but binary not found!" -ForegroundColor Red
    exit 1
}
