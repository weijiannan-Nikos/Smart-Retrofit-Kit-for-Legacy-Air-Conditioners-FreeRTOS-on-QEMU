@echo off
REM Run simple version in QEMU

echo [QEMU] Running simple version...

set BUILD_DIR=..\build
set QEMU_PATH=F:\QEMU

if not exist "%BUILD_DIR%\firmware_simple.elf" (
    echo [ERROR] firmware_simple.elf not found!
    echo [ERROR] Run build_simple.bat first
    pause
    exit /b 1
)

echo [QEMU] Starting simple firmware...
echo [QEMU] Press Ctrl+C to exit
echo.

"%QEMU_PATH%\qemu-system-arm.exe" ^
    -M mps2-an386 ^
    -cpu cortex-m4 ^
    -kernel "%BUILD_DIR%\firmware_simple.elf" ^
    -nographic ^
    -semihosting-config enable=on,target=native

echo.
echo [QEMU] Simple execution completed
pause