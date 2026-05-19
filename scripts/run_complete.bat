@echo off
REM Complete system startup script

echo ================================================================================
echo   Smart AC Control Gateway - Complete System
echo ================================================================================

set PROJECT_ROOT=%~dp0..

echo [SYSTEM] Starting complete system...
echo.

REM Check build files
if not exist "%PROJECT_ROOT%\build\main.o" (
    echo [ERROR] Firmware not built! Running build first...
    call "%~dp0build.bat"
    if errorlevel 1 (
        echo [ERROR] Build failed!
        pause
        exit /b 1
    )
)

echo [SYSTEM] 1. Starting Web Bridge Server...
start "Web Bridge Server" cmd /k "cd /d %PROJECT_ROOT%\simulators && python web_bridge.py"

timeout /t 3 /nobreak >nul

echo [SYSTEM] 2. Starting QEMU Firmware...
start "QEMU Firmware" cmd /k "cd /d %PROJECT_ROOT%\scripts && run_qemu.bat"

timeout /t 3 /nobreak >nul

echo [SYSTEM] 3. Opening Web Interface...
start "" "%PROJECT_ROOT%\web\index_simple.html"

echo.
echo [SYSTEM] Complete system started!
echo.
echo Components:
echo   - Web Bridge Server (Port 8080)
echo   - QEMU Firmware (Console)
echo   - Web Interface (Browser)
echo.
echo Press any key to exit...
pause >nul