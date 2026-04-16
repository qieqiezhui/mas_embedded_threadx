@echo off
setlocal enabledelayedexpansion

:: =============================================================
::  .zed\flash.bat  <board> <probe>
::
::  Build project then flash firmware to the target device.
::
::  Usage:
::    flash.bat <board> <probe>
::
::  Boards:   damiao_h7  |  dji_c
::  Probes:   stlink     |  daplink  |  jlink
::
::  Probe details:
::    stlink  -> OpenOCD + interface/stlink.cfg
::    daplink -> OpenOCD + interface/cmsis-dap.cfg
::    jlink   -> SEGGER JLink Commander (JLink.exe)
:: =============================================================

set "BOARD=%~1"
set "PROBE=%~2"
set "BUILD_TYPE=Debug"
set "SCRIPT_DIR=%~dp0"

:: J-Link installation path
set "JLINK_DIR=D:\jlink\JLink"

if "%BOARD%"=="" goto :usage
if "%PROBE%"=="" goto :usage

:: =============================================================
::  Board config  ->  OpenOCD target + J-Link device name
:: =============================================================
if /I "%BOARD%"=="damiao_h7" (
    set "TARGET=stm32h7x"
    set "JLINK_DEV=STM32H743BI"
    goto :board_ok
)
if /I "%BOARD%"=="dji_c" (
    set "TARGET=stm32f4x"
    set "JLINK_DEV=STM32F407IG"
    goto :board_ok
)
echo.
echo [ERROR] Unknown board: "%BOARD%"
echo         Supported: damiao_h7, dji_c
exit /b 1

:board_ok

:: =============================================================
::  Probe validation
:: =============================================================
if /I "%PROBE%"=="stlink"  goto :probe_ok
if /I "%PROBE%"=="daplink" goto :probe_ok
if /I "%PROBE%"=="jlink"   goto :probe_ok
echo.
echo [ERROR] Unknown probe: "%PROBE%"
echo         Supported: stlink, daplink, jlink
exit /b 1

:probe_ok

:: ELF paths: relative (forward slashes) for OpenOCD,
::            absolute (backslashes)    for J-Link
set "ELF_REL=build/%BOARD%/%BUILD_TYPE%/base.elf"
set "ELF_ABS=%SCRIPT_DIR%..\build\%BOARD%\%BUILD_TYPE%\base.elf"

echo.
echo ============================================================
echo   Flash:  [%PROBE%]  %BOARD%  (%BUILD_TYPE%)
echo ============================================================

:: =============================================================
::  [1/2]  Build
:: =============================================================
echo.
echo [1/2]  Building %BOARD% ^(%BUILD_TYPE%^) ...
echo.

set "NOPAUSE=1"
call "%SCRIPT_DIR%build.bat" "%BOARD%" "%BUILD_TYPE%"
if errorlevel 1 (
    echo.
    echo [ERROR] Build failed - aborting flash.
    exit /b 1
)

:: =============================================================
::  [2/2]  Flash
:: =============================================================
echo.
echo [2/2]  Flashing firmware ...
echo.

if /I "%PROBE%"=="jlink" goto :flash_jlink

:: ── OpenOCD  (stlink / daplink) ──────────────────────────────
if /I "%PROBE%"=="stlink"  set "IFACE=interface/stlink.cfg"
if /I "%PROBE%"=="daplink" set "IFACE=interface/cmsis-dap.cfg"

echo   Probe     : %PROBE%
echo   Interface : %IFACE%
echo   Target    : target/%TARGET%.cfg
echo   ELF       : %ELF_REL%
echo.

openocd -f %IFACE% ^
        -f target/%TARGET%.cfg ^
        -c "program %ELF_REL% verify reset exit"

if errorlevel 1 (
    echo.
    echo [ERROR] OpenOCD flash failed.
    echo         Check that the probe is connected and the target is powered.
    exit /b 1
)
goto :done

:: ── SEGGER J-Link Commander ───────────────────────────────────
:flash_jlink
set "JLINK_EXE=%JLINK_DIR%\JLink.exe"
set "JLINK_SCRIPT=%TEMP%\zed_flash_%BOARD%.jlink"

if not exist "%JLINK_EXE%" (
    echo.
    echo [ERROR] JLink.exe not found: %JLINK_EXE%
    echo         Edit JLINK_DIR in flash.bat to match your installation.
    exit /b 1
)

echo   Device : %JLINK_DEV%
echo   ELF    : %ELF_ABS%
echo   Script : %JLINK_SCRIPT%
echo.

:: Write J-Link Commander script to a temp file
(
    echo loadfile "%ELF_ABS%"
    echo r
    echo g
    echo exit
) > "%JLINK_SCRIPT%"

"%JLINK_EXE%" -device %JLINK_DEV% ^
              -if SWD ^
              -speed 4000 ^
              -autoconnect 1 ^
              -CommanderScript "%JLINK_SCRIPT%"

set "JLINK_ERR=%errorlevel%"
del "%JLINK_SCRIPT%" >nul 2>&1

if %JLINK_ERR% neq 0 (
    echo.
    echo [ERROR] J-Link flash failed.
    echo         Check that the probe is connected and JLINK_DEV is correct.
    echo         Run: "%JLINK_EXE%" -listdevices ^| findstr STM32
    exit /b 1
)

:: ── Done ─────────────────────────────────────────────────────
:done
echo.
echo ============================================================
echo   Flash Complete!  [%PROBE%]  %BOARD%
echo ============================================================
echo.
exit /b 0

:: =============================================================
:usage
echo.
echo   Usage : flash.bat ^<board^> ^<probe^>
echo.
echo   board : damiao_h7 ^| dji_c
echo   probe : stlink    ^| daplink ^| jlink
echo.
echo   Examples:
echo     flash.bat damiao_h7 stlink
echo     flash.bat dji_c     daplink
echo     flash.bat damiao_h7 jlink
echo.
exit /b 1
