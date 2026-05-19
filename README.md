# 智能空调控制网关 (Intelligent Air Conditioner Control Gateway)

**作者**: jiannan WEI (MC555577)  
**课程**: Embedded System  
**版本**: v5.0 完整系统集成版  
**日期**: 2025-01-11

## 📋 项目概览

基于STM32F407的智能空调控制网关，支持温湿度监测、红外控制、WiFi通信和Web界面控制。

### 🔧 硬件平台
- **MCU**: STM32F407 (ARM Cortex-M4F @ 25MHz)
- **RTOS**: FreeRTOS LTS (202406版本)
- **仿真**: QEMU (qemu-system-arm, MPS2-AN386)
- **工具链**: GNU Arm Embedded Toolchain 10.3-2021.10
- **内存**: 256KB SRAM + 4MB Flash

### 📡 外设模块
- **SHT30** 温湿度传感器 (I2C, 0x44, 400kHz)
- **VS1838B** 红外接收器 (GPIO PA0, 38kHz)
- **IR333C** 红外发射器 (PWM PA1, 38kHz)
- **ESP8266** WiFi模块 (UART1, 115200 baud)

## 📁 项目结构

```
project2/
├── firmware/                    # STM32固件 (QEMU中运行)
│   ├── src/                    # 源代码
│   │   ├── main.c              # 主程序
│   │   ├── sensor_task.c       # SHT30传感器任务
│   │   ├── ir_task.c           # 红外控制任务
│   │   ├── wifi_task.c         # WiFi通信任务
│   │   ├── control_task.c      # 系统控制任务
│   │   ├── bridge_task.c       # 桥接任务
│   │   ├── bridge_comm.c       # 网桥通信
│   │   ├── system_hal.c        # 硬件抽象层
│   │   ├── startup.c           # 启动代码
│   │   ├── syscalls.c          # 系统调用
│   │   ├── freertos_stubs.c    # FreeRTOS存根
│   │   └── minimal_libc.c      # 最小C库
│   ├── inc/                    # 头文件
│   │   ├── system_config.h     # 系统配置
│   │   ├── system_hal.h        # 硬件抽象层
│   │   ├── FreeRTOS.h          # FreeRTOS主头文件
│   │   ├── task.h              # 任务头文件
│   │   ├── queue.h             # 队列头文件
│   │   ├── semphr.h            # 信号量头文件
│   │   └── [各任务头文件]
│   └── config/
│       └── FreeRTOSConfig.h    # FreeRTOS配置
├── simulators/                  # 外设模拟器 (独立运行)
│   ├── peripheral_simulator.py # 外设模拟器
│   ├── bridge_server.py        # 桥接服务器
│   └── web_bridge.py           # Web桥接服务器
├── web/                        # Web界面
│   ├── index_simple.html       # 中文界面
│   ├── index_en.html           # 英文界面
│   └── bridge_mock.py          # 模拟桥接器
├── scripts/                    # 构建和运行脚本
│   ├── build_elf.bat           # 完整构建脚本
│   ├── run_qemu_real.bat       # QEMU运行脚本
│   ├── test_compile.bat        # 测试编译脚本
│   └── linker.ld               # 链接脚本
├── build/                      # 构建输出目录
└── FreeRTOS-LTS_sourcecode/    # FreeRTOS LTS源码
```

## 🚀 快速开始

### 一键启动完整系统
```bash
cd scripts
start_working.bat
```

**启动过程**：
1. 启动QEMU桥接服务器 (端口8080/9999)
2. 编译STM32固件
3. 启动QEMU运行固件
4. 自动打开Web界面 (中英文版)

### 手动启动 (可选)
```bash
# 1. 启动桥接服务器
cd simulators
python qemu_bridge.py

# 2. 编译并运行QEMU
cd scripts
build_elf.bat
run_qemu_real.bat

# 3. 打开Web界面
# 浏览器打开 web/index_simple.html 和 web/index_en.html
```

## 🏗️ 系统架构

```
┌─────────────────┐    HTTP     ┌─────────────────┐   stdout/TCP ┌─────────────────┐
│   Web Interface │ ◄────────► │  Bridge Server  │ ◄─────────► │  QEMU Firmware  │
│   (Browser)     │   Port 8080 │   (Python)      │   Port 9999 │ (STM32+FreeRTOS)│
└─────────────────┘             └─────────────────┘              └─────────────────┘
                                         │                                │
                                    ┌────▼────┐                    ┌─────▼─────┐
                                    │ Weather │                    │  Sensors  │
                                    │   API   │                    │ IR Control│
                                    └─────────┘                    │ WiFi Comm │
                                                                   └───────────┘
```

### 🔄 数据流程
1. **Web界面** 发送控制命令 (HTTP POST)
2. **桥接服务器** 转换命令并发送到QEMU
3. **QEMU固件** 处理命令，控制传感器和红外
4. **固件** 输出JSON状态数据到stdout
5. **桥接服务器** 解析状态并提供给Web界面
6. **Web界面** 实时显示传感器数据和控制状态

## 💻 核心功能

### QEMU固件 (STM32F407 + FreeRTOS)
- **传感器任务**: SHT30温湿度读取 (I2C, 5Hz)
- **红外任务**: NEC协议空调控制 (PWM + GPIO)
- **WiFi任务**: ESP8266通信 (UART)
- **控制任务**: 系统逻辑控制和智能配置
- **桥接任务**: JSON状态输出到stdout

### QEMU桥接服务器 (Python)
- **Web API**: HTTP REST接口 (端口8080)
- **QEMU通信**: stdout解析 + TCP命令发送 (端口9999)
- **天气API**: Open-Meteo实时天气数据
- **智能控制**: 自动模式选择和温度建议

### Web界面 (HTML5 + JavaScript)
- **中文界面**: `web/index_simple.html` (智能配置功能)
- **英文界面**: `web/index_en.html` (完整控制面板)
- **实时控制**: 温度、模式、风速、定时等
- **状态显示**: 传感器数据、天气信息、命令反馈

## 📋 通信协议

### Web → Bridge (HTTP)
```json
POST /command
{
  "cmd": "power",
  "val": 1
}

GET /status
{
  "temp": 25.5,
  "humidity": 55.0,
  "target": 24,
  "power": 1,
  "mode": "auto",
  "fan": 2,
  "forecast_temp": 28,
  "suggestion": 26
}
```

### Bridge → QEMU (JSON)
```json
// 发送命令
{"cmd":"power","val":1}
{"cmd":"temp","val":24}
{"cmd":"smart_config","val":1}

// 接收状态 (从QEMU stdout)
{"temp":25.5,"humidity":55.0,"target":24,"power":1,"mode":"auto","fan":2}
```

## 🛠️ 开发环境

### 必需工具
- **ARM工具链**: `F:\GNU Arm Embedded Toolchain\10 2021.10`
- **QEMU**: `F:\QEMU`
- **Python**: 3.7+ (用于模拟器和桥接服务器)
- **Git Bash**: `F:\Git\git-bash.exe`
- **MSYS64**: `F:\msys64`

### 构建流程
1. **编译**: 使用ARM GCC编译所有源文件
2. **链接**: 生成ELF可执行文件
3. **运行**: 在QEMU MPS2-AN386平台执行

## 🔧 技术特点

- **RTOS**: FreeRTOS任务调度和同步
- **通信**: I2C、UART、PWM、JSON、TCP/HTTP
- **仿真**: QEMU ARM Cortex-M4完整仿真
- **模拟**: Python外设模拟器
- **界面**: 响应式Web界面
- **API**: RESTful Web服务

## 📝 功能特性

### 核心功能
- ✅ 实时温湿度监测 (SHT30, 5Hz更新)
- ✅ 红外空调控制 (NEC协议)
- ✅ WiFi远程控制 (ESP8266)
- ✅ 智能配置系统 (自动模式选择)
- ✅ 定时控制 (开机/关机/倒计时)

### Web界面功能
- ✅ 双语界面 (中文/英文)
- ✅ 实时数据可视化 (3秒更新)
- ✅ 命令反馈系统 (成功/失败提示)
- ✅ 天气API集成 (澳门实时天气)
- ✅ 智能温度建议
- ✅ 防抖控制 (避免高频命令)

## 🐛 故障排除

### 系统启动问题
```bash
# 检查所有组件
netstat -an | findstr 8080  # Web服务器
netstat -an | findstr 9999  # QEMU桥接
tasklist | findstr qemu     # QEMU进程
tasklist | findstr python   # 桥接服务器
```

### Web界面显示"连接断开"
1. 确认桥接服务器运行: `python qemu_bridge.py`
2. 确认QEMU固件运行: `run_qemu_real.bat`
3. 检查端口占用: `netstat -an | findstr 8080`

### 构建问题
```bash
# 重新构建固件
cd scripts
build_elf.bat

# 检查ELF文件
dir ..\build\firmware.elf
```

### 完全重启系统
```bash
# 停止所有服务
taskkill /f /im qemu-system-arm.exe
taskkill /f /im python.exe

# 重新启动
start_working.bat
```

## 📊 性能指标

- **CPU使用率**: < 50% (QEMU仿真)
- **内存使用**: < 32KB (堆栈+全局变量)
- **响应时间**: < 100ms (命令处理)
- **数据更新**: 5Hz (传感器读取)
- **网络延迟**: < 50ms (本地通信)

## 🎯 项目亮点

1. **完整的嵌入式系统**: 从硬件抽象到应用层
2. **现代化架构**: 微服务化设计，模块解耦
3. **跨平台兼容**: QEMU仿真 + 真实硬件
4. **实时性保证**: FreeRTOS任务调度
5. **用户友好**: 直观的Web界面
6. **可扩展性**: 模块化设计，易于扩展

---

**联系方式**: jiannan WEI (MC555577)  
**项目地址**: `F:\University of Macau master course\embeded system\SENSOR_project\project2`