@echo off
REM Builds fh2_ngx.dll with MSVC. Run from an "x64 Native Tools Command Prompt for VS".
REM Needs NVIDIA's DLSS SDK (https://github.com/NVIDIA/DLSS), cloned to ..\third_party\DLSS
REM or pointed to with:  set DLSS_SDK=C:\path\to\DLSS
setlocal
cd /d "%~dp0"
if "%DLSS_SDK%"=="" set DLSS_SDK=%~dp0..\third_party\DLSS
if not exist "%DLSS_SDK%\include\nvsdk_ngx.h" (
    echo DLSS SDK not found at "%DLSS_SDK%".
    echo   git clone --depth 1 https://github.com/NVIDIA/DLSS "%~dp0..\third_party\DLSS"
    exit /b 1
)
set LIBDIR=%DLSS_SDK%\lib\Windows_x86_64\x64
REM _s = static CRT (/MT, no runtime dependency). Falls back to the /MD build if that is all there is.
set CRT=/MT
set NGXLIB=nvsdk_ngx_s.lib
if not exist "%LIBDIR%\%NGXLIB%" (
    set CRT=/MD
    set NGXLIB=nvsdk_ngx_d.lib
)
if not exist "%~dp0..\build" mkdir "%~dp0..\build"
cl /nologo /O2 /EHsc %CRT% /LD /I"%DLSS_SDK%\include" ngx_bridge.cpp ^
   /Fo"%~dp0..\build\\" /Fe"%~dp0..\build\fh2_ngx.dll" ^
   /link /LIBPATH:"%LIBDIR%" %NGXLIB% d3d12.lib user32.lib advapi32.lib
if errorlevel 1 exit /b 1
echo built build\fh2_ngx.dll
