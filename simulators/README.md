# 外设模拟器 (Peripheral Simulators)

本目录包含所有外设的模拟器程序，用于在QEMU外部模拟硬件外设的行为。

## 文件说明

### peripheral_simulator.py
- **功能**: 模拟所有外设（SHT30、ESP8266、红外收发器）
- **端口**: 8081
- **用途**: 独立运行，提供外设数据模拟

### bridge_server.py  
- **功能**: 桥接QEMU固件和Web界面
- **端口**: 8080 (Web API), 9999 (QEMU通信)
- **用途**: 数据转发和协议转换

## 使用方法

### 1. 启动外设模拟器
```bash
python peripheral_simulator.py
```

### 2. 启动桥接服务器
```bash
python bridge_server.py
```

### 3. 或使用系统启动脚本
```bash
cd ../scripts
start_system.bat
```

## 通信协议

### QEMU ↔ Bridge Server
- **协议**: TCP Socket (端口9999)
- **格式**: JSON
- **数据**: 传感器数据、控制命令

### Bridge Server ↔ Web Interface  
- **协议**: HTTP REST API (端口8080)
- **端点**: 
  - GET /status - 获取状态
  - POST /command - 发送命令

### 外设模拟器 ↔ Web Interface
- **协议**: HTTP REST API (端口8081) 
- **用途**: 直接模拟模式（无QEMU）

## 数据格式

### 传感器数据
```json
{
  "temp": 25.0,
  "humidity": 50.0,
  "timestamp": 1641900000
}
```

### 控制命令
```json
{
  "cmd": "power",
  "val": 1
}
```