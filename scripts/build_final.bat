@echo off
REM Final working build script

echo [BUILD] Starting build...

set TOOLCHAIN="F:\GNU Arm Embedded Toolchain\10 2021.10\bin\arm-none-eabi-gcc.exe"
set FREERTOS_INC="..\FreeRTOS-LTS_sourcecode\FreeRTOS\FreeRTOS-Kernel\include"
set FREERTOS_PORT="..\FreeRTOS-LTS_sourcecode\FreeRTOS\FreeRTOS-Kernel\portable\GCC\ARM_CM4F"
set SRC_DIR=..\firmware\src
set INC_DIR=..\firmware\inc
set CFG_DIR=..\firmware\config
set OUT_DIR=..\build

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

%TOOLCHAIN% --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Toolchain not found!
    pause
    exit /b 1
)

set FLAGS=-mcpu=cortex-m4 -mthumb -DQEMU_MPS2_AN386 -I"%INC_DIR%" -I"%CFG_DIR%" -I%FREERTOS_INC% -I%FREERTOS_PORT% -O2 -g -Wall -Wno-unused-function

echo [BUILD] Compiling main.c...
%TOOLCHAIN% %FLAGS% -c "%SRC_DIR%\main.c" -o "%OUT_DIR%\main.o"
if errorlevel 1 goto :error

echo [BUILD] Compiling system_hal.c...
%TOOLCHAIN% %FLAGS% -c "%SRC_DIR%\system_hal.c" -o "%OUT_DIR%\system_hal.o"
if errorlevel 1 goto :error

echo [BUILD] Compiling minimal_libc.c...
%TOOLCHAIN% %FLAGS% -c "%SRC_DIR%\minimal_libc.c" -o "%OUT_DIR%\minimal_libc.o"
if errorlevel 1 goto :error

echo [BUILD] Compiling sensor_task.c...
%TOOLCHAIN% %FLAGS% -c "%SRC_DIR%\sensor_task.c" -o "%OUT_DIR%\sensor_task.o"
if errorlevel 1 goto :error

echo [BUILD] Compiling ir_task.c...
%TOOLCHAIN% %FLAGS% -c "%SRC_DIR%\ir_task.c" -o "%OUT_DIR%\ir_task.o"
if errorlevel 1 goto :error

echo [BUILD] Compiling wifi_task.c...
%TOOLCHAIN% %FLAGS% -c "%SRC_DIR%\wifi_task.c" -o "%OUT_DIR%\wifi_task.o"
if errorlevel 1 goto :error

echo [BUILD] Compiling control_task.c...
%TOOLCHAIN% %FLAGS% -c "%SRC_DIR%\control_task.c" -o "%OUT_DIR%\control_task.o"
if errorlevel 1 goto :error

echo [BUILD] Compiling bridge_task.c...
%TOOLCHAIN% %FLAGS% -c "%SRC_DIR%\bridge_task.c" -o "%OUT_DIR%\bridge_task.o"
if errorlevel 1 goto :error

echo [BUILD] Compiling bridge_comm.c...
%TOOLCHAIN% %FLAGS% -c "%SRC_DIR%\bridge_comm.c" -o "%OUT_DIR%\bridge_comm.o"
if errorlevel 1 goto :error

echo [BUILD] Compiling freertos_stubs.c...
%TOOLCHAIN% %FLAGS% -c "%SRC_DIR%\freertos_stubs.c" -o "%OUT_DIR%\freertos_stubs.o"
if errorlevel 1 goto :error

echo [BUILD] SUCCESS! All files compiled.
echo [BUILD] Object files in %OUT_DIR%
goto :end

:error
echo [ERROR] Build failed!
pause
exit /b 1

:end
echo [BUILD] Build completed successfully!
pause