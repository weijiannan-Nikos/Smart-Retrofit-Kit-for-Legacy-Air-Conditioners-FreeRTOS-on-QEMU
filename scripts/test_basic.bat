@echo off
REM Test basic ARM execution

echo [TEST] Testing basic ARM code...

set TOOLCHAIN_PATH="F:\GNU Arm Embedded Toolchain\10 2021.10\bin"
set GCC=%TOOLCHAIN_PATH%\arm-none-eabi-gcc.exe
set OUT_DIR=..\build

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

echo [TEST] Creating minimal test...

echo int main(void) { while(1); return 0; } > "%OUT_DIR%\test.c"

%GCC% -mcpu=cortex-m4 -mthumb -nostdlib -nostartfiles -Wl,--entry=main -o "%OUT_DIR%\test.elf" "%OUT_DIR%\test.c"

if errorlevel 1 (
    echo [ERROR] Basic test failed!
    pause
    exit /b 1
)

echo [TEST] Basic ARM code compiled successfully!
echo [TEST] Running in QEMU...

"F:\QEMU\qemu-system-arm.exe" -M mps2-an386 -cpu cortex-m4 -kernel "%OUT_DIR%\test.elf" -nographic -d guest_errors

echo [TEST] Test completed
pause