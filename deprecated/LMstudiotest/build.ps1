param(
    [string]$QtPath = "",
    [switch]$Clean = $false,
    [switch]$Verbose = $false,
    [switch]$SkipValidation = $false
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
    exit 1
}

function Write-Warn {
    param([string]$Message)
    Write-Host "[WARN] $Message" -ForegroundColor Yellow
}

# Set script directory
$scriptDir = $PSScriptRoot
Write-Log "Script directory: $scriptDir"

# Change to script directory
Set-Location $scriptDir

# Run pre-build validation
if (-not $SkipValidation) {
    Write-Log "Running pre-build validation..."
    & "$scriptDir\validate_build.ps1" -QtPath $QtPath -Verbose:$Verbose
    if ($LASTEXITCODE -ne 0) {
        Write-ErrorLog "Pre-build validation failed"
    }
}

# Create build directory
$buildDir = Join-Path $scriptDir "build"
if (-not (Test-Path $buildDir)) {
    Write-Log "Creating build directory: $buildDir"
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

# Clean build if requested
if ($Clean) {
    Write-Log "Cleaning build directory..."
    Remove-Item -Path "$buildDir\*" -Recurse -Force -ErrorAction SilentlyContinue
}

# Set Qt environment variables
if (-not [string]::IsNullOrEmpty($QtPath)) {
    $qtBinPath = Join-Path $QtPath "bin"
    Write-Log "Setting Qt environment variables..."
    $env:Path = "$qtBinPath;$env:Path"
    $env:Qt6_DIR = $QtPath
}

# Set Visual Studio environment variables
Write-Log "Setting up Visual Studio environment..."
$vsWherePath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vsWherePath) {
    $vsPath = & $vsWherePath -latest -property installationPath
    $vcvarsPath = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"
    if (Test-Path $vcvarsPath) {
        Write-Log "Initializing Visual Studio environment..."
        
        # Use a temporary file to capture environment variables cleanly
        $tempFile = [System.IO.Path]::GetTempFileName()
        
        try {
            # Get environment variables and filter out the Visual Studio banner
            $envVars = cmd /c "`"$vcvarsPath`" && set" 2>$null | Where-Object { $_ -match "^(PATH|INCLUDE|LIB|LIBPATH)=" -and $_ -notmatch "Copyright" }
            $envVars | Out-File $tempFile
            
            # Apply environment variables
            Get-Content $tempFile | ForEach-Object {
                if ($_ -match "^([^=]+)=(.*)") {
                    Set-Item -Path "env:$($matches[1])" -Value $matches[2]
                    Write-Log "Set environment variable: $($matches[1])"
                }
            }
            
            # Verify compiler is available
            $compilerCheck = cmd /c "cl.exe 2>&1" 2>$null
            if ($compilerCheck -match "Microsoft") {
                Write-Log "MSVC compiler successfully initialized"
            }
            else {
                Write-Log "ERROR: MSVC compiler still not available"
            }
        }
        finally {
            Remove-Item $tempFile -ErrorAction SilentlyContinue
        }
    }
}

# Configure CMake
Write-Log "Configuring CMake..."
try {
    # Use a different approach - run CMake directly after setting up the environment
    $envVars = cmd /c "`"$vcvarsPath`" && set" 2>$null | Where-Object { $_ -match "^(PATH|INCLUDE|LIB|LIBPATH)=" }
    foreach ($envVar in $envVars) {
        if ($envVar -match "^([^=]+)=(.*)") {
            Set-Item -Path "env:$($matches[1])" -Value $matches[2]
        }
    }
    
    # Now run CMake with the environment already set
    $cmakeCommand = "cmake -B $buildDir -G `"Visual Studio 17 2022`" -A x64"
    Write-Log "Running CMake command: $cmakeCommand"
    
    $result = cmd /c $cmakeCommand
    $exitCode = $LASTEXITCODE
    
    Write-Log "CMake configuration result:"
    Write-Log $result
    
    if ($exitCode -eq 0) {
        Write-Log "CMake configuration successful"
    }
    else {
        Write-ErrorLog "CMake configuration failed with exit code: $exitCode"
    }
}
catch {
    Write-ErrorLog "CMake configuration failed: $($_.Exception.Message)"
}

# Build the project
Write-Log "Building the project..."
try {
    # Use the same approach - run build directly after setting up the environment
    $buildCommand = "cmake --build $buildDir --config Release"
    Write-Log "Running build command: $buildCommand"
    
    $result = cmd /c $buildCommand
    $exitCode = $LASTEXITCODE
    
    Write-Log "Build result:"
    Write-Log $result
    
    if ($exitCode -eq 0) {
        Write-Log "Build successful"
    }
    else {
        Write-ErrorLog "Build failed with exit code: $exitCode"
    }
}
catch {
    Write-ErrorLog "Build failed: $($_.Exception.Message)"
}

# Copy Qt DLLs if needed
Write-Log "Copying Qt DLLs..."
if ($QtPath) {
    $qtBinPath = Join-Path $QtPath "bin"
    $outputDir = Join-Path $buildDir "Release"
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
            Write-Warn "DLL not found: $dll"
        }
    }
}

# Run tests
Write-Log "Running tests..."
try {
    $testExe = Join-Path $buildDir "Release\tst_helloworld.exe"
    if (Test-Path $testExe) {
        & $testExe
        if ($LASTEXITCODE -ne 0) {
            Write-Warn "Tests failed with exit code: $LASTEXITCODE"
        }
        else {
            Write-Log "All tests passed"
        }
    }
    else {
        Write-Warn "Test executable not found: $testExe"
    }
}
catch {
    Write-Warn "Test execution failed: $($_.Exception.Message)"
}

# Run the application
Write-Log "Running the application..."
try {
    $appExe = Join-Path $buildDir "Release\HelloWorld.exe"
    if (Test-Path $appExe) {
        Write-Log "Starting application..."
        Start-Process $appExe
        Write-Log "Application started successfully"
    }
    else {
        Write-ErrorLog "Application executable not found: $appExe"
    }
}
catch {
    Write-ErrorLog "Failed to start application: $($_.Exception.Message)"
}

Write-Log "Build process completed successfully"