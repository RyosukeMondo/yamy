$ErrorActionPreference = "Stop"

$root = "$PSScriptRoot\.."
$distDir = "$root\dist"
$releaseDir = "$root\dist\yamy-mingw"

# -----------------------------------------------------------------------------
# Toolchain Detection
# -----------------------------------------------------------------------------
$msysDir = $null
$possibleMsys = @("C:\tools\msys64", "C:\msys64", "$env:ProgramData\mingw64")
foreach ($path in $possibleMsys) {
    if (Test-Path "$path\mingw64\bin\gcc.exe") {
        $msysDir = $path
        Write-Host "Found MinGW-w64 (64-bit) at: $msysDir" -ForegroundColor Green
        break
    }
}

if (-not $msysDir) {
    throw "Could not find MinGW-w64 64-bit installation (checked C:\tools\msys64, C:\msys64, ProgramData). Please install MSYS2 or MinGW64."
}

# Check for 32-bit toolchain
$mingw32Dir = "$msysDir\mingw32"
$has32Bit = Test-Path "$mingw32Dir\bin\gcc.exe"
if ($has32Bit) {
    Write-Host "Found MinGW-w64 (32-bit) at: $mingw32Dir" -ForegroundColor Green
}
else {
    Write-Host "MinGW-w64 (32-bit) not found at: $mingw32Dir" -ForegroundColor Yellow
    Write-Host "To enable 32-bit binaries, run: $msysDir\usr\bin\pacman.exe -S --noconfirm mingw-w64-i686-toolchain" -ForegroundColor Yellow
}

& "$PSScriptRoot\check_missing_sources.ps1"
if ($LASTEXITCODE -ne 0) { throw "Missing sources check failed" }
& "$PSScriptRoot\check_encoding.ps1"
if ($LASTEXITCODE -ne 0) { throw "Encoding check failed" }

# -----------------------------------------------------------------------------
# Prepare Output Directory
# -----------------------------------------------------------------------------
if (Test-Path $releaseDir) {
    Remove-Item -Recurse -Force $releaseDir
}
New-Item -Path $releaseDir -ItemType Directory -Force | Out-Null


# -----------------------------------------------------------------------------
# Build 64-bit
# -----------------------------------------------------------------------------
Write-Output "Building 64-bit Target..."

# Sanitize PATH to remove msys/usr/bin which might contain sh.exe that breaks MinGW Makefiles
$rawPath = $env:PATH
# Remove msys/usr/bin to avoid sh.exe
$cleanPath = ($rawPath -split ';' | Where-Object { $_ -notlike "*\usr\bin*" }) -join ';'
$env:PATH = "$msysDir\mingw64\bin;C:\Program Files\CMake\bin;C:\Windows\System32\WindowsPowerShell\v1.0;$cleanPath"

Write-Output "PATH for 64-bit build: $env:PATH"

Write-Output "Configuring CMake (64-bit)..."
$logsDir = "$root\logs"
if (-not (Test-Path $logsDir)) { New-Item -Path $logsDir -ItemType Directory -Force | Out-Null }
if (Test-Path "$root\build\mingw64") { Remove-Item -Recurse -Force "$root\build\mingw64" }

# Log file moved to logs directory
cmake -S $root -B "$root\build\mingw64" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON > "$logsDir\configure_64.log" 2>&1
if ($LASTEXITCODE -ne 0) { 
    Get-Content "$logsDir\configure_64.log" | Write-Host -ForegroundColor Red
    throw "CMake Configure (64-bit) failed" 
}

Write-Output "Building Targets (64-bit)..."
cmake --build "$root\build\mingw64" --config Release
if ($LASTEXITCODE -ne 0) { throw "CMake Build (64-bit) failed" }

# -----------------------------------------------------------------------------
# Run Tests
# -----------------------------------------------------------------------------
Write-Output "Running Tests..."
Push-Location "$root\build\mingw64"
try {
    ctest --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw "Tests failed! Packaging aborted." }
}
finally {
    Pop-Location
}

# Copy 64-bit artifacts
Write-Output "Waiting for file locks to release..."
Start-Sleep -Seconds 2
$binDir64 = "$root\build\mingw64\bin"
Copy-Item "$binDir64\yamy.exe" "$releaseDir\" -Force
Copy-Item "$binDir64\yamy64.exe" "$releaseDir\" -Force
Copy-Item "$binDir64\yamy64.dll" "$releaseDir\" -Force
Copy-Item -Recurse "$root\keymaps" "$releaseDir\keymaps"
Copy-Item "$root\scripts\launch_yamy.bat" "$releaseDir\"
Copy-Item "$root\scripts\launch_yamy_admin.bat" "$releaseDir\"

# Copy missing assets requested by user
Copy-Item "$root\src\yamy.ini" "$releaseDir\"
if (Test-Path "$root\resources\workaround.reg") {
    Copy-Item "$root\resources\workaround.reg" "$releaseDir\"
}
if (Test-Path "$root\docs\readme.txt") {
    Copy-Item "$root\docs\readme.txt" "$releaseDir\"
}



# -----------------------------------------------------------------------------
# Build 32-bit
# -----------------------------------------------------------------------------
if ($has32Bit) {
    Write-Output "Building 32-bit Target..."
    
    # Temporarily prepend 32-bit bin to PATH to use correct compiler
    # We use $cleanPath which has usr/bin removed
    $env:PATH = "$mingw32Dir\bin;C:\Program Files\CMake\bin;C:\Windows\System32\WindowsPowerShell\v1.0;$cleanPath"
    
    # Clean up previous build to ensure fresh config
    if (Test-Path "$root\build\mingw32") { Remove-Item -Recurse -Force "$root\build\mingw32" }

    Write-Output "Configuring CMake (32-bit)..."
    # Use standard generator
    $c32 = "$mingw32Dir\bin\gcc.exe"
    $cxx32 = "$mingw32Dir\bin\g++.exe"
    
    cmake -S $root -B "$root\build\mingw32" -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER="$c32" -DCMAKE_CXX_COMPILER="$cxx32"
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
}
else {
    Write-Host "Skipping 32-bit build (Toolchain not found)" -ForegroundColor Gray
}

# -----------------------------------------------------------------------------
# Bundle MinGW runtime DLLs for standalone execution
# -----------------------------------------------------------------------------
$mingwBin = "$msysDir\mingw64\bin"
Copy-Item "$mingwBin\libstdc++-6.dll" "$releaseDir\"
Copy-Item "$mingwBin\libgcc_s_seh-1.dll" "$releaseDir\"
Copy-Item "$mingwBin\libwinpthread-1.dll" "$releaseDir\"

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
        
        # yamy64.exe and yamy64.dll are ALLOWED to depend on MinGW DLLs (we bundled them)
        # yamyd32.exe and yamy32.dll/exe MUST be static to avoid 32-bit DLL conflicts
        if ($bin.Name -match "yamy32\.dll|yamyd32\.exe|yamy32\.exe") {
            if ($deps -match "libstdc\+\+|libgcc|libwinpthread") {
                Write-Host "Error: $($bin.Name) has unexpected forbidden dependencies!" -ForegroundColor Red
                $deps | Select-String "libstdc\+\+|libgcc|libwinpthread" | ForEach-Object { Write-Host "  - $_" -ForegroundColor Red }
                $failed = $true
            }
            else {
                Write-Host "  $($bin.Name): OK (Static)" -ForegroundColor Gray
            }
        }
        else {
            Write-Host "  $($bin.Name): OK (Bundled Runtime)" -ForegroundColor Gray
        }
    }

    if ($failed) {
        throw "Dependency check failed! binaries are not statically linked as required."
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

Remove-Item -Recurse -Force $releaseDir

Write-Output "Success! Created $zipPath"
