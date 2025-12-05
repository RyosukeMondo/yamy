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
timeout /t 1 /nobreak >nul

echo ==========================================
echo Cleaning Release folder...
echo ==========================================
if exist Release (
    rmdir /S /Q Release
)
mkdir Release

echo ==========================================
echo Cleaning Intermediate folders...
echo ==========================================
if exist proj\Release rmdir /S /Q proj\Release
if exist proj\x64 rmdir /S /Q proj\x64
if exist proj\yamy64\x64 rmdir /S /Q proj\yamy64\x64
if exist proj\yamy64dll\x64 rmdir /S /Q proj\yamy64dll\x64

echo ==========================================
echo Copying Boost Libraries...
echo ==========================================
if not exist "proj\ext_lib64\Release" mkdir "proj\ext_lib64\Release"
if not exist "proj\ext_lib32\Release" mkdir "proj\ext_lib32\Release"
copy /Y "..\boost_1_84_0\libboost_regex-mt-s-1_84.lib" "proj\ext_lib64\Release\libboost_regex-mt-s-1_84.lib"
copy /Y "..\boost_1_84_0\libboost_regex-mt-s-1_84.lib" "proj\ext_lib32\Release\libboost_regex-mt-s-1_84.lib"

echo ==========================================
echo Setting up Visual Studio Environment...
echo ==========================================
if exist "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" (
    call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=amd64
) else (
    echo Warning: VsDevCmd.bat not found. Assuming Developer Command Prompt.
)

echo ==========================================
echo Building Yamy (x64 Release)...
echo ==========================================
msbuild proj\yamy.sln /p:Configuration=Release /p:Platform=x64 /t:Build
if errorlevel 1 goto error

echo ==========================================
echo Building Yamy (Win32 Release)...
echo ==========================================
msbuild proj\yamy.sln /p:Configuration=Release /p:Platform=Win32 /t:Build
if errorlevel 1 goto error

echo ==========================================
echo Finalizing (Renaming executables)...
echo ==========================================
pushd Release
if exist yamy64 move /Y yamy64 yamy64.exe
if exist yamyd32 move /Y yamyd32 yamyd32.exe
if exist yamy32 move /Y yamy32 yamy32.exe
popd

echo ==========================================
echo Packaging (Copying keymaps and scripts)...
echo ==========================================
echo Copying keymaps...
xcopy /E /I /Y keymaps Release\keymaps >nul
echo Copying scripts...
xcopy /E /I /Y scripts Release\scripts >nul
if exist Release\scripts\build_yamy.bat del Release\scripts\build_yamy.bat

echo Copying launch scripts to root...
copy /Y scripts\launch_yamy.bat Release\launch_yamy.bat >nul
copy /Y scripts\launch_yamy_admin.bat Release\launch_yamy_admin.bat >nul
copy /Y docs\readme.txt Release\readme.txt >nul

echo ==========================================
echo Cleaning up debug symbols...
echo ==========================================
if exist Release\*.pdb del Release\*.pdb

echo ==========================================
echo Creating Zip Archive...
echo ==========================================
if not exist dist mkdir dist
if exist dist\yamy-dist.zip del dist\yamy-dist.zip
powershell -command "Compress-Archive -Path Release\* -DestinationPath dist\yamy-dist.zip -Force"

echo ==========================================
echo Build Successful!
echo Created dist\yamy-dist.zip
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
