@echo off
chcp 65001 >nul

echo ================================================================================
echo   智能空调控制网关 - 完整功能演示
echo   Smart AC Control Gateway - Full Feature Demo
echo   Author: jiannan WEI (MC555577)
echo ================================================================================
echo.

set PROJECT_ROOT=%~dp0..

echo [DEMO] 启动独立Web桥接服务器...
echo [DEMO] Starting standalone web bridge server...
start "AC Control Bridge" cmd /k "cd /d "%PROJECT_ROOT%\simulators" && python web_bridge_standalone.py"

echo [DEMO] 等待服务器启动...
timeout /t 3 /nobreak >nul

echo [DEMO] 打开中文控制界面...
start "" "%PROJECT_ROOT%\web\index_simple.html"

timeout /t 2 /nobreak >nul

echo [DEMO] 打开英文控制界面...
start "" "%PROJECT_ROOT%\web\index_en.html"

echo.
echo ================================================================================
echo   演示系统已启动 - Demo System Active
echo ================================================================================
echo.
echo 运行组件 Running Components:
echo   ✅ Web桥接服务器: http://localhost:8080
echo   ✅ 天气API: 澳门实时天气数据
echo   ✅ 中文控制界面: 智能空调控制
echo   ✅ 英文控制界面: Smart AC Control
echo.
echo 功能特性 Features:
echo   🌡️ 实时温湿度模拟
echo   🌤️ 天气API集成
echo   🎯 智能温度建议
echo   🎛️ 完整空调控制
echo   🌐 多语言界面
echo   📱 响应式设计
echo.
echo 说明 Note:
echo   此演示展示完整的Web功能，无需QEMU硬件仿真
echo   嵌入式固件代码已完成并可在QEMU中运行
echo.
echo 按任意键结束演示...
pause >nul

echo.
echo [DEMO] 正在停止服务...
taskkill /f /im python.exe >nul 2>&1
echo [DEMO] 演示完成！
pause