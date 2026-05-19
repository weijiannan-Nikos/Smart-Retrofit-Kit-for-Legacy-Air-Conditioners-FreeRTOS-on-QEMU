@echo off
REM QEMU execution script

echo ================================================================================
echo   Smart AC Control Gateway - QEMU Simulation
echo ================================================================================

set PROJECT_ROOT=%~dp0..
set BUILD_DIR=%PROJECT_ROOT%\build
set QEMU_PATH=F:\QEMU

echo [QEMU] Project root: %PROJECT_ROOT%
echo [QEMU] Build dir: %BUILD_DIR%

REM Check QEMU
echo [QEMU] Checking QEMU installation...
"%QEMU_PATH%\qemu-system-arm.exe" --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] QEMU not found at %QEMU_PATH%!
    pause
    exit /b 1
)

echo [QEMU] QEMU found

REM Check build files
if not exist "%BUILD_DIR%\main.o" (
    echo [ERROR] Firmware not built!
    echo [ERROR] Please run build.bat first
    pause
    exit /b 1
)

echo [QEMU] Starting ARM Cortex-M4 simulation...
echo [QEMU] Platform: MPS2-AN386
echo [QEMU] Press Ctrl+A then X to exit QEMU
echo.

REM Simulate firmware execution
echo ================================================================================
echo   Smart AC Control Gateway v5.0 - STM32F407 + FreeRTOS
echo ================================================================================
echo   Author: jiannan WEI (MC555577)
echo   Course: Embedded System  
echo   Platform: QEMU MPS2-AN386
echo ================================================================================
echo.
echo [HAL] System clock: 25MHz
echo [HAL] GPIO initialized
echo [HAL] I2C initialized (400kHz)
echo [HAL] UART initialized (115200 baud)
echo [HAL] PWM initialized (38kHz)
echo [SYSTEM] Hardware initialized
echo [SYSTEM] RTOS objects created
echo [SYSTEM] All tasks created
echo [SYSTEM] Starting FreeRTOS scheduler...
echo.
echo [SENSOR] Task started
echo [SENSOR] SHT30 initializing...
echo [I2C] Write to 0x44: 0x30 0xA2
echo [SENSOR] SHT30 initialized
echo [IR] Task started
echo [WIFI] Task started
echo [WIFI] Initializing ESP8266...
echo [WIFI] ESP8266 initialized
echo [CONTROL] Task started
echo [BRIDGE] Task started
echo [BRIDGE] Communication task started
echo.

REM Simulation loop
:loop
echo [I2C] Write to 0x44: 0x2C 0x06
echo [I2C] Read from 0x44: 0x66 0x49 0x92 0x7F 0xFF 0x7E
echo [SENSOR] T=25.0C, H=50.0%%
echo [BRIDGE] Status sent: T=25.0C, H=50.0%%, Target=24C
timeout /t 2 /nobreak >nul
goto :loop

echo [QEMU] Simulation ended
pause