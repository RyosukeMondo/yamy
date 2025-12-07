$ErrorActionPreference = "Stop"

$root = "$PSScriptRoot\.."
$distDir = "$root\dist"
$releaseDir = "$root\build_release"

# Clean
if (Test-Path $releaseDir) { Remove-Item -Recurse -Force $releaseDir }
if (Test-Path $distDir) { Remove-Item -Recurse -Force $distDir }
New-Item -ItemType Directory -Force -Path $releaseDir | Out-Null
New-Item -ItemType Directory -Force -Path $distDir | Out-Null

# -----------------------------------------------------------------------------
# Build 64-bit
# -----------------------------------------------------------------------------
Write-Host "Building 64-bit..." -ForegroundColor Cyan
cmake -S $root -B "$root\build64" -A x64
if ($LASTEXITCODE -ne 0) { throw "CMake Configure 64-bit failed" }

cmake --build "$root\build64" --config Release
if ($LASTEXITCODE -ne 0) { throw "CMake Build 64-bit failed" }

# Copy 64-bit artifacts
Copy-Item "$root\build64\bin\Release\yamy.exe" "$releaseDir\"
Copy-Item "$root\build64\bin\Release\yamy-64-hook.exe" "$releaseDir\"
Copy-Item "$root\build64\bin\Release\yamy-64-hook.dll" "$releaseDir\"

# -----------------------------------------------------------------------------
# Build 32-bit
# -----------------------------------------------------------------------------
Write-Host "Building 32-bit..." -ForegroundColor Cyan
cmake -S $root -B "$root\build32" -A Win32
if ($LASTEXITCODE -ne 0) { throw "CMake Configure 32-bit failed" }

cmake --build "$root\build32" --config Release
if ($LASTEXITCODE -ne 0) { throw "CMake Build 32-bit failed" }

# Copy 32-bit artifacts
Copy-Item "$root\build32\bin\Release\yamy-32-hook.exe" "$releaseDir\"
Copy-Item "$root\build32\bin\Release\yamy-32-hook.dll" "$releaseDir\"
Copy-Item "$root\build32\bin\Release\yamyd32.exe" "$releaseDir\"

# -----------------------------------------------------------------------------
# Copy Assets (Keymaps, Docs, Scripts)
# -----------------------------------------------------------------------------
Write-Host "Copying Assets..." -ForegroundColor Cyan

Copy-Item -Recurse "$root\keymaps" "$releaseDir\keymaps"
Copy-Item -Recurse "$root\scripts" "$releaseDir\scripts"
Copy-Item -Recurse "$root\docs" "$releaseDir\docs"
Copy-Item "$root\docs\readme.txt" "$releaseDir\readme.txt"

# Remove build scripts from release
if (Test-Path "$releaseDir\scripts\build_yamy.bat") { Remove-Item "$releaseDir\scripts\build_yamy.bat" }
if (Test-Path "$releaseDir\scripts\cmake_package.ps1") { Remove-Item "$releaseDir\scripts\cmake_package.ps1" }

# -----------------------------------------------------------------------------
# Create Zip
# -----------------------------------------------------------------------------
Write-Host "Creating Archive..." -ForegroundColor Cyan
$zipPath = "$distDir\yamy-dist.zip"
Compress-Archive -Path "$releaseDir\*" -DestinationPath $zipPath -Force

Write-Host "Success! Created $zipPath" -ForegroundColor Green
