# Antigravity Development Watcher
# Automatically triggers the Matrix Automation Suite on file changes.

param (
    [string]$ScanPath = "D:/Pictures", # Default path to test
    [string]$SourceDir = "D:/Dev/antigravity/src"
)

Write-Host "`n[Watcher] Monitoring $SourceDir for changes..." -ForegroundColor Cyan
Write-Host "[Watcher] Test Target: $ScanPath"
Write-Host "[Watcher] Press Ctrl+C to stop.`n"

$fsw = New-Object IO.FileSystemWatcher $SourceDir, "*.cpp"
$fsw.IncludeSubdirectories = $true
$fsw.EnableRaisingEvents = $true

$action = {
    $path = $Event.SourceEventArgs.FullPath
    Write-Host "`n[Change Detected] $path" -ForegroundColor Yellow
    Write-Host "[Action] Rebuilding & Running Matrix Suite..." -ForegroundColor Gray
    
    # 1. Rebuild (Quick incremental)
    powershell -ExecutionPolicy Bypass -File D:/Dev/antigravity/build.ps1
    
    if ($LASTEXITCODE -eq 0) {
        # 2. Run Automation Matrix
        powershell -ExecutionPolicy Bypass -File D:/Dev/antigravity/scripts/run_automation_suite.ps1 $ScanPath
    }
    else {
        Write-Host "[Error] Build failed! Check compiler output." -ForegroundColor Red
    }
}

Register-ObjectEvent $fsw "Changed" -Action $action | Out-Null
Register-ObjectEvent $fsw "Created" -Action $action | Out-Null

while ($true) { Start-Sleep 1 }
