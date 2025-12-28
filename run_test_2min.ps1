$ErrorActionPreference = "Stop"
$logFile = "d:\Dev\antigravity\logs\runtime_test.log"
$exe = "d:\Dev\antigravity\test_scrollbench\deploy\appScrollBench.exe"
$workDir = "d:\Dev\antigravity\test_scrollbench\deploy"

# Create logs dir if not exists
New-Item -ItemType Directory -Force -Path "d:\Dev\antigravity\logs" | Out-Null

# Clean old log
if (Test-Path $logFile) { Remove-Item $logFile }

Write-Host "Starting ScrollBench and logging to $logFile..."
$p = Start-Process -FilePath $exe -WorkingDirectory $workDir -RedirectStandardOutput $logFile -RedirectStandardError $logFile -PassThru

if (!$p) {
    Write-Error "Failed to start application."
    exit 1
}

Write-Host "App started with PID $($p.Id). Sleeping for 120 seconds..."
Start-Sleep -Seconds 120

Write-Host "Terminating application..."
Stop-Process -InputObject $p -Force
Write-Host "Test Complete."
