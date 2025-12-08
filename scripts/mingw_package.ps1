$ErrorActionPreference = "Stop"

$root = "$PSScriptRoot\.."
$distDir = "$root\dist"
$releaseDir = "$root\dist\yamy-mingw-release"
$msysDir = "C:\tools\msys64"

# -----------------------------------------------------------------------------
# Clean
# -----------------------------------------------------------------------------
Write-Output "Cleaning build and dist directories..."
if (Test-Path "$root\build\mingw64") { Remove-Item -Recurse -Force "$root\build\mingw64" }
if (Test-Path "$root\build\mingw32") { Remove-Item -Recurse -Force "$root\build\mingw32" }
if (Test-Path $distDir) { Remove-Item -Recurse -Force $distDir }
New-Item -ItemType Directory -Force -Path $releaseDir | Out-Null

# -----------------------------------------------------------------------------
# Quality Checks
# -----------------------------------------------------------------------------
Write-Output "Running QA Checks..."
& "$PSScriptRoot\check_antipatterns.ps1"
if ($LASTEXITCODE -ne 0) { throw "Anti-pattern check failed" }
& "$PSScriptRoot\check_missing_sources.ps1"
if ($LASTEXITCODE -ne 0) { throw "Missing sources check failed" }
& "$PSScriptRoot\check_encoding.ps1"
if ($LASTEXITCODE -ne 0) { throw "Encoding check failed" }


# -----------------------------------------------------------------------------
# Build 64-bit
# -----------------------------------------------------------------------------
Write-Output "Building 64-bit Target..."
$env:PATH = "$msysDir\mingw64\bin;$env:PATH"

Write-Output "Configuring CMake (64-bit)..."
cmake -S $root -B "$root\build\mingw64" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { throw "CMake Configure (64-bit) failed" }

Write-Output "Building Targets (64-bit)..."
cmake --build "$root\build\mingw64" --config Release
if ($LASTEXITCODE -ne 0) { throw "CMake Build (64-bit) failed" }

# Copy 64-bit artifacts
$binDir64 = "$root\build\mingw64\bin"
Copy-Item "$binDir64\yamy.exe" "$releaseDir\"
Copy-Item "$binDir64\yamy64.exe" "$releaseDir\"
Copy-Item "$binDir64\yamy64.dll" "$releaseDir\"
# Copy exports/lib if needed
if (Test-Path "$binDir64\yamy64.dll.a") { Copy-Item "$binDir64\yamy64.dll.a" "$releaseDir\yamy64.lib" }


# -----------------------------------------------------------------------------
# Build 32-bit
# -----------------------------------------------------------------------------
Write-Output "Building 32-bit Target..."
# Reset PATH to prioritize 32-bit, removing 64-bit MSYS entry to be safe
$env:PATH = "$msysDir\mingw32\bin;" + ($env:PATH.Replace("$msysDir\mingw64\bin;", ""))

Write-Output "Configuring CMake (32-bit)..."
cmake -S $root -B "$root\build\mingw32" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { throw "CMake Configure (32-bit) failed" }

Write-Output "Building Targets (32-bit)..."
cmake --build "$root\build\mingw32" --config Release
if ($LASTEXITCODE -ne 0) { throw "CMake Build (32-bit) failed" }

# Copy 32-bit artifacts
$binDir32 = "$root\build\mingw32\bin"
Copy-Item "$binDir32\yamy32.exe" "$releaseDir\"
Copy-Item "$binDir32\yamy32.dll" "$releaseDir\"
Copy-Item "$binDir32\yamyd32.exe" "$releaseDir\"
# Copy exports/lib if needed
if (Test-Path "$binDir32\yamy32.dll.a") { Copy-Item "$binDir32\yamy32.dll.a" "$releaseDir\yamy32.lib" }


# -----------------------------------------------------------------------------
# Copy Assets
# -----------------------------------------------------------------------------
Write-Output "Copying Assets..."
Copy-Item -Recurse "$root\keymaps" "$releaseDir\keymaps"
Copy-Item "$root\scripts\launch_yamy.bat" "$releaseDir\"
Copy-Item "$root\scripts\launch_yamy_admin.bat" "$releaseDir\"
Copy-Item -Recurse "$root\docs" "$releaseDir\docs"
Copy-Item "$root\docs\readme.txt" "$releaseDir\readme.txt"


# -----------------------------------------------------------------------------
# Automated Dependency Verification
# -----------------------------------------------------------------------------
function Check-Dependencies {
    param (
        [string]$Directory
    )
    Write-Host "Checking dependencies for binaries in $Directory..." -ForegroundColor Cyan
    $binaries = Get-ChildItem -Path $Directory -Include *.exe, *.dll -Recurse
    $failed = $false
    # Temporarily add 64-bit bin to path for objdump
    $env:PATH = "C:\tools\msys64\mingw64\bin;" + $env:PATH

    foreach ($bin in $binaries) {
        $deps = Invoke-Expression "objdump -p '$($bin.FullName)'" | Select-String "DLL Name"
        if ($deps -match "libstdc\+\+|libgcc|libwinpthread") {
            Write-Host "Error: $($bin.Name) has unexpected forbidden dependencies!" -ForegroundColor Red
            $deps | Select-String "libstdc\+\+|libgcc|libwinpthread" | ForEach-Object { Write-Host "  - $_" -ForegroundColor Red }
            $failed = $true
        }
        else {
            Write-Host "  $($bin.Name): OK" -ForegroundColor Gray
        }
    }

    if ($failed) {
        throw "Dependency check failed! Binaries are linking against forbidden MinGW DLLs."
    }
    Write-Host "Dependency check passed." -ForegroundColor Green
}

Check-Dependencies -Directory $releaseDir


# -----------------------------------------------------------------------------
# Create Zip
# -----------------------------------------------------------------------------
Write-Output "Creating Archive..."
$zipPath = "$distDir\yamy-dist-mingw.zip"
Compress-Archive -Path "$releaseDir\*" -DestinationPath $zipPath -Force

# Cleanup release dir safely (keeping zip)
Remove-Item -Recurse -Force $releaseDir

Write-Output "Success! Created $zipPath"
