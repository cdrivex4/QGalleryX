# ============================================================================
# ScrollBench Single Executable Build Script
# ============================================================================
# Builds a single, portable executable of ScrollBench for Windows 7-11
# Supports both x86 (32-bit) and x64 (64-bit) architectures
# ============================================================================

param(
    [ValidateSet('x86', 'x64', 'all')]
    [string]$Arch = 'x64',
    
    [switch]$Minimal = $false,
    [switch]$Compress = $false,
    [switch]$Clean = $false
)

$ErrorActionPreference = "Stop"

# ============================================================================
# Configuration
# ============================================================================

# Qt Static Build Paths (Update these to your Qt static installation)
$QtStaticPath_x64 = "D:\Qt\6.9.3-static-x64"  # Update this path
$QtStaticPath_x86 = "D:\Qt\6.9.3-static-x86"  # Update this path

# Build directories
$BuildDir_x64 = "build_single_x64"
$BuildDir_x86 = "build_single_x86"

# UPX Path (for compression)
$UpxPath = "upx.exe"  # Update if UPX is not in PATH

# ============================================================================
# Helper Functions
# ============================================================================

function Write-Header {
    param([string]$Message)
    Write-Host "`n============================================" -ForegroundColor Cyan
    Write-Host "  $Message" -ForegroundColor Cyan
    Write-Host "============================================`n" -ForegroundColor Cyan
}

function Write-Step {
    param([string]$Message)
    Write-Host ">>> $Message" -ForegroundColor Yellow
}

function Write-Success {
    param([string]$Message)
    Write-Host "✓ $Message" -ForegroundColor Green
}

function Write-Error-Custom {
    param([string]$Message)
    Write-Host "✗ $Message" -ForegroundColor Red
}

function Test-QtStatic {
    param([string]$QtPath, [string]$Arch)
    
    if (-not (Test-Path $QtPath)) {
        Write-Error-Custom "Qt static build not found at: $QtPath"
        Write-Host "  "
        Write-Host "  You need to build Qt statically first." -ForegroundColor Yellow
        Write-Host "  See: docs/QT_STATIC_BUILD.md for instructions" -ForegroundColor Yellow
        Write-Host "  Or update the path in this script if Qt static is installed elsewhere." -ForegroundColor Yellow
        return $false
    }
    
    $QtCmakeDir = Join-Path $QtPath "lib\cmake\Qt6"
    if (-not (Test-Path $QtCmakeDir)) {
        Write-Error-Custom "Qt CMake files not found at: $QtCmakeDir"
        return $false
    }
    
    Write-Success "Found Qt static build ($Arch) at: $QtPath"
    return $true
}

function Build-Architecture {
    param(
        [string]$TargetArch,
        [string]$QtPath,
        [string]$BuildDir
    )
    
    Write-Header "Building ScrollBench Portable - $TargetArch"
    
    # Verify Qt static build
    if (-not (Test-QtStatic -QtPath $QtPath -Arch $TargetArch)) {
        throw "Qt static build verification failed for $TargetArch"
    }
    
    # Clean if requested
    if ($Clean -and (Test-Path $BuildDir)) {
        Write-Step "Cleaning previous build..."
        Remove-Item -Recurse -Force $BuildDir
        Write-Success "Build directory cleaned"
    }
    
    # Create build directory
    if (-not (Test-Path $BuildDir)) {
        New-Item -ItemType Directory -Path $BuildDir | Out-Null
    }
    
    # Configure CMake
    Write-Step "Configuring CMake for $TargetArch..."
    
    $cmakeArgs = @(
        "-G", "Ninja",
        "-DCMAKE_BUILD_TYPE=Release",
        "-DBUILD_SINGLE_EXE=ON",
        "-DTARGET_ARCH=$TargetArch",
        "-DQt6_DIR=$QtPath\lib\cmake\Qt6",
        "-DENABLE_LTO=ON",
        "-DSTRIP_SYMBOLS=ON"
    )
    
    if ($Minimal) {
        $cmakeArgs += "-DMINIMAL_BUILD=ON"
    }
    
    $cmakeArgs += @("-S", ".", "-B", $BuildDir)
    
    & cmake @cmakeArgs
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed for $TargetArch"
    }
    Write-Success "CMake configuration complete"
    
    # Build
    Write-Step "Building $TargetArch executable..."
    cmake --build $BuildDir --config Release
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed for $TargetArch"
    }
    Write-Success "Build complete"
    
    # Find the executable
    $exeName = "ScrollBenchPortable_$TargetArch.exe"
    $exePath = Join-Path $BuildDir $exeName
    
    if (-not (Test-Path $exePath)) {
        throw "Executable not found at: $exePath"
    }
    
    # Get file size
    $fileSize = (Get-Item $exePath).Length
    $fileSizeMB = [math]::Round($fileSize / 1MB, 2)
    Write-Success "Executable created: $exePath ($fileSizeMB MB)"
    
    # Compress with UPX if requested
    if ($Compress) {
        Write-Step "Compressing with UPX..."
        
        # Check if UPX is available
        try {
            $upxVersion = & $UpxPath --version 2>&1 | Select-Object -First 1
            Write-Host "  Using: $upxVersion" -ForegroundColor Gray
        }
        catch {
            Write-Error-Custom "UPX not found. Skipping compression."
            Write-Host "  Download from: https://upx.github.io/" -ForegroundColor Yellow
            return $exePath
        }
        
        # Create backup
        $backupPath = "$exePath.backup"
        Copy-Item $exePath $backupPath -Force
        
        # Compress
        & $UpxPath --best --lzma $exePath
        if ($LASTEXITCODE -ne 0) {
            Write-Error-Custom "UPX compression failed. Restoring backup."
            Move-Item $backupPath $exePath -Force
        }
        else {
            # Get compressed size
            $compressedSize = (Get-Item $exePath).Length
            $compressedSizeMB = [math]::Round($compressedSize / 1MB, 2)
            $compressionRatio = [math]::Round(($compressedSize / $fileSize) * 100, 1)
            
            Write-Success "Compressed to $compressedSizeMB MB ($compressionRatio% of original)"
            Remove-Item $backupPath -Force
        }
    }
    
    return $exePath
}

# ============================================================================
# Main Build Process
# ============================================================================

Write-Header "ScrollBench Single Executable Builder"

# Change to script directory
Set-Location $PSScriptRoot

# Ensure we're in the right location
if (-not (Test-Path "../test_scrollbench")) {
    throw "Error: test_scrollbench directory not found. Run this script from single_exe directory."
}

$builtExecutables = @()

try {
    if ($Arch -eq 'all' -or $Arch -eq 'x64') {
        $exePath = Build-Architecture -TargetArch 'x64' -QtPath $QtStaticPath_x64 -BuildDir $BuildDir_x64
        $builtExecutables += $exePath
    }
    
    if ($Arch -eq 'all' -or $Arch -eq 'x86') {
        $exePath = Build-Architecture -TargetArch 'x86' -QtPath $QtStaticPath_x86 -BuildDir $BuildDir_x86
        $builtExecutables += $exePath
    }
    
    # ========================================================================
    # Final Summary
    # ========================================================================
    
    Write-Header "Build Successful!"
    
    Write-Host "Built Executables:" -ForegroundColor Green
    foreach ($exe in $builtExecutables) {
        $fullPath = Resolve-Path $exe
        $size = (Get-Item $exe).Length
        $sizeMB = [math]::Round($size / 1MB, 2)
        Write-Host "  • $fullPath ($sizeMB MB)" -ForegroundColor Cyan
    }
    
    Write-Host "`nNext Steps:" -ForegroundColor Yellow
    Write-Host "  1. Test the executable(s) on target systems"
    Write-Host "  2. Verify compatibility (Windows 7-11)"
    Write-Host "  3. Package for distribution"
    
    if (-not $Compress) {
        Write-Host "`nTip: Use -Compress flag to reduce file size with UPX" -ForegroundColor DarkGray
    }
}
catch {
    Write-Header "Build Failed!"
    Write-Error-Custom $_.Exception.Message
    exit 1
}
