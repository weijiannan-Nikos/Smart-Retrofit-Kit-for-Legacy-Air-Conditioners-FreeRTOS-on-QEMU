@echo off
echo ================================================================================
echo   Complete QEMU + Web System Startup
echo   STM32 Firmware + Bridge Server + Web Interface
echo ================================================================================

set PROJECT_ROOT=%~dp0..

echo [1/4] Building firmware with deadlock protection...
cd /d "%PROJECT_ROOT%\scripts"
call build_elf.bat
if errorlevel 1 (
    echo [ERROR] Build failed! Check toolchain and source files.
    pause
    exit /b 1
)

echo [2/4] Starting QEMU Bridge Server...
cd /d "%PROJECT_ROOT%\simulators"
start "QEMU Bridge" cmd /k "python qemu_bridge.py"

echo [3/4] Waiting for bridge server startup...
timeout /t 3 /nobreak >nul

echo [4/4] Starting QEMU with STM32 firmware (5-min timeout)...
cd /d "%PROJECT_ROOT%\scripts"
start "QEMU STM32" cmd /k "run_qemu_real.bat"

echo [5/5] Opening web interfaces...
timeout /t 2 /nobreak >nul
start "" "%PROJECT_ROOT%\web\index_simple.html"
timeout /t 1 /nobreak >nul
start "" "%PROJECT_ROOT%\web\index_en.html"

echo.
echo ================================================================================
echo   System Ready - All Components Running
echo ================================================================================
echo.
echo Components:
echo   - QEMU STM32F407 Firmware (Real ARM Cortex-M4)
echo   - FreeRTOS with deadlock protection
echo   - Bridge Server (localhost:8080 + localhost:9999)
echo   - Chinese Web Interface
echo   - English Web Interface
echo   - Auto-timeout after 5 minutes
echo.
echo Data Flow: Web Interface <-> Bridge Server <-> QEMU STM32 Firmware
echo.
echo Troubleshooting:
echo   - If QEMU crashes: Check firmware build
echo   - If Web shows disconnected: Check bridge server
echo   - If commands fail: Check QEMU output
echo.
echo Press any key to stop all services...
pause >nul

echo.
echo Stopping all services...
taskkill /f /im qemu-system-arm.exe >nul 2>&1
taskkill /f /im python.exe >nul 2>&1
echo System shutdown complete!
pause