@echo off

set BASE_DIR=%~dp0

REM --------------------------------------------------
REM 各ビルド環境用の環境変数設定
REM --------------------------------------------------
REM https://www.appveyor.com/docs/environment-variables/
if "%APPVEYOR%"=="True" (
    set ZIP_CMD=7z
    REM ARTIFACT_VERSION は appveyor.yml の before_build: で設定済
) else (
    set ZIP_CMD="C:\Program Files\7-Zip\7z"
    set ARTIFACT_VERSION=X_X_X
)

REM --------------------------------------------------
REM 作業ディレクトリの準備
REM --------------------------------------------------
set WORK_DIR=%BASE_DIR%packaging
rmdir /S /Q %WORK_DIR%
mkdir %WORK_DIR%
if %errorlevel% neq 0 (echo error && exit /b 1)

REM --------------------------------------------------
REM ファイル収集
REM --------------------------------------------------
echo.
echo collect files
echo.
copy /Y /B Release\CompactMeter.exe        %WORK_DIR%
copy /Y /B Release\register.exe            %WORK_DIR%
copy /Y /B x64\Release\CompactMeter_64.exe %WORK_DIR%
copy /Y /B README.md                       %WORK_DIR%

REM --------------------------------------------------
REM コード署名
REM --------------------------------------------------
if "%APPVEYOR%"=="True" (
    echo.
    echo skip sign
    echo.
) else (
    echo.
    echo sign
    echo.
    set SIGNTOOL="C:\Program Files (x86)\Windows Kits\10\bin\10.0.18362.0\x64\signtool"
    %SIGNTOOL% sign /a /fd SHA256 /v /tr http://timestamp.comodoca.com/?td=sha256 /td sha256 ^
        %WORK_DIR%\CompactMeter.exe ^
        %WORK_DIR%\CompactMeter_64.exe ^
        %WORK_DIR%\register.exe
)

REM --------------------------------------------------
REM ZIP
REM --------------------------------------------------
echo.
echo ZIP
echo.
cd %WORK_DIR%
%ZIP_CMD% a CompactMeter_v%ARTIFACT_VERSION%.zip CompactMeter.exe CompactMeter_64.exe register.exe README.md
cd ..
