@echo off
REM Simple QEMU run without serial issues

echo [QEMU] Starting firmware (simple mode)...

set BUILD_DIR=..\build
set QEMU_PATH=F:\QEMU

if not exist "%BUILD_DIR%\firmware.elf" (
    echo [ERROR] firmware.elf not found!
    pause
    exit /b 1
)

echo [QEMU] Running firmware in QEMU...
echo [QEMU] Press Ctrl+C to exit
echo.

REM Simple QEMU execution
"%QEMU_PATH%\qemu-system-arm.exe" ^
    -M mps2-an386 ^
    -cpu cortex-m4 ^
    -kernel "%BUILD_DIR%\firmware.elf" ^
    -nographic ^
    -d guest_errors

echo.
echo [QEMU] Execution completed
pause