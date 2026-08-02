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
$env:PATH = "$QtBin;D:\\Qt\\Tools\\mingw1310_64\\bin;D:\\Qt\\Tools\\CMake_64\\bin;D:\\Qt\\Tools\\Ninja;$env:PATH"

# Configuration
$BuildDir = "build"
$ExeName = "QGalleryX.exe"
$ExePath = Join-Path $BuildDir $ExeName

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "   SamsungGallery Build System" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan

# Increment Build Number
& "$PSScriptRoot\increment_build.ps1"

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
$pNames = @("QGalleryX", "QGalleryXTest", "QGalleryXBench")
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
            taskkill /F /IM "$pName.exe" 2>$null | Out-Null # Last resort
        }
    }
}

# Step 1.5: Verify File Locks & Record Initial Hashes
Write-Host "[1.5/4] Verifying File Locks & Recording Hashes..." -ForegroundColor Yellow
$CheckFiles = @($ExePath, (Join-Path $PSScriptRoot "test_scrollbench/deploy/appScrollBench.exe"))
$InitialHashes = @{}

foreach ($file in $CheckFiles) {
    if (Test-Path $file) {
        # Check for Lock
        $locked = $true
        $waitCount = 0
        while ($locked) {
            try {
                $fileStream = [System.IO.File]::Open($file, 'Open', 'Read', 'None')
                $fileStream.Close()
                $fileStream.Dispose()
                $locked = $false
                if ($waitCount -gt 0) {
                    Write-Host "  Lock released! Continuing..." -ForegroundColor Green
                }
            }
            catch {
                if ($waitCount -eq 0) {
                    Write-Host "File is LOCKED by another process: $file" -ForegroundColor Red
                    Write-Host "  Waiting for remote instances to be closed..." -ForegroundColor Yellow
                }
                Start-Sleep -Seconds 2
                $waitCount++
            }
        }
        
        # Record Hash
        $hash = (Get-FileHash -Path $file -Algorithm SHA256).Hash
        $InitialHashes[$file] = $hash
        Write-Host "  Current Hash ($($file | Split-Path -Leaf)): $hash" -ForegroundColor Gray
    }
}

# Step 2: Clean if requested or ensure freshness
# Remove old binary to ensure we never run stale code if build fails
if (Test-Path $ExePath) {
    try {
        Remove-Item -Force $ExePath -ErrorAction Stop
    }
    catch {
        Write-Host "Could not remove old binary (still locked?): $ExePath" -ForegroundColor Red
        exit 1
    }
}

if ($Clean) {
    Write-Host "[2/4] Full Clean requested. Removing build directory..." -ForegroundColor Magenta
    if (Test-Path $BuildDir) {
        Remove-Item -Recurse -Force $BuildDir | Out-Null
    }
}
else {
    # We remove the autogen folder to force Qt to re-parse QML and Signals
    if (Test-Path "$BuildDir/QGalleryX_autogen") {
        Remove-Item -Recurse -Force "$BuildDir/QGalleryX_autogen" | Out-Null
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

    # Deploy Qt dependencies
    Write-Host "  -> Deploying Qt Dependencies..." -ForegroundColor Gray
    $WindeployQt = "D:\Qt\6.9.3\mingw_64\bin\windeployqt.exe"
    
    # Standard deploy for Main App
    & $WindeployQt --qmldir $PSScriptRoot/resources/qml --dir $BuildDir $BuildDir/QGalleryX.exe --compiler-runtime --no-opengl-sw
    
    # Standard deploy for ScrollBench (Segregated)
    $ScrollBenchDeployDir = Join-Path $PSScriptRoot "test_scrollbench/deploy"
    if (Test-Path "$ScrollBenchDeployDir/QGalleryXBench.exe") {
        Write-Host "  -> Deploying QGalleryXBench to test_scrollbench/deploy..." -ForegroundColor Gray
        & $WindeployQt --qmldir $PSScriptRoot/test_scrollbench/qml --dir $ScrollBenchDeployDir "$ScrollBenchDeployDir/QGalleryXBench.exe" --compiler-runtime --no-opengl-sw
    }
}
catch {
    Write-Host "BUILD FAILED!" -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Red
    exit 1
}

# Step 4: Report and Freshness Verification
Write-Host "[4/4] Finalizing and Verifying Freshness..." -ForegroundColor Yellow

$FreshnessMet = $true
foreach ($file in $CheckFiles) {
    if (Test-Path $file) {
        $finalHash = (Get-FileHash -Path $file -Algorithm SHA256).Hash
        if ($InitialHashes.ContainsKey($file)) {
            if ($finalHash -eq $InitialHashes[$file]) {
                Write-Host "  WARNING: Binary is STALE (Hash unchanged): $($file | Split-Path -Leaf)" -ForegroundColor Red
                $FreshnessMet = $false
            }
            else {
                Write-Host "  Binary is FRESH: $($file | Split-Path -Leaf)" -ForegroundColor Green
            }
        }
        else {
            Write-Host "  Binary is NEW: $($file | Split-Path -Leaf)" -ForegroundColor Cyan
        }
    }
}

if (Test-Path $ExePath) {
    Write-Host "==========================================" -ForegroundColor Green
    Write-Host " BUILD SUCCESS" -ForegroundColor Green
    Write-Host " Status:      $(if ($FreshnessMet) { "Binary is FRESH" } else { "Binary is STALE" })" -ForegroundColor $(if ($FreshnessMet) { "Green" } else { "Yellow" })
    Write-Host " Stable:      $(Resolve-Path $ExePath)" -ForegroundColor Magenta
    
    $ScrollBenchExePath = Join-Path $PSScriptRoot "test_scrollbench/deploy/appScrollBench.exe"
    if (Test-Path $ScrollBenchExePath) {
        Write-Host " ScrollBench: $(Resolve-Path $ScrollBenchExePath)" -ForegroundColor Cyan
    }
}
else {
    Write-Host "Build finished but binary not found!" -ForegroundColor Red
    exit 1
}

# Run Linkage Verification (Safety Check)
Write-Host " Verifying Module Linkage..." -ForegroundColor Yellow
$LinkageTool = Join-Path $PSScriptRoot "test_scrollbench/deploy/tst_linkage.exe"
if (Test-Path $LinkageTool) {
    & $LinkageTool
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Module linkage verification FAILED! Please check backend integration."
        exit 1
    }
}

# Auto-deploy to Dist so user runs fresh code
Write-Host "[5/5] Deploying to Dist folder..." -ForegroundColor Yellow
& "$PSScriptRoot\deploy.ps1"
if ($LASTEXITCODE -ne 0) {
    Write-Host "Warning: Deploy to Dist failed." -ForegroundColor Red
} else {
    Write-Host "Successfully deployed to Dist!" -ForegroundColor Green
}

