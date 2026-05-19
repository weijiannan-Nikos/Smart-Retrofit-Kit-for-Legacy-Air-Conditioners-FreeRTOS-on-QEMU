@echo off
REM Link object files into executable

echo [LINK] Linking object files...

set TOOLCHAIN="F:\GNU Arm Embedded Toolchain\10 2021.10\bin\arm-none-eabi-gcc.exe"
set OUT_DIR=..\build

REM Link all object files
%TOOLCHAIN% -mcpu=cortex-m4 -mthumb -nostartfiles -T linker.ld ^
    "%OUT_DIR%\main.o" ^
    "%OUT_DIR%\system_hal.o" ^
    "%OUT_DIR%\minimal_libc.o" ^
    "%OUT_DIR%\sensor_task.o" ^
    "%OUT_DIR%\ir_task.o" ^
    "%OUT_DIR%\wifi_task.o" ^
    "%OUT_DIR%\control_task.o" ^
    "%OUT_DIR%\bridge_task.o" ^
    "%OUT_DIR%\bridge_comm.o" ^
    "%OUT_DIR%\freertos_stubs.o" ^
    -o "%OUT_DIR%\firmware.elf"

if errorlevel 1 (
    echo [ERROR] Link failed!
    pause
    exit /b 1
)

echo [LINK] Success! firmware.elf created
pause