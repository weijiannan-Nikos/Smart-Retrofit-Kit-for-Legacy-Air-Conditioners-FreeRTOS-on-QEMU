@echo off
echo ================================================================================
echo 测试死锁保护机制 - STM32F407 + FreeRTOS
echo Testing Deadlock Protection - STM32F407 + FreeRTOS  
echo ================================================================================
echo.

cd /d "F:\University of Macau master course\embeded system\SENSOR_project\project2 - 副本"

echo [1] 编译带死锁保护的固件...
echo [1] Compiling firmware with deadlock protection...
cd scripts
call build_elf.bat
if errorlevel 1 (
    echo 编译失败！
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo [2] 启动QEMU运行固件 (5分钟自动超时)...
echo [2] Starting QEMU with firmware (5-minute auto timeout)...
echo.
echo 监控要点 / Monitoring points:
echo - 任务超时检测 / Task timeout detection
echo - 互斥锁超时保护 / Mutex timeout protection  
echo - 队列阻塞保护 / Queue blocking protection
echo - 自动恢复机制 / Automatic recovery mechanism
echo - 5分钟后自动退出 / Auto exit after 5 minutes
echo.

timeout /t 3 /nobreak > nul

call run_qemu_real.bat

echo.
echo ================================================================================
echo 死锁保护测试完成！
echo Deadlock protection test completed!
echo ================================================================================
pause