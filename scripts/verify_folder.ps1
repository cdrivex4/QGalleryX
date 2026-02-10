# ScrollBench - Automation & Integrity Verification Script
# This script performs headless verification of a directory to validate:
# 1. Module Linkage (Backend stability)
# 2. Media Detection (Image, Video, RAW counts)
# 3. Path Normalization (Sanity check)

param (
    [string]$TargetFolder = "."
)

$LinkageTool = "d:\Dev\antigravity\test_scrollbench\deploy\tst_linkage.exe"
if (-not (Test-Path $LinkageTool)) {
    $LinkageTool = "d:\Dev\antigravity\build\test_scrollbench\tst_linkage.exe"
}

if (-not (Test-Path $LinkageTool)) {
    Write-Error "Could not find tst_linkage.exe. Please run build.ps1 first."
    exit 1
}

Write-Host "`n[Automated Verification] Testing folder: $TargetFolder" -ForegroundColor Cyan
Write-Host "--------------------------------------------------------"

# Run the linkage tool with headless scan
& $LinkageTool --scan $TargetFolder

if ($LASTEXITCODE -eq 0) {
    Write-Host "`n[SUCCESS] Integrity verified. Backend modules are healthy and scanning correctly." -ForegroundColor Green
}
else {
    Write-Host "`n[FAILURE] Integrity check failed with exit code $LASTEXITCODE." -ForegroundColor Red
    exit $LASTEXITCODE
}
