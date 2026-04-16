@echo off
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "WORKSPACE_ROOT=%SCRIPT_DIR%.."

:: --------------------------------------------------
:: Default Values
:: --------------------------------------------------
set "DEFAULT_BOARD=damiao_h7"
set "DEFAULT_TYPE=Debug"
set "BOARD_1=damiao_h7"
set "BOARD_2=dji_c"
set "TYPE_1=Debug"
set "TYPE_2=Release"

:: --------------------------------------------------
:: Command-line Argument Mode
:: If arguments are supplied, skip interactive prompts.
:: Usage: build.bat [board] [type]
::   e.g. build.bat damiao_h7 Debug
::        build.bat dji_c Release
:: --------------------------------------------------
if not "%~1"=="" (
    set "SELECTED_BOARD=%~1"
    if not "%~2"=="" (
        set "SELECTED_BUILD_TYPE=%~2"
    ) else (
        set "SELECTED_BUILD_TYPE=%DEFAULT_TYPE%"
    )
    goto :build
)

:: --------------------------------------------------
:: Interactive Mode – Select Board
:: --------------------------------------------------
echo Select Board:
echo   1. %BOARD_1%
echo   2. %BOARD_2%
echo.

set "SELECTION="
set /p "SELECTION=Enter choice [1-2] (Default=%DEFAULT_BOARD%): "

if "%SELECTION%"=="1" (
    set "SELECTED_BOARD=%BOARD_1%"
) else if "%SELECTION%"=="2" (
    set "SELECTED_BOARD=%BOARD_2%"
) else (
    set "SELECTED_BOARD=%DEFAULT_BOARD%"
)

:: --------------------------------------------------
:: Interactive Mode – Select Build Type
:: --------------------------------------------------
echo.
echo Select Build Type:
echo   1. %TYPE_1%
echo   2. %TYPE_2%
echo.

set "SELECTION="
set /p "SELECTION=Enter choice [1-2] (Default=%DEFAULT_TYPE%): "

if "%SELECTION%"=="1" (
    set "SELECTED_BUILD_TYPE=%TYPE_1%"
) else if "%SELECTION%"=="2" (
    set "SELECTED_BUILD_TYPE=%TYPE_2%"
) else (
    set "SELECTED_BUILD_TYPE=%DEFAULT_TYPE%"
)

:: --------------------------------------------------
:: Build
:: --------------------------------------------------
:build
set "BOARD_PATH=%WORKSPACE_ROOT%\board\%SELECTED_BOARD%"
set "BUILD_PATH=%WORKSPACE_ROOT%\build\%SELECTED_BOARD%\%SELECTED_BUILD_TYPE%"

echo.
echo =====================================
echo   Building: %SELECTED_BOARD% (%SELECTED_BUILD_TYPE%)
echo =====================================
echo.

:: Validation
if not exist "%BOARD_PATH%" (
    echo [ERROR] Board directory missing: %BOARD_PATH%
    exit /b 1
)

:: Configure CMake
cmake -S "%BOARD_PATH%" -B "%BUILD_PATH%" --preset "%SELECTED_BUILD_TYPE%" -G "Ninja" >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake configuration failed.
    exit /b 1
)

:: Build
cmake --build "%BUILD_PATH%" --config "%SELECTED_BUILD_TYPE%" -j%NUMBER_OF_PROCESSORS%
if errorlevel 1 (
    echo [ERROR] Build failed.
    exit /b 1
)

:: --------------------------------------------------
:: Post-Build
:: --------------------------------------------------
echo.
echo =====================================
echo   Build Successful!
echo =====================================

set "ELF_SRC=%BUILD_PATH%\base.elf"
set "ELF_DEST=%WORKSPACE_ROOT%\build\%SELECTED_BOARD%.elf"
set "COMPILE_DB_SRC=%BUILD_PATH%\compile_commands.json"
set "COMPILE_DB_DEST=%WORKSPACE_ROOT%\build\compile_commands.json"

:: Copy Files
echo Copying files...
if exist "%ELF_SRC%" (
    copy /Y "%ELF_SRC%" "%ELF_DEST%" >nul
    echo   Firmware: %ELF_DEST%
)
if exist "%COMPILE_DB_SRC%" (
    copy /Y "%COMPILE_DB_SRC%" "%COMPILE_DB_DEST%" >nul
    echo   Index:    %COMPILE_DB_DEST%
)

echo =====================================
echo.
