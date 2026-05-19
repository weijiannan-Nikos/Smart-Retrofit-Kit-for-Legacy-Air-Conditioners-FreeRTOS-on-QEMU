@echo off
REM Test compilation without linking

echo [TEST] Testing compilation only...

set TOOLCHAIN="F:\GNU Arm Embedded Toolchain\10 2021.10\bin\arm-none-eabi-gcc.exe"
set SRC_DIR=..\firmware\src
set INC_DIR=..\firmware\inc
set CFG_DIR=..\firmware\config
set OUT_DIR=..\build

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

set FLAGS=-mcpu=cortex-m4 -mthumb -DQEMU_MPS2_AN386 -I"%INC_DIR%" -I"%CFG_DIR%" -O0 -g -Wall -c

echo [TEST] Compiling main.c...
%TOOLCHAIN% %FLAGS% "%SRC_DIR%\main.c" -o "%OUT_DIR%\main.o"
if errorlevel 1 goto :error

echo [TEST] Compiling system_hal.c...
%TOOLCHAIN% %FLAGS% "%SRC_DIR%\system_hal.c" -o "%OUT_DIR%\system_hal.o"
if errorlevel 1 goto :error

echo [TEST] Compiling minimal_libc.c...
%TOOLCHAIN% %FLAGS% "%SRC_DIR%\minimal_libc.c" -o "%OUT_DIR%\minimal_libc.o"
if errorlevel 1 goto :error

echo [TEST] Compiling freertos_stubs.c...
%TOOLCHAIN% %FLAGS% "%SRC_DIR%\freertos_stubs.c" -o "%OUT_DIR%\freertos_stubs.o"
if errorlevel 1 goto :error

echo [TEST] SUCCESS! All core files compiled.
goto :end

:error
echo [ERROR] Compilation failed!
pause
exit /b 1

:end
echo [TEST] Test compilation completed
pause