param(
    [switch]$Clean = $false,
    [switch]$Verbose = $false
)

function Write-Log {
    param([string]$Message)
    if ($Verbose) {
        Write-Host "[INFO] $Message" -ForegroundColor Green
    }
}

# Set script directory
$scriptDir = $PSScriptRoot
Write-Log "Script directory: $scriptDir"

# Change to parent directory (main project)
Set-Location $scriptDir\..

# Clean build if requested
if ($Clean) {
    Write-Log "Cleaning build directory..."
    Remove-Item -Path "build\LMstudiotest" -Recurse -Force -ErrorAction SilentlyContinue
}

# Set Qt environment variables
$qtPath = "D:/Qt/6.9.3/msvc2022_64"
$qtBinPath = Join-Path $qtPath "bin"
Write-Log "Setting Qt environment variables..."
$env:Path = "$qtBinPath;$env:Path"
$env:Qt6_DIR = $qtPath

# Find Visual Studio installation
$vsWherePath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vsWherePath) {
    $vsPath = & $vsWherePath -latest -property installationPath
    $vcvarsPath = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"
    
    if (Test-Path $vcvarsPath) {
        Write-Log "Found Visual Studio at: $vsPath"
        
        # Configure CMake within Visual Studio environment
        Write-Log "Configuring CMake..."
        $cmakeConfig = cmd /c "`"$vcvarsPath`" && cmake -B build/LMstudiotest -S LMstudiotest -G `"Visual Studio 17 2022`" -A x64 2>&1"
        $exitCode = $LASTEXITCODE
        
        if ($Verbose) {
            Write-Log "CMake configuration output:"
            Write-Log $cmakeConfig
        }
        
        if ($exitCode -ne 0) {
            Write-Host "[ERROR] CMake configuration failed with exit code: $exitCode" -ForegroundColor Red
            exit $exitCode
        }
        
        # Build the project
        Write-Log "Building the project..."
        $cmakeBuild = cmd /c "`"$vcvarsPath`" && cmake --build build/LMstudiotest --config Release 2>&1"
        $exitCode = $LASTEXITCODE
        
        if ($Verbose) {
            Write-Log "CMake build output:"
            Write-Log $cmakeBuild
        }
        
        if ($exitCode -ne 0) {
            Write-Host "[ERROR] Build failed with exit code: $exitCode" -ForegroundColor Red
            exit $exitCode
        }
        
        # Copy Qt DLLs
        Write-Log "Copying Qt DLLs..."
        $outputDir = Join-Path "build/LMstudiotest" "Release"
        $requiredDlls = @(
            "Qt6Core.dll",
            "Qt6Gui.dll", 
            "Qt6Quick.dll",
            "Qt6Qml.dll"
        )
        
        foreach ($dll in $requiredDlls) {
            $sourceDll = Join-Path $qtBinPath $dll
            $destDll = Join-Path $outputDir $dll
            
            if (Test-Path $sourceDll) {
                Copy-Item $sourceDll $destDll -Force
                Write-Log "Copied $dll"
            }
            else {
                Write-Host "[WARN] DLL not found: $dll" -ForegroundColor Yellow
            }
        }
        
        # Run tests
        Write-Log "Running tests..."
        $testExe = Join-Path "build/LMstudiotest/Release" "tst_helloworld.exe"
        if (Test-Path $testExe) {
            $testResult = & $testExe
            if ($LASTEXITCODE -ne 0) {
                Write-Host "[WARN] Tests failed with exit code: $LASTEXITCODE" -ForegroundColor Yellow
            }
            else {
                Write-Log "All tests passed"
            }
        }
        else {
            Write-Host "[WARN] Test executable not found: $testExe" -ForegroundColor Yellow
        }
        
        # Run the application
        Write-Log "Running the application..."
        $appExe = Join-Path "build/LMstudiotest/Release" "HelloWorld.exe"
        if (Test-Path $appExe) {
            Write-Log "Starting application..."
            Start-Process $appExe
            Write-Log "Application started successfully"
        }
        else {
            Write-Host "[ERROR] Application executable not found: $appExe" -ForegroundColor Red
            exit 1
        }
    }
    else {
        Write-Host "[ERROR] vcvars64.bat not found at: $vcvarsPath" -ForegroundColor Red
        exit 1
    }
}
else {
    Write-Host "[ERROR] vswhere.exe not found. Visual Studio may not be installed." -ForegroundColor Red
    exit 1
}

Write-Log "Build process completed successfully"