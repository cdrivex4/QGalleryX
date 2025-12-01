$env:PATH = "D:\Qt\6.9.3\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;$env:PATH"

$BuildDir = "build"
$DeployDir = "deploy"
$ExeName = "appSamsungGallery.exe"

if (Test-Path $DeployDir) {
    Remove-Item -Recurse -Force $DeployDir
}
mkdir $DeployDir

# Copy executable
Copy-Item "$BuildDir\$ExeName" "$DeployDir\$ExeName"

# Run windeployqt
windeployqt --qmldir resources/qml --dir $DeployDir "$DeployDir\$ExeName"

if ($LASTEXITCODE -eq 0) {
    Write-Host "Deployment successful! Output in $DeployDir" -ForegroundColor Green
}
else {
    Write-Host "Deployment failed!" -ForegroundColor Red
}
