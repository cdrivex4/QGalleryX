# setup_ffmpeg.ps1
$ErrorActionPreference = "Stop"

$ffmpegUrl = "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-gpl-shared.zip"
$destDir = "$PSScriptRoot/ffmpeg"
$zipPath = "$PSScriptRoot/ffmpeg.zip"

Write-Host "Setting up FFmpeg..."

if (Test-Path $destDir) {
    Write-Host "FFmpeg directory already exists at $destDir. Skipping download."
    exit 0
}

Write-Host "Downloading FFmpeg from $ffmpegUrl..."
Invoke-WebRequest -Uri $ffmpegUrl -OutFile $zipPath

Write-Host "Extracting..."
Expand-Archive -Path $zipPath -DestinationPath "$PSScriptRoot/temp_ffmpeg" -Force

# Move the inner folder content to $destDir
$innerFolder = Get-ChildItem "$PSScriptRoot/temp_ffmpeg" | Select-Object -First 1
Move-Item -Path "$($innerFolder.FullName)" -Destination $destDir

# Cleanup
Remove-Item $zipPath -Force
Remove-Item "$PSScriptRoot/temp_ffmpeg" -Recurse -Force

Write-Host "FFmpeg setup complete. Libraries located at $destDir"
