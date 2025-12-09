# ScrollBench Build Script
# Adapted from main project build.ps1

param(
    [switch]$Clean = $false
)

$ErrorActionPreference = "Stop"

# Set PATH to include Qt and MinGW tools (from main project)
$env:PATH = "D:\Qt\6.9.3\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;D:\Qt\Tools\CMake_64\bin;D:\Qt\Tools\Ninja;$env:PATH"

# Ensure we are in the project root
Set-Location $PSScriptRoot

# Configuration
$BuildDir = "build"
$ExeName = "appScrollBench.exe"
$ExePath = Join-Path $BuildDir $ExeName

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "   ScrollBench Build System" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan

# Step 1: Kill running instances
Write-Host "[1/4] Checking for running instances..." -ForegroundColor Yellow
if (Get-Process -Name "appScrollBench" -ErrorAction SilentlyContinue) {
    Write-Host "  Killing appScrollBench..." -ForegroundColor Gray
    Stop-Process -Name "appScrollBench" -Force -ErrorAction SilentlyContinue
    
    # Wait for it to actually die
    $retries = 10
    while ((Get-Process -Name "appScrollBench" -ErrorAction SilentlyContinue) -and ($retries -gt 0)) {
        Start-Sleep -Milliseconds 500
        $retries--
    }
    
    if (Get-Process -Name "appScrollBench" -ErrorAction SilentlyContinue) {
        Write-Host "  FAILED to kill appScrollBench. File might be locked." -ForegroundColor Red
        taskkill /F /IM "appScrollBench.exe" | Out-Null
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
    # Remove the autogen folder to force Qt to re-parse QML
    if (Test-Path "$BuildDir/appScrollBench_autogen") {
        Remove-Item -Recurse -Force "$BuildDir/appScrollBench_autogen" | Out-Null
    }
    # ALWAYS remove QML cache to prevent stale code
    if (Test-Path "$BuildDir/.rcc") {
        Remove-Item -Recurse -Force "$BuildDir/.rcc" | Out-Null
    }
    Write-Host "[2/4] Cleaned autogen & QML cache (forcing QML recompile)." -ForegroundColor Yellow
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

    # Deploy
    Write-Host "  -> Deploying..." -ForegroundColor Gray
    & (Join-Path $PSScriptRoot "deploy.ps1")

}
catch {
    Write-Host "BUILD FAILED!" -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Red
    exit 1
}

# Step 4: Report
$DeployPath = Join-Path "deploy" $ExeName

if (Test-Path $DeployPath) {
    Write-Host "==========================================" -ForegroundColor Green
    Write-Host " BUILD SUCCESSFUL" -ForegroundColor Green
    Write-Host " Binary: $(Resolve-Path $DeployPath)" -ForegroundColor Magenta
    Write-Host "==========================================" -ForegroundColor Green
}
else {
    Write-Host "Build finished but binary not found!" -ForegroundColor Red
    exit 1
}
