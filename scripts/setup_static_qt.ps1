# Script to Download and Build Qt 6.9.3 Statically inside the Project
# Usage: .\setup_static_qt.ps1
# Robustness: Includes retry logic, disk checks, and logging.

$ErrorActionPreference = "Stop"
$QtVersion = "6.9.3"
$ProjectRoot = Resolve-Path "$PSScriptRoot/.."
$ToolsDir = Join-Path $ProjectRoot "3rdparty/tools"
$StaticQtDir = Join-Path $ProjectRoot "3rdparty/qt_static"
$LogFile = Join-Path $PSScriptRoot "build_qt.log"

$SourceDir = "D:\Qt\Src_$QtVersion" 
$BuildDir = "D:\Qt\Build_$QtVersion"

# === HELPER FUNCTIONS ===
function Log-Me ($Message, $Color = "Cyan") {
    $Timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    Write-Host "[$Timestamp] $Message" -ForegroundColor $Color
}

function Assert-DiskSpace ($DriveLetter, $MinGB) {
    Log-Me "Checking disk space on $DriveLetter..." "Gray"
    $drive = Get-Volume -DriveLetter $DriveLetter
    $freeGB = $drive.SizeRemaining / 1GB
    if ($freeGB -lt $MinGB) {
        throw "Insufficient disk space on $DriveLetter. Required: ${MinGB}GB, Available: $([math]::Round($freeGB, 2))GB"
    }
}

function Run-Exec ($Cmd, $CmdArgs) {
    Log-Me "Exec: $Cmd $CmdArgs" "DarkGray"
    & $Cmd @CmdArgs
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code ${LASTEXITCODE}: $Cmd"
    }
}

# === START ===
try { Stop-Transcript | Out-Null } catch { } # Clear any separate session transcript
Start-Transcript -Path $LogFile -Append | Out-Null
Log-Me "=== Qt Static Build Setup (Robust) ===" "Cyan"
Log-Me "Target: $StaticQtDir" "Gray"

try {
    # 0. Pre-flight Checks
    Assert-DiskSpace "D" 15
    
    # Ensure directories exist
    New-Item -ItemType Directory -Force -Path $ToolsDir | Out-Null
    New-Item -ItemType Directory -Force -Path $SourceDir | Out-Null
    New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

    # 1. Setup Dependencies (Perl)
    Log-Me "`n[1/5] Checking Dependencies..." "Yellow"
    
    # Download Portable Perl
    $PerlDir = Join-Path $ToolsDir "perl"
    $PerlExe = Join-Path $PerlDir "bin/perl.exe"
    
    if (-not (Test-Path $PerlExe)) {
        Log-Me "  -> Downloading Strawberry Perl Portable..."
        $PerlUrl = "https://github.com/StrawberryPerl/Perl-Dist-Strawberry/releases/download/SP_53822_64bit/strawberry-perl-5.38.2.2-64bit-portable.zip"
        $PerlZip = Join-Path $ToolsDir "perl.zip"
        
        # Retry logic for download
        $maxRetries = 3
        $retryCount = 0
        while ($retryCount -lt $maxRetries) {
            try {
                Invoke-WebRequest -Uri $PerlUrl -OutFile $PerlZip
                break
            }
            catch {
                $retryCount++
                Log-Me "Download failed. Retrying ($retryCount/$maxRetries)..." "Red"
                Start-Sleep -Seconds 2
            }
        }
        if ($retryCount -eq $maxRetries) { throw "Failed to download Perl after retries." }
        
        Log-Me "  -> Extracting Perl..."
        Expand-Archive -Path $PerlZip -DestinationPath $PerlDir -Force
        Remove-Item $PerlZip
    }
    Log-Me "  -> Perl verified." "Green"

    # Add Perl to PATH
    $env:PATH = "$PerlDir/bin;$env:PATH"

    # Check tools
    $tools = @("git", "cmake", "ninja", "python")
    foreach ($tool in $tools) {
        if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
            throw "Missing required tool: $tool"
        }
    }

    # 2. Download Source
    Log-Me "`n[2/5] Downloading Qt Source (Minimal)..." "Yellow"

    function Setup-Module ($Name) {
        $Path = Join-Path $SourceDir $Name
        $GitDir = Join-Path $Path ".git"
        
        if ((-not (Test-Path $Path)) -or (-not (Test-Path $GitDir))) {
            if (Test-Path $Path) {
                Log-Me "  -> Found partial/corrupt $Name. Removing..." "Red"
                Remove-Item -Recurse -Force $Path | Out-Null
            }
            Log-Me "  -> Cloning $Name..."
            git clone "https://code.qt.io/qt/$Name.git" -b "$QtVersion" --depth 1 $Path
            if ($LASTEXITCODE -ne 0) { throw "Git clone failed for $Name" }
        }
        else {
            Log-Me "  -> $Name already exists (Verified)." "Gray"
        }
    }

    Setup-Module "qtbase"
    $Modules = @("qtdeclarative", "qtmultimedia", "qtimageformats", "qtshadertools", "qtsvg")
    foreach ($mod in $Modules) {
        Setup-Module $mod
    }

    # 3. Configure & 4. Build & 5. Install (BASE)
    $CoreLibInstalled = Join-Path $StaticQtDir "lib/libQt6Core.a"
    
    if (Test-Path $CoreLibInstalled) {
        Log-Me "`n[3-5] Base Qt (qtbase) already installed. Skipping..." "Green"
    } 
    else {
        Log-Me "`n[3/5] Configuring Build (Base)..." "Yellow"
        
        # Clean build directory to avoid CMakeCache corruption
        if (Test-Path $BuildDir) {
            Log-Me "  -> Cleaning build directory..." "Gray"
            Remove-Item -Recurse -Force $BuildDir | Out-Null
            New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
        }
        Set-Location $BuildDir
    
        # EXPLICTLY Set Compilers to avoid CMake detection errors
        $env:CC = "D:\Qt\Tools\mingw1310_64\bin\gcc.exe"
        $env:CXX = "D:\Qt\Tools\mingw1310_64\bin\g++.exe"
        
        Log-Me "  -> Compilers set to: $env:CC / $env:CXX" "Gray"
    
        # We call configure.bat directly
        $ConfigScript = "$SourceDir\qtbase\configure.bat"
        
        # Construct arguments carefully
        $ConfigArgs = @(
            "-static",
            "-release",
            "-optimize-size",
            "-prefix", "$StaticQtDir",
            "-confirm-license",
            "-opensource",
            "-nomake", "examples",
            "-nomake", "tests",
            "-DQT_BUILD_TESTS=OFF",
            "-DQT_BUILD_EXAMPLES=OFF",
            "-no-pch"
        )
    
        Run-Exec $ConfigScript $ConfigArgs
    
        # 4. Build
        Log-Me "`n[4/5] Building Base (Using Ninja/CMake)..." "Yellow"
        Run-Exec "cmake" @("--build", ".", "--parallel")
    
        # 5. Install
        Log-Me "`n[5/5] Installing Base to Project..." "Yellow"
        Run-Exec "cmake" @("--install", ".")
    }

    # 5.1 Build Additional Modules
    # Now that Base is installed, we must build the other modules against it
    $QtBin = Join-Path $StaticQtDir "bin"
    $QtConfigureModule = Join-Path $QtBin "qt-configure-module.bat"
    
    # Modules to build in order
    $ExtraModules = @("qtshadertools", "qtdeclarative", "qtimageformats", "qtsvg", "qtmultimedia")
    
    foreach ($Mod in $ExtraModules) {
        Log-Me "`n[5.1] Building Module: $Mod ..." "Magenta"
        $ModSource = Join-Path $SourceDir $Mod
        $ModBuild = Join-Path $BuildDir $Mod
        
        # Clean module build dir
        if (Test-Path $ModBuild) { Remove-Item -Recurse -Force $ModBuild | Out-Null }
        New-Item -ItemType Directory -Force -Path $ModBuild | Out-Null
        Set-Location $ModBuild
        
        # Configure Module
        Log-Me "  -> Configuring $Mod..."
        Run-Exec $QtConfigureModule $ModSource
        
        # Build Module
        Log-Me "  -> Building $Mod..."
        Run-Exec "cmake" @("--build", ".", "--parallel")
        
        # Install Module
        Log-Me "  -> Installing $Mod..."
        Run-Exec "cmake" @("--install", ".")
    }

    # 6. Verify Installation
    Log-Me "`n[6/6] Verifying Installation..." "Yellow"
    $Qmake = Join-Path $StaticQtDir "bin/qmake.exe"
    $CoreLib = Join-Path $StaticQtDir "lib/libQt6Core.a" # MinGW Static Lib

    if ((Test-Path $Qmake) -and (Test-Path $CoreLib)) {
        Log-Me "`n=== 🌟 SUCCESS: Static Qt Verified at $StaticQtDir ===" "Green"
        Log-Me "  qmake: Found" "Green"
        Log-Me "  Static Core: Found ($(Get-Item $CoreLib | Select-Object -ExpandProperty Length | ForEach-Object { $_ / 1MB } | ForEach-Object { "{0:N2} MB" -f $_ }))" "Green"
        Log-Me "`nYou can now update build.ps1 to point to this directory." "Gray"
    }
    else {
        throw "Installation completed but verification failed. Missing $Qmake or $CoreLib"
    }

}
catch {
    Log-Me "`n!!! FATAL ERROR !!!" "Red"
    Log-Me $_.Exception.Message "Red"
    Log-Me "Check log file: $LogFile" "Red"
    exit 1
}
finally {
    Stop-Transcript
}
