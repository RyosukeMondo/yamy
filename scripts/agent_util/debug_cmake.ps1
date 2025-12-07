$ErrorActionPreference = "Stop"
try {
    Write-Host "Running CMake manual configuration..."
    cmake -G "Visual Studio 17 2022" -A x64 -S . -B build_manual > cmake_full_log.txt 2>&1
    Write-Host "CMake configuration completed. Log saved to cmake_full_log.txt"
} catch {
    Write-Error "CMake configuration failed. Check cmake_full_log.txt for details."
    exit 1
}
