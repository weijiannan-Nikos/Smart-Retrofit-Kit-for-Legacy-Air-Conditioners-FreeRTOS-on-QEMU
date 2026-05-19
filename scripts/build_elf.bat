@echo off
REM Complete build script with ELF generation

echo [BUILD] Building firmware with ELF output...

set TOOLCHAIN_PATH="F:\GNU Arm Embedded Toolchain\10 2021.10\bin"
set GCC=%TOOLCHAIN_PATH%\arm-none-eabi-gcc.exe
set OBJCOPY=%TOOLCHAIN_PATH%\arm-none-eabi-objcopy.exe
set SRC_DIR=..\firmware\src
set INC_DIR=..\firmware\inc
set CFG_DIR=..\firmware\config
set OUT_DIR=..\build

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

REM Check toolchain
%GCC% --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Toolchain not found!
    pause
    exit /b 1
)

set CFLAGS=-mcpu=cortex-m4 -mthumb -DQEMU_MPS2_AN386 -I"%INC_DIR%" -I"%CFG_DIR%" -O2 -g -Wall -Wno-unused-function
set LDFLAGS=-mcpu=cortex-m4 -mthumb -nostartfiles -T linker.ld -Wl,--gc-sections

echo [BUILD] Compiling source files...

REM Compile all source files
%GCC% %CFLAGS% -c "%SRC_DIR%\main.c" -o "%OUT_DIR%\main.o"
if errorlevel 1 goto :error

%GCC% %CFLAGS% -c "%SRC_DIR%\system_hal.c" -o "%OUT_DIR%\system_hal.o"
if errorlevel 1 goto :error

%GCC% %CFLAGS% -c "%SRC_DIR%\minimal_libc.c" -o "%OUT_DIR%\minimal_libc.o"
if errorlevel 1 goto :error

%GCC% %CFLAGS% -c "%SRC_DIR%\freertos_stubs.c" -o "%OUT_DIR%\freertos_stubs.o"
if errorlevel 1 goto :error

%GCC% %CFLAGS% -c "%SRC_DIR%\sensor_task.c" -o "%OUT_DIR%\sensor_task.o"
if errorlevel 1 goto :error

%GCC% %CFLAGS% -c "%SRC_DIR%\ir_task.c" -o "%OUT_DIR%\ir_task.o"
if errorlevel 1 goto :error

%GCC% %CFLAGS% -c "%SRC_DIR%\wifi_task.c" -o "%OUT_DIR%\wifi_task.o"
if errorlevel 1 goto :error

%GCC% %CFLAGS% -c "%SRC_DIR%\control_task.c" -o "%OUT_DIR%\control_task.o"
if errorlevel 1 goto :error

%GCC% %CFLAGS% -c "%SRC_DIR%\bridge_task.c" -o "%OUT_DIR%\bridge_task.o"
if errorlevel 1 goto :error

%GCC% %CFLAGS% -c "%SRC_DIR%\bridge_comm.c" -o "%OUT_DIR%\bridge_comm.o"
if errorlevel 1 goto :error

%GCC% %CFLAGS% -c "%SRC_DIR%\startup.c" -o "%OUT_DIR%\startup.o"
if errorlevel 1 goto :error

%GCC% %CFLAGS% -c "%SRC_DIR%\syscalls.c" -o "%OUT_DIR%\syscalls.o"
if errorlevel 1 goto :error

echo [BUILD] Linking ELF file...

REM Link all object files
%GCC% %LDFLAGS% ^
    "%OUT_DIR%\startup.o" ^
    "%OUT_DIR%\main.o" ^
    "%OUT_DIR%\system_hal.o" ^
    "%OUT_DIR%\minimal_libc.o" ^
    "%OUT_DIR%\freertos_stubs.o" ^
    "%OUT_DIR%\sensor_task.o" ^
    "%OUT_DIR%\ir_task.o" ^
    "%OUT_DIR%\wifi_task.o" ^
    "%OUT_DIR%\control_task.o" ^
    "%OUT_DIR%\bridge_task.o" ^
    "%OUT_DIR%\bridge_comm.o" ^
    "%OUT_DIR%\syscalls.o" ^
    -o "%OUT_DIR%\firmware.elf"

if errorlevel 1 goto :error

echo [BUILD] Creating binary file...
%OBJCOPY% -O binary "%OUT_DIR%\firmware.elf" "%OUT_DIR%\firmware.bin"

echo [BUILD] SUCCESS!
echo [BUILD] Files created:
echo   - %OUT_DIR%\firmware.elf (for QEMU)
echo   - %OUT_DIR%\firmware.bin (binary)
echo.
echo [BUILD] Ready for QEMU execution:
echo   qemu-system-arm -M mps2-an386 -kernel %OUT_DIR%\firmware.elf -nographic
goto :end

:error
echo [ERROR] Build failed!
pause
exit /b 1

:end
echo [BUILD] Build completed successfully!
pause