# ScrollBench - Automation Suite & Scenario Runner
# This script executes a complete 'User Scenario' tree including:
# - Recursive Discovery
# - Viewport Simulation
# - Selection & Batch Logic
# - Metadata verification

param (
    [string]$TargetFolder = "."
)

$AutomationTool = "d:\Dev\antigravity\test_scrollbench\deploy\tst_automation.exe"
if (-not (Test-Path $AutomationTool)) {
    $AutomationTool = "d:\Dev\antigravity\build\test_scrollbench\tst_automation.exe"
}

if (-not (Test-Path $AutomationTool)) {
    Write-Error "Could not find tst_automation.exe. Please run build.ps1 first."
    exit 1
}

Write-Host "`n[Automated Suite] Running Scenario Tree for: $TargetFolder" -ForegroundColor Cyan
Write-Host "--------------------------------------------------------"

# Run the scenario automation tool
& $AutomationTool --scan $TargetFolder

if ($LASTEXITCODE -eq 0) {
    Write-Host "`n[SUCCESS] All scenarios passed. System integrity verified via action tree." -ForegroundColor Green
}
else {
    Write-Host "`n[FAILURE] One or more scenarios failed. Check output above for 'FAIL' status." -ForegroundColor Red
    exit $LASTEXITCODE
}
