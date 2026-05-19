@echo off
chcp 65001 >nul
REM Web-only demo - no QEMU, focus on functionality

echo ================================================================================
echo   Smart AC Control Gateway - WEB DEMO (No QEMU)
echo   Author: jiannan WEI (MC555577)
echo ================================================================================
echo.
echo [DEMO] Starting web-based demonstration...
echo [DEMO] Features: Weather API + Smart Control + Real-time Data
echo.

set PROJECT_ROOT=%~dp0..

echo [DEMO] 1. Starting Enhanced Web Bridge (with weather API)...
start "Enhanced Web Bridge" cmd /k "cd /d "%PROJECT_ROOT%\simulators" && python web_bridge.py"

timeout /t 3 /nobreak >nul

echo [DEMO] 2. Opening Chinese Interface...
start "" "%PROJECT_ROOT%\web\index_simple.html"

timeout /t 2 /nobreak >nul

echo [DEMO] 3. Opening English Interface...
start "" "%PROJECT_ROOT%\web\index_en.html"

echo.
echo ================================================================================
echo   WEB DEMO ACTIVE - Full Functionality
echo ================================================================================
echo.
echo Components running:
echo   - Web Bridge Server: http://localhost:8080
echo   - Weather API: Real Macau weather data
echo   - Chinese Interface: Smart AC control
echo   - English Interface: Smart AC control
echo.
echo Features demonstrated:
echo   - Real-time weather API (Open-Meteo)
echo   - Smart temperature suggestions
echo   - Air conditioner remote control
echo   - Multi-language interface
echo   - Responsive design
echo   - Live data updates
echo.
echo NOTE: This demo shows the complete web functionality
echo       without QEMU hardware simulation issues.
echo       The embedded firmware code is complete and ready.
echo.
echo Press any key when demo is complete...
pause >nul

echo.
echo [DEMO] Stopping services...
taskkill /f /im python.exe >nul 2>&1
echo [DEMO] Web demo completed!
pause