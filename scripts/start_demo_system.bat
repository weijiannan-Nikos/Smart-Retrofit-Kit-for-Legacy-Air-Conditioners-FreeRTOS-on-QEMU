@echo off
REM Complete demo system startup

echo ================================================================================
echo   Smart AC Control Gateway - Complete Demo System
echo ================================================================================

set PROJECT_ROOT=%~dp0..

echo [DEMO] Starting complete demonstration system...
echo.

REM Check if firmware is built
if not exist "%PROJECT_ROOT%\build\firmware.elf" (
    echo [DEMO] Building firmware first...
    call "%~dp0build_elf.bat"
    if errorlevel 1 (
        echo [ERROR] Build failed!
        pause
        exit /b 1
    )
)

echo [DEMO] 1. Starting Web Bridge Server (with weather API)...
start "Web Bridge Server" cmd /k "cd /d %PROJECT_ROOT%\simulators && python web_bridge.py"

timeout /t 3 /nobreak >nul

echo [DEMO] 2. Starting QEMU Firmware (5 minutes)...
start "QEMU Firmware Demo" cmd /k "cd /d %PROJECT_ROOT%\scripts && run_demo.bat"

timeout /t 3 /nobreak >nul

echo [DEMO] 3. Opening Web Interface...
start "" "%PROJECT_ROOT%\web\index_simple.html"

echo.
echo ================================================================================
echo   DEMO SYSTEM ACTIVE
echo ================================================================================
echo.
echo Components running:
echo   - QEMU Firmware: STM32F407 + FreeRTOS (5 minutes)
echo   - Web Bridge: http://localhost:8080 (with weather API)
echo   - Web Interface: Smart AC control panel
echo.
echo Features demonstrated:
echo   - Real-time temperature/humidity monitoring
echo   - Weather API integration (outdoor conditions)
echo   - Smart temperature suggestions
echo   - Air conditioner remote control
echo   - Multi-language interface (Chinese/English)
echo   - FreeRTOS task scheduling
echo.
echo The demo will run for 5 minutes automatically.
echo You can interact with the web interface during this time.
echo.
pause