$currentDir = Get-Location
$deployDir = Join-Path $PSScriptRoot "test_scrollbench/deploy"
$exePath = Join-Path $deployDir "appScrollBench.exe"

if (!(Test-Path $exePath)) {
    Write-Host "ERROR: appScrollBench.exe not found in $deployDir" -ForegroundColor Red
    exit 1
}

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "   ScrollBench Watchdog Runner" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host " -> Launching: $exePath" -ForegroundColor Gray
Write-Host " -> Process will stay open on crash for feedback." -ForegroundColor Yellow

# Use Start-Process with -Wait and capture the exit code
$process = Start-Process -FilePath $exePath -WorkingDirectory $deployDir -PassThru -Wait

Write-Host "`n==========================================" -ForegroundColor Cyan
Write-Host "   Process Exited" -ForegroundColor Cyan
Write-Host "   Exit Code: $($process.ExitCode)" -ForegroundColor ($process.ExitCode -eq 0 ? "Green" : "Red")

if ($process.ExitCode -ne 0) {
    Write-Host "`n[CRASH DETECTED]" -ForegroundColor Red
    Write-Host "Searching System Event Log for technical details..." -ForegroundColor Yellow
    
    # Get last 2 minutes of Application Errors related to the EXE
    $foundEvents = Get-WinEvent -FilterHashtable @{LogName = 'Application'; Level = 2; StartTime = (Get-Date).AddMinutes(-2) } -ErrorAction SilentlyContinue | 
    Where-Object { $_.Message -like "*appScrollBench.exe*" }

    if ($foundEvents) {
        foreach ($ev in $foundEvents) {
            Write-Host "`n--- Event Viewer Entry ---" -ForegroundColor Cyan
            Write-Host $ev.Message -ForegroundColor White
            Write-Host "---------------------------" -ForegroundColor Cyan
        }
    }
    else {
        Write-Host "No explicit Event Log entries found. It might have been a silent DLL exit or stack overflow." -ForegroundColor Gray
    }
}

Write-Host "`nPress any key to close this terminal..." -ForegroundColor Gray
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
