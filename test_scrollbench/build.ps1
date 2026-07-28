# ScrollBench Build Script
# Kills running instances, builds, and deploys

$ErrorActionPreference = "Stop"

Write-Host "`n=== ScrollBench Build ===" -ForegroundColor Cyan

# 1. Kill any running instances
Write-Host "Stopping any running instances..." -ForegroundColor Yellow
Stop-Process -Name "QGalleryXBench" -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 500

# 1.5 Verify File Locks for network/remote execution
$exeFile = ".\deploy\QGalleryXBench.exe"
if (Test-Path $exeFile) {
    Write-Host "Verifying file lock on $exeFile..." -ForegroundColor Yellow
    $locked = $true
    $waitCount = 0
    while ($locked) {
        try {
            $fileStream = [System.IO.File]::Open($exeFile, 'Open', 'Read', 'None')
            $fileStream.Close()
            $fileStream.Dispose()
            $locked = $false
            if ($waitCount -gt 0) {
                Write-Host "  Lock released! Continuing..." -ForegroundColor Green
            }
        }
        catch {
            if ($waitCount -eq 0) {
                Write-Host "File is LOCKED by another process: $exeFile" -ForegroundColor Red
                Write-Host "  Waiting for remote instances to be closed..." -ForegroundColor Yellow
            }
            Start-Sleep -Seconds 2
            $waitCount++
        }
    }
}

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
Write-Host "Run: .\deploy\QGalleryXBench.exe" -ForegroundColor Yellow
Write-Host "`nNote: If you suspect stale code, manually delete build directory and reconfigure." -ForegroundColor DarkGray
