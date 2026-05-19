@echo off
REM Build script for STM32 firmware

echo ================================================================================
echo   Smart AC Control Gateway - Firmware Build
echo ================================================================================

set PROJECT_ROOT=%~dp0..
set FIRMWARE_DIR=%PROJECT_ROOT%\firmware
set BUILD_DIR=%PROJECT_ROOT%\build
set FREERTOS_DIR=%PROJECT_ROOT%\FreeRTOS-LTS_sourcecode
set TOOLCHAIN_PATH=F:\GNU Arm Embedded Toolchain\10 2021.10\bin

echo [BUILD] Project root: %PROJECT_ROOT%
echo [BUILD] Firmware dir: %FIRMWARE_DIR%
echo [BUILD] Build dir: %BUILD_DIR%

REM Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

REM Check toolchain
echo [BUILD] Checking toolchain...
"%TOOLCHAIN_PATH%\arm-none-eabi-gcc.exe" --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] ARM GCC toolchain not found!
    pause
    exit /b 1
)

echo [BUILD] ARM GCC toolchain found

REM Compile parameters
set CC="%TOOLCHAIN_PATH%\arm-none-eabi-gcc.exe"
set CFLAGS=-mcpu=cortex-m4 -mthumb -DQEMU_MPS2_AN386
set CFLAGS=%CFLAGS% -I"%FIRMWARE_DIR%\inc"
set CFLAGS=%CFLAGS% -I"%FIRMWARE_DIR%\config"
set CFLAGS=%CFLAGS% -I"%FREERTOS_DIR%\FreeRTOS\FreeRTOS-Kernel\include"
set CFLAGS=%CFLAGS% -I"%FREERTOS_DIR%\FreeRTOS\FreeRTOS-Kernel\portable\GCC\ARM_CM4F"
set CFLAGS=%CFLAGS% -O2 -g -Wall

echo [BUILD] Compiling firmware...

REM Compile source files
%CC% %CFLAGS% -c "%FIRMWARE_DIR%\src\main.c" -o "%BUILD_DIR%\main.o"
if errorlevel 1 goto :error

%CC% %CFLAGS% -c "%FIRMWARE_DIR%\src\sensor_task.c" -o "%BUILD_DIR%\sensor_task.o"
if errorlevel 1 goto :error

%CC% %CFLAGS% -c "%FIRMWARE_DIR%\src\bridge_task.c" -o "%BUILD_DIR%\bridge_task.o"
if errorlevel 1 goto :error

%CC% %CFLAGS% -c "%FIRMWARE_DIR%\src\system_hal.c" -o "%BUILD_DIR%\system_hal.o"
if errorlevel 1 goto :error

%CC% %CFLAGS% -c "%FIRMWARE_DIR%\src\bridge_comm.c" -o "%BUILD_DIR%\bridge_comm.o"
if errorlevel 1 goto :error

%CC% %CFLAGS% -c "%FIRMWARE_DIR%\src\ir_task.c" -o "%BUILD_DIR%\ir_task.o"
if errorlevel 1 goto :error

%CC% %CFLAGS% -c "%FIRMWARE_DIR%\src\wifi_task.c" -o "%BUILD_DIR%\wifi_task.o"
if errorlevel 1 goto :error

%CC% %CFLAGS% -c "%FIRMWARE_DIR%\src\control_task.c" -o "%BUILD_DIR%\control_task.o"
if errorlevel 1 goto :error

%CC% %CFLAGS% -c "%FIRMWARE_DIR%\src\minimal_libc.c" -o "%BUILD_DIR%\minimal_libc.o"
if errorlevel 1 goto :error

echo [BUILD] Compilation successful!
echo [BUILD] Object files created in %BUILD_DIR%
echo [BUILD] Ready for QEMU execution

goto :end

:error
echo [ERROR] Build failed!
pause
exit /b 1

:end
echo [BUILD] Build completed successfully!
pause