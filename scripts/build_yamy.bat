@echo off
setlocal
cd /d "%~dp0.."

echo ==========================================
echo Killing Yamy processes...
echo ==========================================
taskkill /F /IM yamy.exe 2>nul
taskkill /F /IM yamy64.exe 2>nul
taskkill /F /IM yamy32.exe 2>nul
taskkill /F /IM yamyd32.exe 2>nul
REM Wait a bit for processes to release locks
timeout /t 1 /nobreak >nul

echo ==========================================
echo Rotating locked DLLs (if any)...
echo ==========================================
pushd Release
if exist yamy64.dll ren yamy64.dll yamy64.dll.%RANDOM%.old
if exist yamy32.dll ren yamy32.dll yamy32.dll.%RANDOM%.old
popd

echo ==========================================
echo Setting up Visual Studio Environment...
echo ==========================================

REM Try to find VS 2026 (v18) or fallback to others if needed.
if exist "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" (
    call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=amd64
) else (
    echo Warning: VsDevCmd.bat not found at expected location.
    echo Assuming you are running from a Developer Command Prompt.
)

echo.
echo ==========================================
echo Building Yamy (x64 Release)...
echo ==========================================
msbuild proj\yamy.sln /p:Configuration=Release /p:Platform=x64
if errorlevel 1 goto error

echo.
echo ==========================================
echo Building Yamy (Win32 Release) for yamyd32...
echo ==========================================
msbuild proj\yamy.sln /p:Configuration=Release /p:Platform=Win32
if errorlevel 1 goto error

echo.
echo ==========================================
echo Finalizing (Renaming executables)...
echo ==========================================
pushd Release

REM Fix extensionless outputs if they exist
if exist yamy64 (
    echo Renaming yamy64 to yamy64.exe...
    move /Y yamy64 yamy64.exe
)
if exist yamyd32 (
    echo Renaming yamyd32 to yamyd32.exe...
    move /Y yamyd32 yamyd32.exe
)
if exist yamy32 (
    echo Renaming yamy32 to yamy32.exe...
    move /Y yamy32 yamy32.exe
)

popd

echo.
echo ==========================================
echo Copying Launch Scripts to Release...
echo ==========================================
copy /Y scripts\launch_yamy.bat Release\launch_yamy.bat >nul
copy /Y scripts\launch_yamy_admin.bat Release\launch_yamy_admin.bat >nul

echo.
echo ==========================================
echo Build Successful!
echo Executables are in the 'Release' folder.
echo ==========================================
endlocal
exit /b 0

:error
echo.
echo ==========================================
echo Build FAILED.
echo ==========================================
endlocal
exit /b 1
