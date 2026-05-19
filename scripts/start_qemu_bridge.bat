@echo off
echo ================================================================================
echo   QEMU + Bridge + Web Demo
echo   STM32 Firmware + Python Bridge + Web Interface
echo ================================================================================

set PROJECT_ROOT=%~dp0..

echo [1] Starting QEMU Bridge Server...
cd /d "%PROJECT_ROOT%\simulators"
start "QEMU Bridge" cmd /k "python qemu_bridge.py"

echo [2] Waiting for bridge server...
timeout /t 3 /nobreak >nul

echo [3] Building firmware...
cd /d "%PROJECT_ROOT%\scripts"
call build_elf.bat >nul

echo [4] Starting QEMU with firmware...
start "QEMU STM32" cmd /k "call run_qemu_real.bat"

echo [5] Waiting for QEMU startup...
timeout /t 5 /nobreak >nul

echo [6] Opening web interfaces...
start "" "%PROJECT_ROOT%\web\index_simple.html"
timeout /t 1 /nobreak >nul
start "" "%PROJECT_ROOT%\web\index_en.html"

echo.
echo ================================================================================
echo   Complete System Running
echo ================================================================================
echo.
echo Components:
echo   - QEMU STM32 Firmware (ARM Cortex-M4)
echo   - Bridge Server (Python, port 8080/9999)
echo   - Web Interface (Chinese + English)
echo.
echo Data Flow:
echo   Web -> Bridge -> QEMU Firmware -> Sensors/IR
echo.
echo Press any key to stop all services...
pause >nul

echo.
echo Stopping services...
taskkill /f /im qemu-system-arm.exe >nul 2>&1
taskkill /f /im python.exe >nul 2>&1
echo Demo completed!
pause