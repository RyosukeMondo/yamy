
# scripts/check_missing_sources.ps1
# Checks if all .cpp files in src/ are referenced in CMakeLists.txt

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Split-Path -Parent $ScriptDir
$CMakeFile = Join-Path $RepoRoot "CMakeLists.txt"
$SrcDir = Join-Path $RepoRoot "src"

Write-Host "Checking for missing sources in CMakeLists.txt..." -ForegroundColor Cyan

# Read CMakeLists.txt content
$cmakeContent = Get-Content $CMakeFile -Raw

# Find all .cpp files effectively
$allCppFiles = Get-ChildItem -Path $SrcDir -Recurse -Filter "*.cpp" | Select-Object -ExpandProperty FullName

$missingFiles = @()

foreach ($file in $allCppFiles) {
    # Get relative path from repo root (e.g., src/platform/windows/registry.cpp)
    $relativePath = $file.Substring($RepoRoot.Length + 1).Replace("\", "/")
    
    # Check if this path exists in CMakeLists.txt
    # We do a simple string check. Ideally we'd parse the cmake file, but this is a heuristic.
    if ($cmakeContent -notmatch [regex]::Escape($relativePath)) {
    # Exclude known non-build files
    # - tests/: Test sources are handled separately or in yamy_test
    # - ts4mayu/: Legacy touchpad support, not currently in CMake build
    if ($relativePath -match "^src/tests/" -or $relativePath -match "^src/ts4mayu/") {
        continue
    }
    }
}

if ($missingFiles.Count -gt 0) {
    Write-Host "Found $($missingFiles.Count) .cpp files missing from CMakeLists.txt:" -ForegroundColor Red
    foreach ($missing in $missingFiles) {
        Write-Host "  - $missing" -ForegroundColor Red
    }
    Write-Host "`nPlease add these files to CMakeLists.txt to avoid linker errors." -ForegroundColor Yellow
    exit 1
} else {
    Write-Host "All .cpp files appear to be included in CMakeLists.txt." -ForegroundColor Green
    exit 0
}
