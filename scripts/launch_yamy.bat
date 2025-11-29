@echo off
cd /d "%~dp0"

echo ==========================================
echo [Kill] Terminating existing Yamy processes...
echo ==========================================
taskkill /F /IM yamy.exe >nul 2>&1
taskkill /F /IM yamy64.exe >nul 2>&1
taskkill /F /IM yamy32.exe >nul 2>&1
taskkill /F /IM yamyd32.exe >nul 2>&1

REM Wait a moment for the OS to release file locks
echo [Wait] Waiting for cleanup...
timeout /t 2 /nobreak >nul

echo ==========================================
echo [Start] Launching Yamy...
echo ==========================================

if exist "yamy64.exe" (
    start "" "yamy64.exe"
    exit /b 0
)
if exist "yamy.exe" (
    start "" "yamy.exe"
    exit /b 0
)

echo Error: Could not find yamy.exe or yamy64.exe.
pause
