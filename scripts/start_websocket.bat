@echo off
echo ================================================================================
echo   Smart AC Control - WebSocket Real-time Demo
echo   Author: jiannan WEI (MC555577)
echo ================================================================================
echo.

set PROJECT_ROOT=%~dp0..

echo [DEMO] Installing websockets module if needed...
pip install websockets >nul 2>&1

echo [DEMO] Starting WebSocket Bridge Server...
start "WebSocket Bridge" cmd /k "cd /d "%PROJECT_ROOT%\simulators" && python websocket_bridge.py"

echo [DEMO] Waiting for server startup...
timeout /t 3 /nobreak >nul

echo [DEMO] Opening WebSocket Interface...
start "" "%PROJECT_ROOT%\web\index_websocket.html"

echo.
echo ================================================================================
echo   WebSocket Demo Active - Real-time Connection
echo ================================================================================
echo.
echo Components:
echo   - WebSocket Server: ws://localhost:8765
echo   - HTTP Fallback: http://localhost:8080  
echo   - Real-time Interface: index_websocket.html
echo.
echo Features:
echo   - Instant real-time updates
echo   - Live sensor data streaming
echo   - Immediate command feedback
echo   - Automatic reconnection
echo   - HTTP fallback support
echo.
echo Press any key to stop demo...
pause >nul

echo.
echo [DEMO] Stopping services...
taskkill /f /im python.exe >nul 2>&1
echo [DEMO] WebSocket demo completed!
pause