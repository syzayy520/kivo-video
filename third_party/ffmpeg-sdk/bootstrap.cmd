@echo off
rem ============================================================================
rem  FFmpeg linkable-SDK bootstrap  (no administrator rights required)
rem
rem  Why this exists:
rem    The prebuilt FFmpeg shared SDK ships import libraries (*.lib) whose DACL
rem    may deny read access to the current account. Headers and DLLs stay
rem    readable, only *.lib is locked. That makes every FFmpeg link fail with
rem    LNK1104, even though nothing is missing from the SDK itself.
rem
rem  What this does:
rem    1. Links include\ and bin\ from the source SDK (junctions, no admin).
rem    2. Regenerates avcodec/avformat/avutil/swscale/swresample import libs
rem       from the *readable* DLL export tables, into lib\ of this folder.
rem    Result is a self-contained SDK root that CMake can use via FFMPEG_ROOT.
rem
rem  Usage:
rem    bootstrap.cmd "C:\path\to\ffmpeg-shared-sdk"
rem ============================================================================
setlocal EnableExtensions EnableDelayedExpansion

set "SRC=%~1"
if "%SRC%"=="" goto :usage
set "DST=%~dp0"
if "%DST:~-1%"=="\" set "DST=%DST:~0,-1%"

if not exist "%SRC%\include\libavcodec\avcodec.h" (
  echo [FAIL] Not an FFmpeg shared SDK: "%SRC%"
  echo        Missing include\libavcodec\avcodec.h
  exit /b 1
)
if not exist "%SRC%\bin" (
  echo [FAIL] Missing "%SRC%\bin"
  exit /b 1
)

rem ---- 1. junctions for include and bin -----------------------------------
for %%D in (include bin) do (
  if exist "%DST%\%%D" (
    echo [ skip ] %DST%\%%D already present
  ) else (
    cmd /c mklink /J "%DST%\%%D" "%SRC%\%%D" >nul 2>&1
    if exist "%DST%\%%D" (echo [ link ] %DST%\%%D) else (echo [FAIL] junction %%D & exit /b 1)
  )
)
if not exist "%DST%\lib" mkdir "%DST%\lib"

rem ---- 2. locate MSVC dumpbin + lib ---------------------------------------
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
  echo [FAIL] vswhere.exe not found. Visual Studio with C++ tools is required.
  exit /b 1
)
for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%I"
if "%VSPATH%"=="" (
  echo [FAIL] No Visual Studio C++ toolset installation detected.
  exit /b 1
)

set "TOOLS="
for /d %%V in ("%VSPATH%\VC\Tools\MSVC\*") do (
  if exist "%%V\bin\Hostx64\x64\lib.exe" set "TOOLS=%%V\bin\Hostx64\x64"
)
if "%TOOLS%"=="" (
  echo [FAIL] lib.exe / dumpbin.exe not found under "%VSPATH%\VC\Tools\MSVC"
  exit /b 1
)
echo [ tool ] %TOOLS%

rem ---- 3. regenerate import libs from DLL export tables -------------------
set "PAIRS=avcodec-61:avcodec avformat-61:avformat avutil-59:avutil swscale-8:swscale swresample-5:swresample"
for %%P in (%PAIRS%) do (
  for /f "tokens=1,2 delims=:" %%A in ("%%P") do (
    set "DLLNAME=%%A"
    set "BASE=%%B"
    if not exist "%DST%\bin\!DLLNAME!.dll" (
      echo [FAIL] missing DLL !DLLNAME!.dll
      exit /b 1
    )
    rem header line, then one symbol per line
    > "%DST%\lib\!BASE!.def" echo LIBRARY !DLLNAME!.dll
    >> "%DST%\lib\!BASE!.def" echo EXPORTS
    "%TOOLS%\dumpbin.exe" -nologo -exports "%DST%\bin\!DLLNAME!.dll" > "%TEMP%\kivo_exports_!BASE!.txt" 2>nul
    for /f "usebackq tokens=4" %%S in (`findstr /r /c:"^ *[0-9][0-9]*  *[0-9A-F][0-9A-F]*  *[0-9A-F][0-9A-F]*  *[A-Za-z_]" "%TEMP%\kivo_exports_!BASE!.txt"`) do >> "%DST%\lib\!BASE!.def" echo %%S
    "%TOOLS%\lib.exe" /nologo /def:"%DST%\lib\!BASE!.def" /machine:x64 /out:"%DST%\lib\!BASE!.lib" > "%TEMP%\kivo_lib_!BASE!.txt" 2>&1
    if exist "%DST%\lib\!BASE!.lib" (
      echo [  ok  ] !BASE!.lib
    ) else (
      echo [FAIL] !BASE!.lib & type "%TEMP%\kivo_lib_!BASE!.txt" & exit /b 1
    )
    del /q "%TEMP%\kivo_exports_!BASE!.txt" "%TEMP%\kivo_lib_!BASE!.txt" 2>nul
  )
)

echo.
echo Done. Use this folder as FFMPEG_ROOT:
echo   %DST%
echo Put "%DST%\bin" on PATH when running FFmpeg-linked binaries.
exit /b 0

:usage
echo Usage: bootstrap.cmd "C:\path\to\ffmpeg-shared-sdk"
echo   e.g. bootstrap.cmd "C:\ffmpeg-sdk\ffmpeg-n7.1-latest-win64-gpl-shared-7.1"
exit /b 1
