@echo off
REM Run minimal version

echo [QEMU] Running minimal version...

set BUILD_DIR=..\build
set QEMU_PATH=F:\QEMU

if not exist "%BUILD_DIR%\firmware_minimal.elf" (
    echo [ERROR] firmware_minimal.elf not found!
    pause
    exit /b 1
)

echo [QEMU] Starting minimal firmware (will exit quickly)...

"%QEMU_PATH%\qemu-system-arm.exe" ^
    -M mps2-an386 ^
    -cpu cortex-m4 ^
    -kernel "%BUILD_DIR%\firmware_minimal.elf" ^
    -nographic ^
    -display none ^
    -no-reboot ^
    -no-shutdown ^
    -serial null ^
    -monitor none ^
    -parallel none ^
    -vga none ^
    -m 8 ^
    -smp 1

echo [QEMU] Minimal execution completed successfully!
pause