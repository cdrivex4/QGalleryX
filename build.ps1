param (
    [switch]$Clean = $false,
    [switch]$BuildSingleExe = $false
)

$ErrorActionPreference = "Stop"

# Ensure we are in the project root
Set-Location $PSScriptRoot

# Step 0.1: Configure Build Environment (Qt Kit Selection)
if ($BuildSingleExe) {
    $QtRoot = Join-Path $PSScriptRoot "3rdparty/qt_static"
    $QtBin = Join-Path $QtRoot "bin"
    if (-not (Test-Path $QtBin)) {
        Write-Error "Static Qt not found at $QtRoot. Please run .\scripts\setup_static_qt.ps1 first."
    }
    $env:CMAKE_PREFIX_PATH = $QtRoot
    Write-Host "=== MODE: STATIC SINGLE EXECUTABLE ===" -ForegroundColor Cyan
    Write-Host "  -> Qt Kit: $QtRoot" -ForegroundColor Cyan
}
else {
    # Default Dynamic Qt
    $QtRoot = "D:\Qt\6.9.3\mingw_64"
    $QtBin = Join-Path $QtRoot "bin"
    Write-Host "=== MODE: STANDARD DYNAMIC BUILD ===" -ForegroundColor Green
    Write-Host "  -> Qt Kit: $QtRoot" -ForegroundColor Gray
}

# Add Tools to PATH
$env:PATH = "$QtBin;D:\Qt\Tools\mingw1310_64\bin;D:\Qt\Tools\CMake_64\bin;D:\Qt\Tools\Ninja;$env:PATH"

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
$pNames = @("appSamsungGallery", "appSamsungGalleryTest", "appScrollBench")
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
    
    # Build CMake arguments
    $cmakeConfigArgs = @("-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release", "-S", ".", "-B", $BuildDir)
    
    # Add single exe flag if requested
    if ($BuildSingleExe) {
        $cmakeConfigArgs += "-DBUILD_SINGLE_EXE=ON"
        Write-Host "  -> Single EXE build enabled" -ForegroundColor Cyan
    }
    
    & cmake @cmakeConfigArgs
    if ($LASTEXITCODE -ne 0) { throw "CMake Configuration Failed" }
    
    # Build
    Write-Host "  -> Compiling..." -ForegroundColor Gray
    cmake --build $BuildDir
    if ($LASTEXITCODE -ne 0) { throw "Build Failed" }

    # Deploy Qt dependencies (only checking if plugins missing logic could be added, but forced for now)
    Write-Host "  -> Deploying Qt Dependencies..." -ForegroundColor Gray
    
    # We construct the command manually to ensure arguments are passed correctly
    $WindeployQt = "D:\Qt\6.9.3\mingw_64\bin\windeployqt.exe"
    
    # Standard deploy for Main App
    & $WindeployQt --qmldir $PSScriptRoot/resources/qml --dir $BuildDir $BuildDir/appSamsungGallery.exe --compiler-runtime --no-opengl-sw
    
    # Standard deploy for ScrollBench (Segregated)
    # Target: test_scrollbench/deploy (per docs/SCROLLBENCH_STRATEGY.md)
    $ScrollBenchDeployDir = Join-Path $PSScriptRoot "test_scrollbench/deploy"
    if (Test-Path "$ScrollBenchDeployDir/appScrollBench.exe") {
        Write-Host "  -> Deploying ScrollBench to test_scrollbench/deploy..." -ForegroundColor Gray
        & $WindeployQt --qmldir $PSScriptRoot/test_scrollbench/qml --dir $ScrollBenchDeployDir "$ScrollBenchDeployDir/appScrollBench.exe" --compiler-runtime --no-opengl-sw
    }

    # Deploy for Single EXE (Native Static Build)
    if ($BuildSingleExe) {
        # Static builds don't typically need windeployqt, but if they do, we point to the static tool
        # Ideally, CMake static linking handles it all.
        
        $SingleExePath = Join-Path $PSScriptRoot "single_exe/bin/ScrollBenchPortable.exe"
        if (Test-Path $SingleExePath) {
            # No WindeyploQt needed for pure static
        }
    }

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
    
    # Check for Single EXE builds
    $ScrollBenchExePath = Join-Path $PSScriptRoot "test_scrollbench/deploy/appScrollBench.exe"
    if (Test-Path $ScrollBenchExePath) {
        Write-Host " ScrollBench: $(Resolve-Path $ScrollBenchExePath)" -ForegroundColor Cyan
    }

    # Check for Single EXE Native build
    $SingleExePath = Join-Path $PSScriptRoot "single_exe/bin/ScrollBenchPortable.exe"
    if ($BuildSingleExe -and (Test-Path $SingleExePath)) {
        Write-Host "==========================================" -ForegroundColor Cyan
        Write-Host " SUCCESS: NATIVE SINGLE EXECUTABLE" -ForegroundColor Cyan
        Write-Host " File: $SingleExePath" -ForegroundColor Magenta
        $size = (Get-Item $SingleExePath).Length / 1MB
        Write-Host (" Size: {0:N2} MB" -f $size) -ForegroundColor Gray
        Write-Host " This is a standalone file." -ForegroundColor DarkGray
    }
}
else {
    Write-Host "Build finished but binary not found!" -ForegroundColor Red
    exit 1
}

# Step 3.5: Run Linkage Verification (Safety Check)
Write-Host "[3.5/4] Verifying Module Linkage..." -ForegroundColor Yellow
$LinkageTool = Join-Path $PSScriptRoot "test_scrollbench/deploy/tst_linkage.exe"
if (Test-Path $LinkageTool) {
    & $LinkageTool
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Module linkage verification FAILED! Please check backend integration."
        exit 1
    }
} else {
    Write-Warning "Linkage verification tool not found at $LinkageTool. Skipping."
}

