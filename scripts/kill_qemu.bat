@echo off
REM Kill all QEMU processes

echo [KILL] Terminating all QEMU processes...

taskkill /f /im qemu-system-arm.exe >nul 2>&1

echo [KILL] QEMU processes terminated
pause