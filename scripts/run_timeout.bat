@echo off
REM Run with timeout to avoid hanging

echo [QEMU] Running with 10 second timeout...

set BUILD_DIR=..\build
set QEMU_PATH=F:\QEMU

timeout /t 10 /nobreak "%QEMU_PATH%\qemu-system-arm.exe" ^
    -M mps2-an386 ^
    -cpu cortex-m4 ^
    -kernel "%BUILD_DIR%\firmware_minimal.elf" ^
    -nographic ^
    -semihosting-config enable=on,target=native

echo [QEMU] Timeout completed
pause