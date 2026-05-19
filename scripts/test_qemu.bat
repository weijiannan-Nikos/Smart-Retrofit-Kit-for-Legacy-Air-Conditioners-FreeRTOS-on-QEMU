@echo off
REM Test QEMU with absolute minimal program

echo [TEST] Testing QEMU with minimal program...

set QEMU_PATH=F:\QEMU
set OUT_DIR=..\build

REM Create absolute minimal C program
echo void _start(void) { return; } > "%OUT_DIR%\minimal.c"

REM Compile with minimal flags
"F:\GNU Arm Embedded Toolchain\10 2021.10\bin\arm-none-eabi-gcc.exe" ^
    -mcpu=cortex-m4 -mthumb -nostdlib -nostartfiles ^
    -Wl,--entry=_start -o "%OUT_DIR%\minimal.elf" "%OUT_DIR%\minimal.c"

if errorlevel 1 (
    echo [ERROR] Compilation failed
    pause
    exit /b 1
)

echo [TEST] Running minimal test (5 second timeout)...

REM Run with timeout
start /b "" "%QEMU_PATH%\qemu-system-arm.exe" ^
    -M mps2-an386 -cpu cortex-m4 -kernel "%OUT_DIR%\minimal.elf" -nographic

REM Wait 5 seconds then kill
timeout /t 5 /nobreak >nul
taskkill /f /im qemu-system-arm.exe >nul 2>&1

echo [TEST] Test completed - QEMU should have run briefly
pause