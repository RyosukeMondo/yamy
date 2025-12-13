@echo off
REM YAMY One-Click Test Script (Windows Batch Wrapper)
REM This runs the PowerShell script in debug mode

echo ========================================
echo YAMY One-Click Test Script
echo ========================================
echo.
echo This will:
echo   1. Download YAMY v1.0.3
echo   2. Extract to %TEMP%\yamy-test
echo   3. Launch in DEBUG mode
echo.
echo Press Ctrl+C to cancel, or
pause

REM Run PowerShell script
powershell -ExecutionPolicy Bypass -File "%~dp0test-yamy.ps1" -Mode debug

pause
