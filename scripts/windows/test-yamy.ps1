# YAMY One-Click Test Script
# Downloads latest v1.0.3, extracts, and launches

param(
    [string]$Mode = "debug"  # Options: debug, admin, normal
)

$ErrorActionPreference = "Stop"

# Configuration
$RELEASE_VERSION = "v1.0.3"
$GITHUB_REPO = "RyosukeMondo/yamy"
$DOWNLOAD_URL = "https://github.com/$GITHUB_REPO/releases/download/$RELEASE_VERSION/yamy-v1.0.3-complete.zip"
$BASE_TEST_DIR = "$env:TEMP\yamy-test"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "YAMY One-Click Test Script" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Step 1: Find next available test folder (auto-rolling)
Write-Host "[1/4] Finding next available test folder..." -ForegroundColor Yellow
$folderNumber = 1
while (Test-Path "$BASE_TEST_DIR-$folderNumber") {
    $folderNumber++
}
$TEST_DIR = "$BASE_TEST_DIR-$folderNumber"
$ZIP_FILE = "$TEST_DIR\yamy-v1.0.3-complete.zip"
Write-Host "      Using: $TEST_DIR" -ForegroundColor Green

# Step 2: Create test directory
Write-Host "[2/4] Creating test directory..." -ForegroundColor Yellow
New-Item -ItemType Directory -Path $TEST_DIR -Force | Out-Null
Write-Host "      Created: $TEST_DIR" -ForegroundColor Green

# Step 3: Download release
Write-Host "[3/4] Downloading YAMY $RELEASE_VERSION..." -ForegroundColor Yellow
Write-Host "      URL: $DOWNLOAD_URL" -ForegroundColor Gray
try {
    Invoke-WebRequest -Uri $DOWNLOAD_URL -OutFile $ZIP_FILE -UseBasicParsing
    $fileSize = (Get-Item $ZIP_FILE).Length / 1MB
    Write-Host "      Downloaded: $([math]::Round($fileSize, 2)) MB" -ForegroundColor Green
} catch {
    Write-Host "      ERROR: Failed to download!" -ForegroundColor Red
    Write-Host "      $_" -ForegroundColor Red
    exit 1
}

# Step 4: Extract zip
Write-Host "[4/4] Extracting archive..." -ForegroundColor Yellow
try {
    Expand-Archive -Path $ZIP_FILE -DestinationPath $TEST_DIR -Force
    Remove-Item $ZIP_FILE
    Write-Host "      Extracted successfully" -ForegroundColor Green
} catch {
    Write-Host "      ERROR: Failed to extract!" -ForegroundColor Red
    Write-Host "      $_" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Ready to Launch!" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Determine which .bat to launch
$batFile = switch ($Mode.ToLower()) {
    "debug"  { "launch_yamy_debug.bat" }
    "admin"  { "launch_yamy_admin.bat" }
    "normal" { "launch_yamy.bat" }
    default  { "launch_yamy_debug.bat" }
}

# Look for bat file in extracted yamy-v1.0.3 subfolder
$extractedFolder = Join-Path $TEST_DIR "yamy-v1.0.3"
$batPath = Join-Path $extractedFolder $batFile

if (-not (Test-Path $batPath)) {
    Write-Host "ERROR: $batFile not found in extracted files!" -ForegroundColor Red
    Write-Host "       Expected: $batPath" -ForegroundColor Red
    Write-Host "       Checking extracted contents..." -ForegroundColor Yellow
    if (Test-Path $extractedFolder) {
        Get-ChildItem $extractedFolder | ForEach-Object { Write-Host "       - $($_.Name)" -ForegroundColor Gray }
    } else {
        Write-Host "       ERROR: Extracted folder not found: $extractedFolder" -ForegroundColor Red
    }
    exit 1
}

Write-Host "Launching: $batFile" -ForegroundColor Green
Write-Host "Location:  $extractedFolder" -ForegroundColor Gray
Write-Host ""
Write-Host "Press Ctrl+C to stop YAMY when done testing." -ForegroundColor Yellow
Write-Host ""

# Launch YAMY from the extracted folder
Set-Location $extractedFolder
Start-Process -FilePath $batPath -NoNewWindow -Wait

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Test Complete!" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Log file: $env:LOCALAPPDATA\YAMY\yamy.log" -ForegroundColor Gray
Write-Host "Test folder: $extractedFolder" -ForegroundColor Gray
Write-Host ""
Write-Host "Previous test folders are preserved for comparison." -ForegroundColor Cyan
Write-Host "To clean up old tests: Remove-Item '$BASE_TEST_DIR-*' -Recurse -Force" -ForegroundColor Gray
