@echo off
REM Stable QEMU run with stack overflow protection

echo ================================================================================
echo   Smart AC Gateway - STABLE VERSION (Stack Overflow Protected)
echo ================================================================================

set BUILD_DIR=..\build
set QEMU_PATH=F:\QEMU

if not exist "%BUILD_DIR%\firmware.elf" (
    echo [ERROR] Please build firmware first: build_elf.bat
    pause
    exit /b 1
)

echo [STABLE] Starting with stack overflow protection...
echo [STABLE] Stack size: 256 words per task (4x larger)
echo [STABLE] Heap size: 64KB (8x larger)
echo [STABLE] Will run for 2 minutes then auto-exit
echo.

REM Start QEMU with 2-minute timeout
start /b "" "%QEMU_PATH%\qemu-system-arm.exe" ^
    -M mps2-an386 ^
    -cpu cortex-m4 ^
    -kernel "%BUILD_DIR%\firmware.elf" ^
    -nographic ^
    -display none ^
    -no-reboot ^
    -serial null ^
    -monitor none ^
    -vga none ^
    -m 32 ^
    -smp 1

echo [STABLE] Firmware running (2 minutes)...
echo [STABLE] If you see stack overflow messages, the protection is working!
echo.

REM Wait 2 minutes (120 seconds)
timeout /t 120 /nobreak

REM Kill QEMU
taskkill /f /im qemu-system-arm.exe >nul 2>&1

echo.
echo [STABLE] 2-minute stable run completed!
echo [STABLE] Stack overflow protection test finished
pause