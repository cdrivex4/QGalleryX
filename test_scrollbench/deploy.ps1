# ScrollBench Deployment Script
param(
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

# Paths
$BuildDir = "build"
$DeployDir = "deploy"
$QtPath = "D:\Qt\6.9.3\mingw_64"
$QtBinPath = "$QtPath\bin"
$MingwPath = "D:\Qt\Tools\mingw1310_64\bin"

# Kill any running instances
Write-Host "Stopping any running instances..." -ForegroundColor Yellow
Stop-Process -Name "appScrollBench" -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 500

if ($Clean -and (Test-Path $DeployDir)) {
    Write-Host "Cleaning deploy directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $DeployDir
}

if (-not (Test-Path $DeployDir)) {
    New-Item -ItemType Directory -Path $DeployDir | Out-Null
}

# Copy executable
Write-Host "Copying executable..." -ForegroundColor Cyan
if (Test-Path "$BuildDir\appScrollBench.exe") {
    Copy-Item "$BuildDir\appScrollBench.exe" $DeployDir -Force
} elseif (Test-Path "$DeployDir\appScrollBench.exe") {
    Write-Host "Executable already in deploy folder." -ForegroundColor Gray
} else {
    throw "appScrollBench.exe not found!"
}

# Deploy Qt dependencies
Write-Host "Deploying Qt dependencies..." -ForegroundColor Cyan
& "$QtBinPath\windeployqt.exe" `
    --qmldir qml `
    --release `
    "$DeployDir\appScrollBench.exe"

# Copy MinGW runtime
Write-Host "Copying MinGW runtime..." -ForegroundColor Cyan
Copy-Item "$MingwPath\libgcc_s_seh-1.dll" $DeployDir -Force
Copy-Item "$MingwPath\libstdc++-6.dll" $DeployDir -Force
Copy-Item "$MingwPath\libwinpthread-1.dll" $DeployDir -Force

# Copy FFmpeg DLLs
Write-Host "Copying FFmpeg DLLs..." -ForegroundColor Cyan
$FfmpegBin = "..\3rdparty\ffmpeg\bin"
if (Test-Path $FfmpegBin) {
    Copy-Item "$FfmpegBin\*.dll" $DeployDir -Force
}

Write-Host "`n✅ Deployment complete!" -ForegroundColor Green
Write-Host "Run: .\deploy\appScrollBench.exe" -ForegroundColor Yellow
