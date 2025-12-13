<#
.SYNOPSIS
    Unified build script for yamy.
    Wraps standard CMake commands using presets.

.DESCRIPTION
    Builds, tests, and packages yamy for different compilers and configurations.
    Uses CMakePresets.json definitions.

.PARAMETER Config
    Build configuration: "Release" (default) or "Debug".

.PARAMETER Compiler
    Target compiler: "MSVC" (default) or "MinGW".

.PARAMETER Clean
    Clean the build directory before building.

.PARAMETER Package
    Run CPack to generate distribution package after build.

.PARAMETER Install
    Run CMake install to the 'dist' directory after build.

.EXAMPLE
    .\build.ps1 -Config Debug
    Builds with MSVC in Debug configuration.

.EXAMPLE
    .\build.ps1 -Compiler MinGW -Package
    Builds with MinGW in Release configuration and creates a package.
#>
param(
    [ValidateSet("Release", "Debug")]
    [string]$Config = "Release",

    [ValidateSet("MSVC", "MinGW")]
    [string]$Compiler = "MSVC",

    [ValidateSet("x64", "x86")]
    [string]$Arch = "x64",

    [switch]$Install,
    [switch]$Package,
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
# Ensure we are in the project root
Set-Location "$PSScriptRoot/../.."
$presetName = "windows-$($Compiler.ToLower())-$($Config.ToLower())"
if ($Arch -eq "x86") {
    $presetName += "-x86"
}

Write-Host "==============================================" -ForegroundColor Cyan
Write-Host "  YAMY Build System" -ForegroundColor Cyan
Write-Host "  Compiler:      $Compiler"
Write-Host "  Configuration: $Config"
Write-Host "  Preset:        $presetName"
Write-Host "==============================================" -ForegroundColor Cyan

# 1. Clean
if ($Clean) {
    $buildDir = "$PSScriptRoot/../../build/$presetName"
    if (Test-Path $buildDir) {
        Write-Host "Cleaning build directory: $buildDir" -ForegroundColor Yellow
        Remove-Item -Recurse -Force $buildDir
    }
}

# 2. Configure
Write-Host "`n[1/3] Configuring..." -ForegroundColor Green
try {
    cmake --preset $presetName
}
catch {
    Write-Error "Configuration failed."
}

# 3. Build
Write-Host "`n[2/3] Building..." -ForegroundColor Green
try {
    cmake --build --preset $presetName
}
catch {
    Write-Error "Build failed."
}

# 4. Install (Optional)
if ($Install) {
    Write-Host "`n[+] Installing to dist..." -ForegroundColor Green
    cmake --install "build/$presetName" --prefix "dist/$presetName"
}

# 5. Package (Optional)
if ($Package) {
    Write-Host "`n[3/3] Packaging..." -ForegroundColor Green
    try {
        cpack --preset $presetName
    }
    catch {
        Write-Error "Packaging failed."
    }
}

Write-Host "`nBuild Complete!" -ForegroundColor Cyan
