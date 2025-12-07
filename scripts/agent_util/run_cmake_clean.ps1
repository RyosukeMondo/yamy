param(
    [string]$BuildDir = "build_clean",
    [string]$LogFile = "build_clean_log.txt"
)

$ErrorActionPreference = "Continue"

Write-Host "Cleaning $BuildDir..."
if (Test-Path $BuildDir) {
    Remove-Item -Recurse -Force $BuildDir
}

Write-Host "Running CMake..."
# Capture stdout and stderr
cmd /c "cmake -G ""Visual Studio 17 2022"" -A x64 -S . -B $BuildDir > $LogFile 2>&1"

if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake failed. See $LogFile for details."
} else {
    Write-Host "CMake finished successfully."
}
