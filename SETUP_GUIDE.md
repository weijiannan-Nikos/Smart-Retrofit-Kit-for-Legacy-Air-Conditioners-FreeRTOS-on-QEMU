# 智能空调控制网关 - 安装配置指南

## 🛠️ 工具链配置

### 1. 已安装工具路径
- **MSYS64**: `F:\msys64`
- **Git Bash**: `F:\Git\git-bash.exe`
- **QEMU**: `F:\QEMU`
- **ARM工具链**: `F:\GNU Arm Embedded Toolchain\10 2021.10`
- **FreeRTOS LTS**: `project2\FreeRTOS-LTS_sourcecode`

### 2. 环境验证
```bash
# 检查ARM工具链
F:\GNU Arm Embedded Toolchain\10 2021.10\bin\arm-none-eabi-gcc.exe --version

# 检查QEMU
F:\QEMU\qemu-system-arm.exe --version

# 检查Python
python --version
```

## 🚀 快速启动

### 方法1: 一键启动完整系统
```bash
cd scripts
run_complete.bat
```

### 方法2: 分步启动
```bash
# 1. 构建固件
cd scripts
build.bat

# 2. 启动Web桥接服务器
cd ../simulators
python web_bridge.py

# 3. 启动QEMU (新窗口)
cd ../scripts
run_qemu.bat

# 4. 打开Web界面
# 浏览器打开: web/index_simple.html
```

## 📡 系统架构

```
┌─────────────────┐    HTTP     ┌─────────────────┐    TCP      ┌─────────────────┐
│   Web Interface │ ◄────────► │  Bridge Server  │ ◄────────► │  QEMU Firmware  │
│  (Browser)      │   Port 8080 │   (Python)      │  Port 9999 │ (STM32+FreeRTOS)│
└─────────────────┘             └─────────────────┘             └─────────────────┘
        │                               │                               │
        │                               │                               │
        ▼                               ▼                               ▼
   用户控制界面                    数据转换和转发                    硬件控制逻辑
   - 温度设置                      - JSON ↔ 二进制                  - 传感器读取
   - 模式选择                      - HTTP ↔ TCP                     - 红外控制
   - 状态显示                      - 天气API集成                    - WiFi通信
```

## 🔧 核心功能

### QEMU固件 (STM32F407 + FreeRTOS)
- **传感器任务**: SHT30温湿度读取 (I2C)
- **红外任务**: NEC协议空调控制 (PWM + GPIO)
- **WiFi任务**: ESP8266通信 (UART)
- **控制任务**: 系统逻辑控制
- **桥接任务**: 与外部服务器通信

### Web桥接服务器 (Python)
- **Web API**: HTTP REST接口 (端口8080)
- **QEMU通信**: TCP Socket (端口9999)
- **天气API**: 实时天气数据获取
- **数据转换**: JSON ↔ 二进制格式

### Web界面 (HTML5 + JavaScript)
- **中文界面**: `web/index_simple.html`
- **英文界面**: `web/index_en.html`
- **实时控制**: 温度、模式、风速等
- **状态显示**: 传感器数据、天气信息

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
  "temp": 25.0,
  "humidity": 50.0,
  "target": 24,
  "power": 1,
  "mode": "cool"
}
```

### Bridge → QEMU (TCP)
```json
{"cmd":"power","val":1}
{"temp":25.0,"humidity":50.0,"target":24}
```

### QEMU内部 (FreeRTOS队列)
```c
typedef struct {
    char cmd[16];
    int value;
    uint32_t timestamp;
} Command_t;

typedef struct {
    float temperature;
    float humidity;
    uint32_t timestamp;
} SensorData_t;
```

## 🐛 故障排除

### 1. 构建失败
- 检查ARM工具链路径
- 确认FreeRTOS源码完整
- 查看编译错误信息

### 2. QEMU启动失败
- 检查QEMU安装路径
- 确认构建文件存在
- 查看QEMU错误输出

### 3. Web连接失败
- 确认桥接服务器运行 (端口8080)
- 检查防火墙设置
- 查看浏览器控制台错误

### 4. QEMU通信失败
- 确认QEMU固件运行
- 检查TCP端口9999占用
- 查看桥接服务器日志

## 📝 开发说明

### 添加新功能
1. **固件端**: 修改 `firmware/src/` 中的任务文件
2. **桥接端**: 更新 `simulators/web_bridge.py`
3. **Web端**: 修改 `web/` 中的HTML/JS文件

### 调试方法
1. **固件调试**: 查看QEMU控制台输出
2. **桥接调试**: 查看Python服务器日志
3. **Web调试**: 使用浏览器开发者工具

### 性能优化
1. **任务优先级**: 调整FreeRTOS任务优先级
2. **通信频率**: 优化数据发送间隔
3. **内存使用**: 监控堆栈使用情况