@echo off
REM Build simple version for QEMU testing

echo [BUILD] Building simple version...

set TOOLCHAIN_PATH="F:\GNU Arm Embedded Toolchain\10 2021.10\bin"
set GCC=%TOOLCHAIN_PATH%\arm-none-eabi-gcc.exe
set SRC_DIR=..\firmware\src
set OUT_DIR=..\build

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

set CFLAGS=-mcpu=cortex-m4 -mthumb -O2 -g -Wall
set LDFLAGS=-mcpu=cortex-m4 -mthumb -nostartfiles -T linker_simple.ld

echo [BUILD] Compiling simple main...
%GCC% %CFLAGS% -c "%SRC_DIR%\main_simple.c" -o "%OUT_DIR%\main_simple.o"
if errorlevel 1 goto :error

%GCC% %CFLAGS% -c "%SRC_DIR%\startup_simple.c" -o "%OUT_DIR%\startup_simple.o"
if errorlevel 1 goto :error

echo [BUILD] Linking simple ELF...
%GCC% %LDFLAGS% ^
    "%OUT_DIR%\startup_simple.o" ^
    "%OUT_DIR%\main_simple.o" ^
    -o "%OUT_DIR%\firmware_simple.elf"

if errorlevel 1 goto :error

echo [BUILD] Simple version SUCCESS!
echo [BUILD] Output: %OUT_DIR%\firmware_simple.elf
goto :end

:error
echo [ERROR] Build failed!
pause
exit /b 1

:end
echo [BUILD] Simple build completed!
pause