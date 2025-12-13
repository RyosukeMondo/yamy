# YAMY One-Click Test Script
# Downloads latest v1.0.3, extracts, and launches

param(
    [string]$Mode = "debug"  # Options: debug, admin, normal
)

$ErrorActionPreference = "Stop"

# Configuration
$RELEASE_VERSION = "v1.0.3"
$GITHUB_REPO = "RyosukeMondo/yamy"
$DOWNLOAD_URL = "https://github.com/$GITHUB_REPO/releases/download/$RELEASE_VERSION/yamy-dist.zip"
$TEST_DIR = "$env:TEMP\yamy-test"
$ZIP_FILE = "$TEST_DIR\yamy-dist.zip"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "YAMY One-Click Test Script" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Step 1: Clean up existing folder
if (Test-Path $TEST_DIR) {
    Write-Host "[1/4] Removing existing test folder..." -ForegroundColor Yellow
    Remove-Item -Path $TEST_DIR -Recurse -Force
    Write-Host "      Cleaned up: $TEST_DIR" -ForegroundColor Green
} else {
    Write-Host "[1/4] No existing folder to clean" -ForegroundColor Green
}

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

$batPath = Join-Path $TEST_DIR $batFile
if (-not (Test-Path $batPath)) {
    Write-Host "ERROR: $batFile not found in extracted files!" -ForegroundColor Red
    exit 1
}

Write-Host "Launching: $batFile" -ForegroundColor Green
Write-Host "Location:  $TEST_DIR" -ForegroundColor Gray
Write-Host ""
Write-Host "Press Ctrl+C to stop YAMY when done testing." -ForegroundColor Yellow
Write-Host ""

# Launch YAMY
Set-Location $TEST_DIR
Start-Process -FilePath $batPath -Wait

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Test Complete!" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Log file: $env:LOCALAPPDATA\YAMY\yamy.log" -ForegroundColor Gray
Write-Host "Test folder: $TEST_DIR" -ForegroundColor Gray
Write-Host ""
Write-Host "To test again, just re-run this script!" -ForegroundColor Green
