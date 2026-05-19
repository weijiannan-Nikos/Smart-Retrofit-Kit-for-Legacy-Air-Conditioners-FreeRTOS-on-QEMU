@echo off
REM Demo version - runs for 5 minutes with full output

echo ================================================================================
echo   Smart AC Control Gateway - DEMO VERSION
echo   Author: jiannan WEI (MC555577)
echo ================================================================================
echo.
echo [DEMO] Starting 5-minute demonstration...
echo [DEMO] Features: FreeRTOS + Sensors + IR + WiFi + Web Interface
echo.

set BUILD_DIR=..\build
set QEMU_PATH=F:\QEMU

if not exist "%BUILD_DIR%\firmware.elf" (
    echo [ERROR] Please build firmware first: build_elf.bat
    pause
    exit /b 1
)

echo [DEMO] Starting QEMU firmware (5 minutes)...

REM Start QEMU with demo timeout (5 minutes = 300 seconds)
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
    -m 32

echo [DEMO] Firmware running... (will auto-stop in 5 minutes)
echo [DEMO] You can now:
echo   1. Open web interface: web\index_simple.html
echo   2. Start bridge server: python simulators\web_bridge.py
echo   3. View real-time data and weather information
echo.

REM Wait 5 minutes (300 seconds)
timeout /t 300 /nobreak

REM Kill QEMU
taskkill /f /im qemu-system-arm.exe >nul 2>&1

echo.
echo [DEMO] 5-minute demonstration completed!
pause