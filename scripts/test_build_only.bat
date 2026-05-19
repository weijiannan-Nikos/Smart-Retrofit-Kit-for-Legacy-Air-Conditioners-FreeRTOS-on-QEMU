@echo off
echo ================================================================================
echo 测试构建死锁保护固件
echo Testing Build with Deadlock Protection
echo ================================================================================

cd /d "F:\University of Macau master course\embeded system\SENSOR_project\project2 - 副本\scripts"

echo [TEST] Building firmware with deadlock protection...
call build_elf.bat

if errorlevel 1 (
    echo [ERROR] Build failed!
    echo 可能的问题 / Possible issues:
    echo - 工具链路径错误 / Toolchain path incorrect
    echo - 源文件缺失 / Source files missing  
    echo - 头文件路径错误 / Header path incorrect
    pause
    exit /b 1
) else (
    echo [SUCCESS] Build completed!
    echo 固件已生成 / Firmware generated:
    dir ..\build\firmware.elf
    echo.
    echo 可以运行QEMU测试 / Ready for QEMU test
)

pause