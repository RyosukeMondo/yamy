<#
.SYNOPSIS
    Launcher for yamy.

.DESCRIPTION
    Launches the built yamy.exe from the dist or build folder.
    Falls back to MSVC Release build by default.

.PARAMETER Config
    Configuration to launch: "Release" (default) or "Debug".
    
.PARAMETER Compiler
    Compiler to launch: "MSVC" (default) or "MinGW".

.PARAMETER Admin
    Launch as Administrator.
#>
param(
    [ValidateSet("Release", "Debug")]
    [string]$Config = "Release",

    [ValidateSet("MSVC", "MinGW")]
    [string]$Compiler = "MSVC",

    [switch]$Admin
)

$presetName = "windows-$($Compiler.ToLower())-$($Config.ToLower())"
$exePath = "$PSScriptRoot/../../dist/$presetName/bin/yamy.exe"

# Fallback to build dir if dist doesn't exist
if (-not (Test-Path $exePath)) {
    $exePath = "$PSScriptRoot/../../build/$presetName/bin/$Config/yamy.exe"
}

if (-not (Test-Path $exePath)) {
    Write-Error "Executable not found at: $exePath`nPlease build first: .\build.ps1 -Config $Config -Compiler $Compiler -Install"
}

Write-Host "Launching: $exePath" -ForegroundColor Cyan

if ($Admin) {
    Start-Process -Verb RunAs -FilePath $exePath
}
else {
    Start-Process -FilePath $exePath
}
