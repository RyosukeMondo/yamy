<#
.SYNOPSIS
    Runs all QA check scripts for yamy.

.DESCRIPTION
    Executes source check, header guard check, and encoding check.
    These checks also run automatically during the CMake build.
#>

$ErrorActionPreference = "Stop"
$ScriptDir = $PSScriptRoot

Write-Host "Running QA Checks..." -ForegroundColor Cyan

# 1. Check Missing Sources
Write-Host "`n[1/3] Checking Missing Sources..." -ForegroundColor Yellow
& "$ScriptDir/qa/check_missing_sources.ps1"

# 2. Check Header Guards
Write-Host "`n[2/3] Checking Header Guards..." -ForegroundColor Yellow
& "$ScriptDir/qa/check_header_guards.ps1"

# 3. Check Encoding
Write-Host "`n[3/3] Checking Encoding (UTF-8 BOM)..." -ForegroundColor Yellow
& "$ScriptDir/qa/check_encoding.ps1"

# 4. Check Antipatterns
Write-Host "`n[4/4] Checking Antipatterns..." -ForegroundColor Yellow
& "$ScriptDir/qa/check_antipatterns.ps1"

Write-Host "`nAll QA Checks Passed!" -ForegroundColor Green
