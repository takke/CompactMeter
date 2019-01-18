@echo off

REM --------------------------------------------------
REM 各ビルド環境用の環境変数設定
REM --------------------------------------------------
REM https://www.appveyor.com/docs/environment-variables/
if "%APPVEYOR%"=="True" (
    set EXTRA_CMD=/verbosity:minimal /logger:"%ProgramFiles%\AppVeyor\BuildAgent\Appveyor.MSBuildLogger.dll"
    set MSBUILD_PATH=msbuild
    set SLN_PATH="%APPVEYOR_BUILD_FOLDER%\CompactMeter.sln"
) else (
    set EXTRA_CMD=
    set MSBUILD_PATH="%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\MSBuild.exe"
    set SLN_PATH=CompactMeter.sln
)

REM --------------------------------------------------
REM 32bit build
REM --------------------------------------------------
set CMD=%MSBUILD_PATH% %SLN_PATH% /p:Platform=x86 /p:Configuration=Release /t:"Build" %EXTRA_CMD%
echo.
echo %CMD%
echo.
%CMD%

REM --------------------------------------------------
REM 64bit build
REM --------------------------------------------------
set CMD=%MSBUILD_PATH% %SLN_PATH% /p:Platform=x64 /p:Configuration=Release /t:"Build" %EXTRA_CMD%
echo.
echo %CMD%
echo.
%CMD%
