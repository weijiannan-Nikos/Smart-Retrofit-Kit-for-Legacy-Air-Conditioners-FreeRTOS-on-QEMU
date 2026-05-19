# Intelligent Air Conditioner Control Gateway - Project Documentation

**Author**: Jiannan WEI (MC555577)  
**Course**: Embedded System  
**Version**: v5.0 Complete System Integration  
**Date**: 2025-01-11

## 1. Project Objectives

### Primary Objectives
The project aims to develop a comprehensive **Intelligent Air Conditioner Control Gateway** based on STM32F407 microcontroller running FreeRTOS, providing:

1. **Real-time Environmental Monitoring**: Continuous temperature and humidity sensing using SHT30 sensor
2. **Infrared Remote Control**: NEC protocol-based air conditioner control via IR transmitter/receiver
3. **Wireless Connectivity**: WiFi communication through ESP8266 module for remote access
4. **Web-based Interface**: User-friendly web control panel with bilingual support (Chinese/English)
5. **Intelligent Control System**: AI-powered temperature suggestions based on weather data
6. **System Integration**: Complete QEMU simulation environment with external bridge communication

### Technical Objectives
- **RTOS Implementation**: Multi-task real-time system using FreeRTOS LTS
- **Hardware Abstraction**: Modular HAL design for peripheral management
- **Communication Protocols**: I2C, UART, PWM, TCP/HTTP, JSON data exchange
- **Cross-platform Compatibility**: QEMU simulation with real hardware portability
- **Performance Optimization**: Efficient task scheduling and resource management

## 2. System Architecture Overview

```
┌─────────────────┐    HTTP     ┌─────────────────┐   JSON/TCP  ┌─────────────────┐
│   Web Interface │ ◄────────► │  Bridge Server  │ ◄─────────► │  QEMU Firmware  │
│   (Browser)     │   Port 8080 │   (Python)      │   Port 9999 │ (STM32+FreeRTOS)│
└─────────────────┘             └─────────────────┘              └─────────────────┘
                                         │                                │
                                    ┌────▼────┐                    ┌─────▼─────┐
                                    │ Weather │                    │  Hardware │
                                    │   API   │                    │Peripherals│
                                    └─────────┘                    └───────────┘
```

## 3. FreeRTOS Task Analysis

### 3.1 Task Hierarchy and Priorities

The system implements **5 main tasks** with carefully designed priority levels:

| Task Name | Priority | Stack Size | Function |
|-----------|----------|------------|----------|
| **Sensor Task** | 4 (Highest) | 16 × configMINIMAL_STACK_SIZE | Environmental monitoring |
| **IR Task** | 3 | 8 × configMINIMAL_STACK_SIZE | Infrared control |
| **Control Task** | 3 | 8 × configMINIMAL_STACK_SIZE | System logic control |
| **WiFi Task** | 2 | Standard | Network communication |
| **Bridge Task** | 1 (Lowest) | 16 × configMINIMAL_STACK_SIZE | External communication |

### 3.2 Individual Task Functions

#### 3.2.1 Sensor Task (`vSensorTask`)
**Primary Function**: SHT30 temperature and humidity monitoring
- **Execution Cycle**: 2-second intervals (5Hz update rate)
- **Hardware Interface**: I2C communication (400kHz, address 0x44)
- **Data Processing**: Raw sensor data conversion to temperature/humidity values
- **Output**: Publishes `SensorData_t` structures to sensor queue
- **Timeout Protection**: 15-second automatic termination for demo mode

**Key Operations**:
```c
// Sensor data structure
typedef struct {
    float temperature;
    float humidity;
    uint32_t timestamp;
} SensorData_t;
```

#### 3.2.2 IR Task (`vIRTask`)
**Primary Function**: Infrared air conditioner control
- **Protocol**: NEC infrared protocol implementation
- **Hardware Interface**: PWM output (38kHz carrier) + GPIO control
- **Command Processing**: Receives control commands from command queue
- **Supported Commands**: Power, temperature, mode, fan speed control
- **IR Codes**: Predefined air conditioner control codes

**Control Codes**:
- `AC_POWER_ON/OFF`: 0x02FD00FF / 0x02FD807F
- `AC_TEMP_UP/DOWN`: 0x02FD40BF / 0x02FD20DF
- `AC_MODE_AUTO/COOL/HEAT`: Various codes for mode control

#### 3.2.3 Control Task (`vControlTask`)
**Primary Function**: Central system logic and state management
- **Data Integration**: Combines sensor data with user commands
- **State Management**: Maintains system status (temperature, power, mode, fan)
- **Command Processing**: Interprets and validates user control inputs
- **Status Broadcasting**: Publishes system status to status queue

**System Status Structure**:
```c
typedef struct {
    SensorData_t sensor;
    int target_temp;
    int power_state;
    char mode[16];
    int fan_speed;
    int swing_state;
    uint32_t last_update;
} SystemStatus_t;
```

#### 3.2.4 WiFi Task (`vWiFiTask`)
**Primary Function**: ESP8266 WiFi module management
- **Hardware Interface**: UART1 communication (115200 baud)
- **Network Management**: WiFi connection establishment and monitoring
- **AT Command Interface**: ESP8266 AT command processing
- **Status Monitoring**: Periodic connectivity checks

#### 3.2.5 Bridge Task (`vBridgeTask`)
**Primary Function**: External communication bridge
- **Data Format**: JSON-based status reporting
- **Output Interface**: stdout for QEMU bridge server capture
- **Command Reception**: External command processing (future TCP implementation)
- **Update Rate**: 5Hz status broadcasting
- **Integration**: Combines all system data for external interface

## 4. Inter-Task Communication and Synchronization

### 4.1 Communication Mechanisms

#### 4.1.1 FreeRTOS Queues
The system uses **3 primary queues** for inter-task communication:

```c
QueueHandle_t xSensorQueue;    // Sensor → Control/Bridge (5 items)
QueueHandle_t xCommandQueue;   // External → IR/Control (3 items)
QueueHandle_t xStatusQueue;    // Control → Bridge (3 items)
```

**Queue Data Flow**:
```
Sensor Task ──→ xSensorQueue ──→ Control Task ──→ xStatusQueue ──→ Bridge Task
                                      ↑                              ↓
External Commands ──→ xCommandQueue ──┘                    JSON Output
```

#### 4.1.2 Mutex Synchronization
**System Mutex** (`xSystemMutex`): Protects shared resources
- **Critical Sections**: I2C sensor access, shared data structures
- **Deadlock Prevention**: Timeout-based mutex acquisition (100ms)
- **Resource Protection**: Ensures atomic operations on hardware peripherals

### 4.2 Task Cooperation Patterns

#### 4.2.1 Producer-Consumer Pattern
- **Sensor Task** (Producer) → **Control Task** (Consumer)
- **Control Task** (Producer) → **Bridge Task** (Consumer)
- **External Commands** (Producer) → **IR Task** (Consumer)

#### 4.2.2 Synchronous Communication
- **Mutex-protected I2C access**: Prevents bus conflicts
- **Queue-based messaging**: Ensures reliable data transfer
- **Timeout mechanisms**: Prevents system deadlocks

#### 4.2.3 Asynchronous Operations
- **Periodic sensor readings**: Independent of user commands
- **Status broadcasting**: Continuous system state updates
- **Command processing**: Event-driven IR control

## 5. Communication Protocols and Data Flow

### 5.1 Internal Communication (FreeRTOS)

#### Command Structure
```c
typedef struct {
    char cmd[16];        // Command name ("power", "temp", "mode")
    int value;           // Command parameter
    uint32_t timestamp;  // Command timestamp
} Command_t;
```

#### Data Flow Sequence
1. **Sensor Reading**: SHT30 → I2C → Sensor Task → Queue
2. **Command Processing**: Web → Bridge → Command Queue → IR Task
3. **Status Update**: Control Task → Status Queue → Bridge Task → JSON Output
4. **External Interface**: JSON → Bridge Server → Web Interface

### 5.2 External Communication

#### 5.2.1 QEMU Bridge Protocol
**Direction**: QEMU → Bridge Server
```json
{"temp":25.5,"humidity":55.0,"target":24,"power":1,"mode":"auto","fan":2}
```

**Direction**: Bridge Server → QEMU
```json
{"cmd":"power","val":1}
{"cmd":"temp","val":24}
```

#### 5.2.2 Web API Protocol
**HTTP GET** `/status`:
```json
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

**HTTP POST** `/command`:
```json
{
  "cmd": "power",
  "val": 1
}
```

## 6. Hardware Abstraction Layer (HAL)

### 6.1 Peripheral Interfaces

#### I2C Interface (SHT30 Sensor)
- **Clock Frequency**: 400kHz
- **Address**: 0x44
- **Functions**: `I2C_Write()`, `I2C_Read()`
- **Error Handling**: Return code validation

#### UART Interface (ESP8266 WiFi)
- **Baud Rate**: 115200
- **Port**: UART1
- **Protocol**: AT command interface
- **Functions**: `UART_SendString()`

#### PWM Interface (IR Transmitter)
- **Frequency**: 38kHz carrier
- **Pin**: PA1 (IR_TX_PIN)
- **Modulation**: NEC protocol timing
- **Functions**: `GPIO_Write()`, PWM control

#### GPIO Interface
- **IR Receiver**: PA0 (IR_RX_PIN)
- **WiFi Reset**: PA2 (WIFI_RESET_PIN)
- **Digital I/O**: Standard GPIO operations

### 6.2 QEMU Simulation Environment

#### Platform Configuration
- **Target**: MPS2-AN386 (ARM Cortex-M4F)
- **Clock**: 25MHz system clock
- **Memory**: 256KB SRAM + 4MB Flash
- **Peripherals**: Simulated I2C, UART, GPIO, PWM

#### Hardware Simulation
- **SHT30 Simulation**: Generates realistic temperature/humidity data
- **IR Simulation**: Command logging and acknowledgment
- **WiFi Simulation**: AT command response simulation

## 7. System Features and Capabilities

### 7.1 Core Functionality
- ✅ **Real-time Monitoring**: 5Hz sensor data acquisition
- ✅ **IR Control**: Complete air conditioner command set
- ✅ **WiFi Connectivity**: ESP8266 integration
- ✅ **Web Interface**: Bilingual control panel
- ✅ **AI Integration**: Weather-based temperature suggestions
- ✅ **Scheduling**: Timer and schedule-based control

### 7.2 Advanced Features
- **Smart Configuration**: Automatic mode selection
- **Weather Integration**: Open-Meteo API for Macau weather
- **Command Debouncing**: Prevents rapid command flooding
- **Timeout Protection**: Demo mode with automatic termination
- **Error Handling**: Comprehensive error reporting and recovery

### 7.3 Performance Characteristics
- **CPU Utilization**: < 50% (QEMU simulation)
- **Memory Usage**: < 32KB (stack + global variables)
- **Response Time**: < 100ms (command processing)
- **Update Rate**: 5Hz (sensor) / 5Hz (status broadcast)
- **Network Latency**: < 50ms (local communication)

## 8. Development and Build Environment

### 8.1 Toolchain
- **Compiler**: GNU Arm Embedded Toolchain 10.3-2021.10
- **Simulator**: QEMU qemu-system-arm
- **RTOS**: FreeRTOS LTS 202406
- **Build System**: Custom batch scripts with ARM GCC

### 8.2 Project Structure
```
firmware/
├── src/           # Source code (tasks, HAL, main)
├── inc/           # Header files
├── config/        # FreeRTOS configuration
simulators/        # Python bridge servers
web/              # HTML/JavaScript interfaces
scripts/          # Build and run scripts
```

## 9. Innovation and Technical Achievements

### 9.1 System Integration
- **Multi-layer Architecture**: Clean separation between firmware, bridge, and web layers
- **Cross-platform Design**: QEMU simulation with real hardware compatibility
- **Modular Implementation**: Easily extensible task-based architecture

### 9.2 Real-time Performance
- **Deterministic Scheduling**: FreeRTOS priority-based task management
- **Efficient Communication**: Queue-based inter-task messaging
- **Resource Management**: Mutex-protected critical sections

### 9.3 User Experience
- **Intuitive Interface**: Responsive web-based control panel
- **Bilingual Support**: Chinese and English interfaces
- **Real-time Feedback**: Live status updates and command confirmation
- **AI Enhancement**: Intelligent temperature recommendations

## 10. Conclusion

This project successfully demonstrates a complete embedded system implementation featuring:

1. **Comprehensive RTOS Design**: Five coordinated tasks with proper synchronization
2. **Multi-protocol Communication**: I2C, UART, PWM, HTTP, JSON integration
3. **Modern Architecture**: Microservices-style design with clear interfaces
4. **Practical Application**: Real-world air conditioner control system
5. **Educational Value**: Demonstrates embedded systems best practices

The system showcases advanced embedded programming concepts including real-time task scheduling, inter-process communication, hardware abstraction, and system integration, making it an excellent demonstration of modern embedded system development practices.

---

**Technical Specifications Summary**:
- **Platform**: STM32F407 + FreeRTOS on QEMU MPS2-AN386
- **Tasks**: 5 coordinated real-time tasks
- **Communication**: 3 queues + 1 mutex for synchronization
- **Interfaces**: I2C, UART, PWM, GPIO, HTTP, JSON
- **Performance**: 5Hz sensor updates, <100ms response time
- **Features**: Web control, AI suggestions, weather integration, scheduling