@echo off
REM Optimized QEMU run with minimal resources

echo [QEMU] Running optimized version...

set BUILD_DIR=..\build
set QEMU_PATH=F:\QEMU

if not exist "%BUILD_DIR%\firmware.elf" (
    echo [ERROR] firmware.elf not found!
    pause
    exit /b 1
)

echo [QEMU] Starting with minimal resources (auto-exit in 10s)...

REM Start QEMU in background with timeout
start /b "" "%QEMU_PATH%\qemu-system-arm.exe" ^
    -M mps2-an386 ^
    -cpu cortex-m4 ^
    -kernel "%BUILD_DIR%\firmware.elf" ^
    -nographic ^
    -display none ^
    -no-reboot ^
    -no-shutdown ^
    -serial null ^
    -monitor none ^
    -vga none ^
    -m 32 ^
    -smp 1

REM Wait 10 seconds then kill
timeout /t 10 /nobreak >nul
taskkill /f /im qemu-system-arm.exe >nul 2>&1

echo [QEMU] Optimized execution completed
pause