<#
.SYNOPSIS
    UAT Helper Script for YAMY

.DESCRIPTION
    1. Unzips the latest build package to UAT\yamy_{Time}
    2. Shuts down running yamy instances
    3. Launches the new build
    4. Waits for user input
    5. Restores the daily production instance

.PARAMETER GetLastLog
    If specified, dumps the content of the most recent UAT log file and exits.

.EXAMPLE
    .\UAT.ps1
    .\UAT.ps1 -GetLastLog
#>

param(
    [switch]$GetLastLog
)

$ErrorActionPreference = "Stop"
$RepoRoot = Resolve-Path "$PSScriptRoot/../../.."
$UATBaseDir = Join-Path $RepoRoot "UAT"

if ($GetLastLog) {
    if (-not (Test-Path $UATBaseDir)) {
        Write-Error "UAT directory not found at $UATBaseDir"
    }
    
    # Find latest UAT directory
    $LatestUAT = Get-ChildItem -Path $UATBaseDir | Where-Object { $_.PSIsContainer -and $_.Name -like "yamy_*" } | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    
    if (-not $LatestUAT) {
        Write-Error "No UAT runs found in $UATBaseDir"
    }
    
    $LogFile = Join-Path $LatestUAT.FullName "logs/yamy.log"
    if (-not (Test-Path $LogFile)) {
        Write-Warning "Log file not found at: $LogFile"
        
        # Try finding any .log file
        $LogFile = Get-ChildItem -Path $LatestUAT.FullName -Filter "*.log" -Recurse | Select-Object -First 1 -ExpandProperty FullName
    }
    
    if ($LogFile -and (Test-Path $LogFile)) {
        Write-Host "--- LOG START: $LogFile ---" -ForegroundColor Cyan
        Get-Content $LogFile
        Write-Host "--- LOG END ---" -ForegroundColor Cyan
    }
    else {
        Write-Error "No logs found for run: $($LatestUAT.Name)"
    }
    exit 0
}

# 1. Find Latest Package
$BuildDir = Join-Path $RepoRoot "build/windows-msvc-release"
$PackageFilter = "yamy-*-Windows-AMD64.zip"
$LatestPackage = Get-ChildItem -Path $BuildDir -Filter $PackageFilter | Sort-Object LastWriteTime -Descending | Select-Object -First 1

if (-not $LatestPackage) {
    Write-Error "No package found in $BuildDir matching $PackageFilter"
}

Write-Host "Found package: $($LatestPackage.Name)" -ForegroundColor Cyan

# 2. Prepare UAT Directory
$Timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$UATBaseDir = Join-Path $RepoRoot "UAT"
$UATDir = Join-Path $UATBaseDir "yamy_$Timestamp"

Write-Host "Extracting to: $UATDir" -ForegroundColor Cyan
New-Item -ItemType Directory -Force -Path $UATDir | Out-Null
Expand-Archive -Path $LatestPackage.FullName -DestinationPath $UATDir -Force

# 3. Stop Existing Instances
function Stop-Yamy {
    Write-Host "Stopping existing yamy instances..." -ForegroundColor Yellow
    $Targets = @("yamy", "yamy32", "yamy64", "yamyd")
    foreach ($t in $Targets) {
        Get-Process -Name $t -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
    }
}

Stop-Yamy

# 4. Launch UAT Instance
$ExePath = Join-Path $UATDir "yamy64.exe"
if (-not (Test-Path $ExePath)) {
    # Fallback if zip structure is different (e.g. inside a folder)
    $ExePath = Get-ChildItem -Path $UATDir -Filter "yamy64.exe" -Recurse | Select-Object -First 1 -ExpandProperty FullName
}

if (-not (Test-Path $ExePath)) {
    Write-Error "Could not find yamy64.exe in extracted package."
}

# 2b. Create logs directory for UAT logging
# Create it relative to the EXE so main_qt.cpp finds it
$ExeDir = Split-Path $ExePath -Parent
$LogsDir = Join-Path $ExeDir "logs"
New-Item -ItemType Directory -Force -Path $LogsDir | Out-Null
Write-Host "Created logs directory for debugging: $LogsDir" -ForegroundColor Cyan


# 4b. Deploy Qt Dependencies (Ensure DLLs are present)
$QtBin = "C:\Qt\5.15.2\msvc2019_64\bin"
$Windeployqt = Join-Path $QtBin "windeployqt.exe"

if (Test-Path $Windeployqt) {
    Write-Host "Running windeployqt to ensure dependencies..." -ForegroundColor Cyan
    $DeployArgs = @(
        "--no-translations",
        "--no-system-d3d-compiler",
        "--compiler-runtime",
        $ExePath
    )
    Start-Process -FilePath $Windeployqt -ArgumentList $DeployArgs -NoNewWindow -Wait
}
else {
    Write-Warning "windeployqt.exe not found at $Windeployqt. DLL errors may occur."
}

Write-Host "Launching UAT Instance: $ExePath" -ForegroundColor Green
Start-Process -FilePath $ExePath -Verb RunAs

# 5. Wait for User
Write-Host "`n=======================================================" -ForegroundColor White
Write-Host " UAT Instance is running." -ForegroundColor White
Write-Host " Test your changes." -ForegroundColor White
Write-Host " Press ENTER to stop UAT instance and restore Production." -ForegroundColor White
Write-Host "=======================================================" -ForegroundColor White
Read-Host

# 6. Cleanup and Restore
Write-Host "Stopping UAT instance..." -ForegroundColor Yellow
Stop-Yamy

$RestoreScript = "C:\Users\ryosu\OneDrive\PortableApp\yamy-0.5\launch_yamy_admin.bat"
if (Test-Path $RestoreScript) {
    Write-Host "Restoring Production Instance..." -ForegroundColor Green
    Start-Process -FilePath $RestoreScript -Verb RunAs
}
else {
    Write-Warning "Restore script not found at: $RestoreScript"
}

Write-Host "Done." -ForegroundColor Cyan
