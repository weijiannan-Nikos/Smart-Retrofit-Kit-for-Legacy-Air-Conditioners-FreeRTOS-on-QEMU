@echo off
REM 系统启动脚本 - 启动所有组件
REM System startup script - Start all components

echo ================================================================================
echo   智能空调控制网关 - 系统启动
echo   Intelligent Air Conditioner Control Gateway - System Startup
echo ================================================================================

set PROJECT_ROOT=%~dp0..

echo [SYSTEM] Starting all components...
echo.

REM 检查Python
python --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Python not found!
    pause
    exit /b 1
)

echo [SYSTEM] 1. Starting peripheral simulator...
start "Peripheral Simulator" cmd /k "cd /d %PROJECT_ROOT%\simulators && python peripheral_simulator.py"

timeout /t 2 /nobreak >nul

echo [SYSTEM] 2. Starting bridge server...
start "Bridge Server" cmd /k "cd /d %PROJECT_ROOT%\simulators && python bridge_server.py"

timeout /t 2 /nobreak >nul

echo [SYSTEM] 3. Starting QEMU firmware...
start "QEMU Firmware" cmd /k "cd /d %PROJECT_ROOT%\scripts && run_qemu.bat"

timeout /t 2 /nobreak >nul

echo [SYSTEM] 4. Opening web interface...
start "" "%PROJECT_ROOT%\web\index_simple.html"

echo.
echo [SYSTEM] All components started!
echo.
echo Components running:
echo   - Peripheral Simulator (Port 8081)
echo   - Bridge Server (Port 8080)
echo   - QEMU Firmware (Console)
echo   - Web Interface (Browser)
echo.
echo Press any key to exit...
pause >nul