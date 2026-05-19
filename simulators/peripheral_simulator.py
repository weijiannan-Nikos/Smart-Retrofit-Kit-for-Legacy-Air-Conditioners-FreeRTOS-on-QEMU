#!/usr/bin/env python3
"""
外设模拟器 - 模拟SHT30、ESP8266、红外等外设
Peripheral Simulator - Simulates SHT30, ESP8266, IR devices
Author: jiannan WEI (MC555577)
"""

import threading
import time
import json
import socket
import random
from http.server import HTTPServer, BaseHTTPRequestHandler

class PeripheralSimulator:
    def __init__(self):
        self.sensor_data = {
            'temperature': 25.0,
            'humidity': 50.0,
            'timestamp': time.time()
        }
        
        self.ac_status = {
            'power': 0,
            'target_temp': 24,
            'mode': 'auto',
            'fan_speed': 2,
            'swing': 0
        }
        
        self.running = True
        
    def simulate_sht30(self):
        """模拟SHT30温湿度传感器"""
        print("[SHT30] Sensor simulation started")
        
        while self.running:
            # 模拟温湿度变化
            self.sensor_data['temperature'] += random.uniform(-0.5, 0.5)
            self.sensor_data['humidity'] += random.uniform(-2.0, 2.0)
            
            # 限制范围
            self.sensor_data['temperature'] = max(15, min(35, self.sensor_data['temperature']))
            self.sensor_data['humidity'] = max(30, min(80, self.sensor_data['humidity']))
            self.sensor_data['timestamp'] = time.time()
            
            print(f"[SHT30] T={self.sensor_data['temperature']:.1f}°C, H={self.sensor_data['humidity']:.1f}%")
            time.sleep(2)
    
    def simulate_esp8266(self):
        """模拟ESP8266 WiFi模块"""
        print("[ESP8266] WiFi simulation started")
        
        # 模拟WiFi连接过程
        time.sleep(1)
        print("[ESP8266] AT+RST")
        time.sleep(2)
        print("[ESP8266] ready")
        
        time.sleep(1)
        print("[ESP8266] AT+CWMODE=1")
        print("[ESP8266] OK")
        
        time.sleep(1)
        print("[ESP8266] AT+CWJAP=\"TestAP\",\"password\"")
        time.sleep(3)
        print("[ESP8266] WIFI CONNECTED")
        print("[ESP8266] WIFI GOT IP")
        print("[ESP8266] OK")
        
        while self.running:
            # 模拟接收网络命令
            time.sleep(5)
            if random.random() < 0.3:  # 30%概率接收命令
                commands = ['power', 'temp', 'mode', 'fan']
                cmd = random.choice(commands)
                value = random.randint(18, 30) if cmd == 'temp' else random.randint(0, 3)
                print(f"[ESP8266] Received: {cmd}={value}")
    
    def simulate_ir_receiver(self):
        """模拟红外接收器"""
        print("[IR_RX] Infrared receiver simulation started")
        
        ir_codes = {
            0x02FD00FF: "POWER_ON",
            0x02FD807F: "POWER_OFF", 
            0x02FD40BF: "TEMP_UP",
            0x02FD20DF: "TEMP_DOWN",
            0x02FD10EF: "MODE_AUTO",
            0x02FD30CF: "MODE_COOL"
        }
        
        while self.running:
            time.sleep(random.uniform(10, 30))  # 随机接收红外信号
            if random.random() < 0.4:  # 40%概率
                code = random.choice(list(ir_codes.keys()))
                print(f"[IR_RX] Received: 0x{code:08X} ({ir_codes[code]})")
    
    def simulate_ir_transmitter(self, command):
        """模拟红外发射器"""
        ir_codes = {
            'power_on': 0x02FD00FF,
            'power_off': 0x02FD807F,
            'temp_up': 0x02FD40BF,
            'temp_down': 0x02FD20DF,
            'mode_auto': 0x02FD10EF,
            'mode_cool': 0x02FD30CF
        }
        
        if command in ir_codes:
            code = ir_codes[command]
            print(f"[IR_TX] Transmitting: 0x{code:08X} ({command.upper()})")
            
            # 模拟空调响应
            if command == 'power_on':
                self.ac_status['power'] = 1
            elif command == 'power_off':
                self.ac_status['power'] = 0
            elif command == 'temp_up':
                self.ac_status['target_temp'] = min(30, self.ac_status['target_temp'] + 1)
            elif command == 'temp_down':
                self.ac_status['target_temp'] = max(16, self.ac_status['target_temp'] - 1)
            
            print(f"[AC] Status updated: {self.ac_status}")
    
    def get_status(self):
        """获取当前状态"""
        return {
            'sensor': self.sensor_data,
            'ac': self.ac_status,
            'timestamp': time.time()
        }
    
    def process_command(self, cmd, value):
        """处理控制命令"""
        print(f"[SIMULATOR] Processing command: {cmd}={value}")
        
        if cmd == 'power':
            self.simulate_ir_transmitter('power_on' if value else 'power_off')
        elif cmd == 'temp':
            current = self.ac_status['target_temp']
            if value > current:
                for _ in range(value - current):
                    self.simulate_ir_transmitter('temp_up')
            elif value < current:
                for _ in range(current - value):
                    self.simulate_ir_transmitter('temp_down')
        elif cmd == 'mode':
            if value == 'auto':
                self.simulate_ir_transmitter('mode_auto')
            elif value == 'cool':
                self.simulate_ir_transmitter('mode_cool')
    
    def start(self):
        """启动所有模拟器"""
        print("=" * 60)
        print("  外设模拟器启动 (Peripheral Simulator)")
        print("=" * 60)
        
        # 启动各个模拟线程
        threading.Thread(target=self.simulate_sht30, daemon=True).start()
        threading.Thread(target=self.simulate_esp8266, daemon=True).start()
        threading.Thread(target=self.simulate_ir_receiver, daemon=True).start()
        
        print("\n[SIMULATOR] All peripherals started")
        print("[SIMULATOR] Listening for QEMU firmware...")
        print("[SIMULATOR] Press Ctrl+C to stop\n")

# HTTP服务器用于与Web界面通信
class SimulatorHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/status':
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            
            status = simulator.get_status()
            self.wfile.write(json.dumps(status).encode())
        else:
            self.send_response(404)
            self.end_headers()
    
    def do_POST(self):
        if self.path == '/command':
            length = int(self.headers['Content-Length'])
            data = self.rfile.read(length)
            command = json.loads(data.decode())
            
            simulator.process_command(command['cmd'], command['val'])
            
            self.send_response(200)
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            self.wfile.write(b'OK')
    
    def log_message(self, format, *args):
        pass

if __name__ == '__main__':
    simulator = PeripheralSimulator()
    simulator.start()
    
    # 启动HTTP服务器
    print("[HTTP] Starting web server on http://localhost:8081")
    server = HTTPServer(('localhost', 8081), SimulatorHandler)
    
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n[SIMULATOR] Shutting down...")
        simulator.running = False