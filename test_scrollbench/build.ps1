# ScrollBench Build Script
# Kills running instances, builds, and deploys

$ErrorActionPreference = "Stop"

Write-Host "`n=== ScrollBench Build ===" -ForegroundColor Cyan

# 1. Kill any running instances
Write-Host "Stopping any running instances..." -ForegroundColor Yellow
Stop-Process -Name "appScrollBench" -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 500

# 2. Build (CMake handles dependencies)
Write-Host "Building..." -ForegroundColor Cyan
Set-Location build
cmake --build . --config Release -j 8
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    Set-Location ..
    exit 1
}
Set-Location ..

# 3. Deploy
Write-Host "Deploying..." -ForegroundColor Cyan
.\deploy.ps1

Write-Host "`n✅ Build and deploy complete!" -ForegroundColor Green
Write-Host "Run: .\deploy\appScrollBench.exe" -ForegroundColor Yellow
Write-Host "`nNote: If you suspect stale code, manually delete build directory and reconfigure." -ForegroundColor DarkGray
