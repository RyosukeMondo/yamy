$ErrorActionPreference = "Stop"

$root = "$PSScriptRoot\.."
$distDir = "$root\dist"
$releaseDir = "$root\dist\yamy-release"

# Clean
if (Test-Path $root\build) { Remove-Item -Recurse -Force $root\build }
if (Test-Path $distDir) { Remove-Item -Recurse -Force $distDir }
New-Item -ItemType Directory -Force -Path $releaseDir | Out-Null

# -----------------------------------------------------------------------------
# Build 64-bit
# -----------------------------------------------------------------------------
Write-Host "Building 64-bit..." -ForegroundColor Cyan
cmake -S $root -B "$root\build\x64" -A x64
if ($LASTEXITCODE -ne 0) { throw "CMake Configure 64-bit failed" }

cmake --build "$root\build\x64" --config Release
if ($LASTEXITCODE -ne 0) { throw "CMake Build 64-bit failed" }

# Copy 64-bit artifacts
Copy-Item "$root\build\x64\bin\Release\yamy.exe" "$releaseDir\"
Copy-Item "$root\build\x64\bin\Release\yamy-64-hook.exe" "$releaseDir\"
Copy-Item "$root\build\x64\bin\Release\yamy-64-hook.dll" "$releaseDir\"

# -----------------------------------------------------------------------------
# Build 32-bit
# -----------------------------------------------------------------------------
Write-Host "Building 32-bit..." -ForegroundColor Cyan
cmake -S $root -B "$root\build\x86" -A Win32
if ($LASTEXITCODE -ne 0) { throw "CMake Configure 32-bit failed" }

cmake --build "$root\build\x86" --config Release
if ($LASTEXITCODE -ne 0) { throw "CMake Build 32-bit failed" }

# Copy 32-bit artifacts
Copy-Item "$root\build\x86\bin\Release\yamy-32-hook.exe" "$releaseDir\"
Copy-Item "$root\build\x86\bin\Release\yamy-32-hook.dll" "$releaseDir\"
Copy-Item "$root\build\x86\bin\Release\yamyd32.exe" "$releaseDir\"

# -----------------------------------------------------------------------------
# Copy Assets (Keymaps, Docs, Scripts)
# -----------------------------------------------------------------------------
Write-Host "Copying Assets..." -ForegroundColor Cyan

Copy-Item -Recurse "$root\keymaps" "$releaseDir\keymaps"
# Copy only launch scripts to root
Copy-Item "$root\scripts\launch_yamy.bat" "$releaseDir\"
Copy-Item "$root\scripts\launch_yamy_admin.bat" "$releaseDir\"
Copy-Item -Recurse "$root\docs" "$releaseDir\docs"
Copy-Item "$root\docs\readme.txt" "$releaseDir\readme.txt"

# -----------------------------------------------------------------------------
# Create Zip
# -----------------------------------------------------------------------------
Write-Host "Creating Archive..." -ForegroundColor Cyan
$zipPath = "$distDir\yamy-dist.zip"
Compress-Archive -Path "$releaseDir\*" -DestinationPath $zipPath -Force

# Cleanup release dir safely (keeping zip)
Remove-Item -Recurse -Force $releaseDir

Write-Host "Success! Created $zipPath" -ForegroundColor Green
