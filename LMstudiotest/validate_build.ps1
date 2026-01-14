param(
    [string]$QtPath = "",
    [switch]$Verbose = $false
)

function Write-Log {
    param([string]$Message)
    if ($Verbose) {
        Write-Host "[INFO] $Message" -ForegroundColor Green
    }
}

function Write-ErrorLog {
    param([string]$Message)
    Write-Host "[ERROR] $Message" -ForegroundColor Red
}

function Write-Warn {
    param([string]$Message)
    Write-Host "[WARN] $Message" -ForegroundColor Yellow
}

# Check if running in PowerShell
if (-not $PSVersionTable) {
    Write-ErrorLog "This script must be run in PowerShell"
    exit 1
}

Write-Log "Starting LMStudioTest Build Validation"

# Check Qt 6.4 Installation
Write-Log "Checking Qt 6.4 Installation..."
if ([string]::IsNullOrEmpty($QtPath)) {
    # Try to find Qt in common locations
    $possibleQtPaths = @(
        "D:/Qt/6.9.3/msvc2022_64",
        "C:/Qt/6.9.3/msvc2022_64",
        "D:/Qt/6.9.3/msvc2019_64",
        "C:/Qt/6.9.3/msvc2019_64",
        "C:/Qt/6.4.0/msvc2019_64",
        "C:/Qt/6.4.0/msvc2022_64",
        "D:/Qt/6.4.0/msvc2019_64",
        "D:/Qt/6.4.0/msvc2022_64",
        "C:/Qt/Qt6.4.0/msvc2019_64",
        "C:/Qt/Qt6.4.0/msvc2022_64"
    )
    
    $QtPath = $null
    foreach ($path in $possibleQtPaths) {
        if (Test-Path $path) {
            $QtPath = $path
            break
        }
    }
    
    if (-not $QtPath) {
        Write-Warn "Qt 6.4 not found in common locations"
        Write-Log "Please install Qt 6.4 from https://www.qt.io/download"
        Write-Log "Or specify Qt path using -QtPath parameter"
    }
    else {
        Write-Log "Qt 6.4 found at: $QtPath"
    }
}
else {
    Write-Log "Qt 6.4 path specified: $QtPath"
}

# Verify Qt bin directory if path is provided
if ($QtPath) {
    $qtBinPath = Join-Path $QtPath "bin"
    if (-not (Test-Path $qtBinPath)) {
        Write-ErrorLog "Qt bin directory not found: $qtBinPath"
        exit 1
    }
    
    Write-Log "Qt bin directory verified: $qtBinPath"
    
    # Check for essential Qt tools
    $requiredTools = @("qmake.exe", "cmake.exe")
    foreach ($tool in $requiredTools) {
        $toolPath = Join-Path $qtBinPath $tool
        if (-not (Test-Path $toolPath)) {
            Write-Warn "Qt tool not found: $toolPath"
        }
        else {
            Write-Log "Qt tool found: $tool"
        }
    }
}

# Check CMake
Write-Log "Checking CMake..."
try {
    $cmakeVersion = cmake --version | Select-Object -First 1
    Write-Log "CMake version: $cmakeVersion"
    
    if ($cmakeVersion -match "(\d+\.\d+\.\d+)") {
        $version = $matches[1]
        $major, $minor, $patch = $version.Split('.')
        
        if ([int]$major -lt 3 -or ([int]$major -eq 3 -and [int]$minor -lt 16)) {
            Write-Warn "CMake version 3.16 or higher recommended"
        }
    }
}
catch {
    Write-ErrorLog "CMake not found or not accessible"
    exit 1
}

# Check for C++ compiler (basic check)
Write-Log "Checking C++ compiler..."
try {
    # Try to find cl.exe (MSVC)
    $clPath = Get-Command "cl.exe" -ErrorAction SilentlyContinue
    if ($clPath) {
        Write-Log "MSVC compiler found: $($clPath.Source)"
    }
    else {
        Write-Warn "MSVC compiler not found in PATH"
        Write-Log "Make sure Visual Studio is installed with C++ workload"
    }
}
catch {
    Write-Warn "Unable to detect C++ compiler"
}

# Check project structure
Write-Log "Checking project structure..."
$requiredFiles = @(
    "src/main.cpp",
    "src/main.qml", 
    "tests/tst_helloworld.cpp",
    "CMakeLists.txt"
)

foreach ($file in $requiredFiles) {
    if (Test-Path $file) {
        Write-Log "Required file found: $file"
    }
    else {
        Write-ErrorLog "Required file missing: $file"
        exit 1
    }
}

Write-Log "All required files found"

# Check Qt modules availability if Qt path is provided
if ($QtPath) {
    Write-Log "Checking Qt modules availability..."
    $qtModules = @("Core", "Gui", "Quick", "Qml", "Test")
    foreach ($module in $qtModules) {
        $modulePath = Join-Path $qtBinPath "Qt6$module.dll"
        if (Test-Path $modulePath) {
            Write-Log "Qt module found: $module"
        }
        else {
            Write-Warn "Qt module not found: $module"
        }
    }
}

Write-Log "Build validation completed successfully"
Write-Log "You can now run the build script"
exit 0