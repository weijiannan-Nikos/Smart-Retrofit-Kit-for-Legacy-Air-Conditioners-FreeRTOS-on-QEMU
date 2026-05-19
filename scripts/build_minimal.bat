@echo off
REM Build minimal version

echo [BUILD] Building minimal version...

set TOOLCHAIN_PATH="F:\GNU Arm Embedded Toolchain\10 2021.10\bin"
set GCC=%TOOLCHAIN_PATH%\arm-none-eabi-gcc.exe
set SRC_DIR=..\firmware\src
set OUT_DIR=..\build

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

echo [BUILD] Compiling minimal main...
%GCC% -mcpu=cortex-m4 -mthumb -O2 -c "%SRC_DIR%\main_minimal.c" -o "%OUT_DIR%\main_minimal.o"
if errorlevel 1 goto :error

%GCC% -mcpu=cortex-m4 -mthumb -O2 -c "%SRC_DIR%\startup_simple.c" -o "%OUT_DIR%\startup_simple.o"
if errorlevel 1 goto :error

echo [BUILD] Linking minimal ELF...
%GCC% -mcpu=cortex-m4 -mthumb -nostartfiles -T linker_simple.ld ^
    "%OUT_DIR%\startup_simple.o" ^
    "%OUT_DIR%\main_minimal.o" ^
    -o "%OUT_DIR%\firmware_minimal.elf"

if errorlevel 1 goto :error

echo [BUILD] Minimal version SUCCESS!
goto :end

:error
echo [ERROR] Build failed!
pause
exit /b 1

:end
echo [BUILD] Minimal build completed!
pause