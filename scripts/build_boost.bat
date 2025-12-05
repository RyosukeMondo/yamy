@echo off
setlocal

echo ==========================================
echo Setting up Visual Studio Environment...
echo ==========================================
if exist "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" (
    call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=amd64
) else (
    echo Warning: VsDevCmd.bat not found. Assuming Developer Command Prompt.
)

cd /d "%~dp0..\..\boost_1_84_0"

echo Building Boost Regex x64...
b2 toolset=msvc address-model=64 link=static runtime-link=static --with-regex release stage
if errorlevel 1 goto error

echo Copying x64 libs...
if not exist "%~dp0..\proj\ext_lib64\Release" mkdir "%~dp0..\proj\ext_lib64\Release"
copy /Y stage\lib\libboost_regex-mt-s-x64-1_84.lib "%~dp0..\proj\ext_lib64\Release\libboost_regex-mt-s-1_84.lib"
if not exist "%~dp0..\proj\ext_lib64\Release\libboost_regex-mt-s-1_84.lib" (
    echo Trying alternative name...
    copy /Y stage\lib\libboost_regex-mt-s-1_84.lib "%~dp0..\proj\ext_lib64\Release\libboost_regex-mt-s-1_84.lib"
)

echo Building Boost Regex x86...
b2 toolset=msvc address-model=32 link=static runtime-link=static --with-regex release stage
if errorlevel 1 goto error

echo Copying x86 libs...
if not exist "%~dp0..\proj\ext_lib32\Release" mkdir "%~dp0..\proj\ext_lib32\Release"
copy /Y stage\lib\libboost_regex-mt-s-x32-1_84.lib "%~dp0..\proj\ext_lib32\Release\libboost_regex-mt-s-1_84.lib"
if not exist "%~dp0..\proj\ext_lib32\Release\libboost_regex-mt-s-1_84.lib" (
    echo Trying alternative name...
    copy /Y stage\lib\libboost_regex-mt-s-1_84.lib "%~dp0..\proj\ext_lib32\Release\libboost_regex-mt-s-1_84.lib"
)

echo Boost build and copy successful.
exit /b 0

:error
echo Boost build failed.
exit /b 1
