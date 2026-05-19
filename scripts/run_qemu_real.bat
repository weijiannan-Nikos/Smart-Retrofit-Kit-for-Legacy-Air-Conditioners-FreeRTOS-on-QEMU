@echo off
REM Run firmware in QEMU

echo ================================================================================
echo   Smart AC Control Gateway - QEMU Execution
echo ================================================================================

set PROJECT_ROOT=%~dp0..
set BUILD_DIR=%PROJECT_ROOT%\build
set QEMU_PATH=F:\QEMU

echo [QEMU] Checking firmware...
if not exist "%BUILD_DIR%\firmware.elf" (
    echo [ERROR] firmware.elf not found!
    echo [ERROR] Please run build_elf.bat first
    pause
    exit /b 1
)

echo [QEMU] Checking QEMU...
"%QEMU_PATH%\qemu-system-arm.exe" --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] QEMU not found at %QEMU_PATH%!
    pause
    exit /b 1
)

echo [QEMU] Starting firmware in QEMU...
echo [QEMU] Platform: MPS2-AN386 (ARM Cortex-M4)
echo [QEMU] Press Ctrl+A then X to exit
echo.

REM Run QEMU with simple command
"%QEMU_PATH%\qemu-system-arm.exe" -M mps2-an386 -cpu cortex-m4 -kernel "%BUILD_DIR%\firmware.elf" -nographic

echo.
echo [QEMU] Execution ended
pause