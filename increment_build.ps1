# Increment Build Number Script (Semantic Versioning: Major.Minor.Build)
$versionFile = "$PSScriptRoot\version.json"

# Default version structure
$version = @{
    Major = 1
    Minor = 0
    Build = 0
}

if (Test-Path $versionFile) {
    try {
        $json = Get-Content $versionFile -Raw
        $loadedVersion = $json | ConvertFrom-Json
        $version.Major = $loadedVersion.Major
        $version.Minor = $loadedVersion.Minor
        $version.Build = $loadedVersion.Build
    }
    catch {
        Write-Warning "Failed to parse version.json. Resetting to 1.0.0"
    }
}

# Auto-increment Build number
$version.Build++

# Save updated version
$version | ConvertTo-Json | Out-File -FilePath $versionFile -Encoding ASCII

# Format version string
$versionString = "$($version.Major).$($version.Minor).$($version.Build)"
$timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"

# Generate BuildInfo.h
$headerContent = @"
#pragma once
// Auto-generated build information - DO NOT EDIT MANUALLY

#define BUILD_NUMBER $($version.Build)
#define BUILD_MAJOR $($version.Major)
#define BUILD_MINOR $($version.Minor)
#define BUILD_TIMESTAMP "$timestamp"
#define BUILD_VERSION "$versionString"

namespace BuildInfo {
    constexpr int Major = $($version.Major);
    constexpr int Minor = $($version.Minor);
    constexpr int Build = $($version.Build);
    constexpr const char* Timestamp = "$timestamp";
    constexpr const char* Version = "$versionString";
}
"@

$headerPath = "$PSScriptRoot\src\BuildInfo.h"
$headerContent | Out-File -FilePath $headerPath -Encoding UTF8
$headerPathLegacy = "$PSScriptRoot\src_legacy\BuildInfo.h"
$headerContent | Out-File -FilePath $headerPathLegacy -Encoding UTF8
$headerPathBench = "$PSScriptRoot\test_scrollbench\src\BuildInfo.h"
$headerContent | Out-File -FilePath $headerPathBench -Encoding UTF8

# Generate BuildInfo.qml singleton
$qmlContent = @"
pragma Singleton
import QtQuick

QtObject {
    readonly property int major: $($version.Major)
    readonly property int minor: $($version.Minor)
    readonly property int build: $($version.Build)
    readonly property string buildTimestamp: "$timestamp"
    readonly property string version: "$versionString"
    
    // Helper to verify version matches C++
    function checkMismatch(cppVersion) {
        return version !== cppVersion
    }
}
"@

$qmlPathLegacy = "$PSScriptRoot\resources\qml_legacy\BuildInfo.qml"
$qmlContent | Out-File -FilePath $qmlPathLegacy -Encoding UTF8
$qmlPathScrollBench = "$PSScriptRoot\test_scrollbench\qml\BuildInfo.qml"
$qmlContent | Out-File -FilePath $qmlPathScrollBench -Encoding UTF8

Write-Host "Build Version: $versionString" -ForegroundColor Green
Write-Host "Timestamp: $timestamp" -ForegroundColor Cyan
