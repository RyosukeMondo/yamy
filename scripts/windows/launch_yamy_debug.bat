@echo off
REM Launch YAMY in debug mode with console window and verbose logging
REM
REM This will:
REM - Show a console window with real-time logs
REM - Save logs to %LOCALAPPDATA%\YAMY\yamy.log
REM - Display detailed startup information

echo Starting YAMY in DEBUG mode...
echo.
echo Console window will show real-time logs.
echo Log file: %LOCALAPPDATA%\YAMY\yamy.log
echo.
echo Press Ctrl+C in this window to stop YAMY
echo.

REM Start YAMY with debug flag
start "YAMY Debug" /WAIT yamy.exe --debug

pause
