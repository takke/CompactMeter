@echo off

REM ローカルで実行する場合は CompactMeter.exe を終了する
if "%APPVEYOR_BUILD_NUMBER%" == "" (
    call killall.bat
)


set OUT_DIR=%1
if "%OUT_DIR%" == "" (
	set OUT_DIR=.
)

REM replace '/' with '\'
set OUT_DIR=%OUT_DIR:/=\%

@echo.
@echo ---- Make prebuild.h ----

@echo APPVEYOR_BUILD_NUMBER : %APPVEYOR_BUILD_NUMBER%

REM Output prebuild.h
set OUTPUT_H=%OUT_DIR%\prebuild.h
set OUTPUT_H_TMP=%OUTPUT_H%.tmp

call :output_prebuild > %OUTPUT_H_TMP%

fc %OUTPUT_H% %OUTPUT_H_TMP% 1>nul 2>&1
if not errorlevel 1 (
	del %OUTPUT_H_TMP%
	@echo %OUTPUT_H% was not updated.
) else (
	if exist %OUTPUT_H% del %OUTPUT_H%
	move /y %OUTPUT_H_TMP% %OUTPUT_H%
	@echo %OUTPUT_H% was updated.
)

@echo.

exit /b 0

:output_prebuild
echo /*! @file */
echo #pragma once
if "%APPVEYOR_BUILD_NUMBER%" == "" (
	echo // APPVEYOR_BUILD_NUMBER     is not defined
	echo // APPVEYOR_BUILD_NUMBER_INT is not defined
	echo // APPVEYOR_BUILD_NUMBER_LABEL is not defined
) else (
	echo #define APPVEYOR_BUILD_NUMBER     "%APPVEYOR_BUILD_NUMBER%"
	echo #define APPVEYOR_BUILD_NUMBER_INT  %APPVEYOR_BUILD_NUMBER%
	echo #define APPVEYOR_BUILD_NUMBER_LABEL "Build %APPVEYOR_BUILD_NUMBER%"
)

exit /b 0
