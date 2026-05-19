#!/usr/bin/env python3
"""
独立Web桥接服务器 - 无需QEMU连接
Standalone Web Bridge Server - No QEMU connection required
Author: jiannan WEI (MC555577)
"""

import json
import time
import urllib.request
from http.server import HTTPServer, BaseHTTPRequestHandler
import threading

class StandaloneWebBridge:
    def __init__(self):
        # 系统状态
        self.status = {
            'temp': 25.0,
            'humidity': 50.0,
            'target': 24,
            'power': 0,
            'mode': 'auto',
            'fan': 2,
            'swing': 0,
            'direction': 'auto',
            'forecast_temp': 25,
            'forecast_hum': 50,
            'condition': 'Unknown',
            'suggestion': 24,
            'last_cmd': '',
            'cmd_ack': 1,
            'last_update': time.time()
        }
        
        self.running = True
        self.simulate_sensor_data()
    
    def simulate_sensor_data(self):
        """模拟传感器数据变化"""
        def update_sensors():
            while self.running:
                # 模拟温度变化
                if self.status['power']:
                    target = self.status['target']
                    current = self.status['temp']
                    
                    if self.status['mode'] == 'cool' and current > target:
                        self.status['temp'] = max(target, current - 0.1)
                    elif self.status['mode'] == 'heat' and current < target:
                        self.status['temp'] = min(target, current + 0.1)
                    elif self.status['mode'] == 'auto':
                        if current > target:
                            self.status['temp'] = max(target, current - 0.1)
                        elif current < target:
                            self.status['temp'] = min(target, current + 0.1)
                
                # 模拟湿度变化
                if self.status['mode'] == 'dry':
                    self.status['humidity'] = max(30, self.status['humidity'] - 0.2)
                else:
                    # 轻微波动
                    import random
                    self.status['humidity'] += random.uniform(-0.5, 0.5)
                    self.status['humidity'] = max(30, min(80, self.status['humidity']))
                
                self.status['last_update'] = time.time()
                time.sleep(2)  # 2秒更新一次
        
        threading.Thread(target=update_sensors, daemon=True).start()
    
    def fetch_weather(self):
        """获取天气数据"""
        while self.running:
            try:
                url = 'https://api.open-meteo.com/v1/forecast?latitude=22.20&longitude=113.54&current_weather=true&hourly=relativehumidity_2m'
                with urllib.request.urlopen(url, timeout=10) as response:
                    data = json.loads(response.read().decode())
                    
                    self.status['forecast_temp'] = int(data['current_weather']['temperature'])
                    self.status['forecast_hum'] = int(data['hourly']['relativehumidity_2m'][0])
                    
                    # 天气状况
                    temp = self.status['forecast_temp']
                    if temp > 30:
                        self.status['condition'] = 'Hot'
                    elif temp > 25:
                        self.status['condition'] = 'Warm'
                    elif temp > 20:
                        self.status['condition'] = 'Mild'
                    else:
                        self.status['condition'] = 'Cool'
                    
                    # 智能建议
                    outdoor_temp = self.status['forecast_temp']
                    if outdoor_temp > 28:
                        self.status['suggestion'] = 26
                    elif outdoor_temp < 18:
                        self.status['suggestion'] = 22
                    else:
                        self.status['suggestion'] = 24
                    
                    print(f"[WEATHER] {self.status['forecast_temp']}°C, "
                          f"{self.status['forecast_hum']}%, {self.status['condition']}")
            
            except Exception as e:
                print(f"[WEATHER] API error: {e}")
            
            time.sleep(300)  # 5分钟更新一次
    
    def process_command(self, command):
        """处理控制命令"""
        cmd = command.get('cmd')
        val = command.get('val')
        
        print(f"[BRIDGE] Processing command: {cmd} = {val}")
        
        if cmd == 'power':
            self.status['power'] = int(val)
        elif cmd == 'temp':
            self.status['target'] = int(val)
        elif cmd == 'mode':
            self.status['mode'] = str(val)
        elif cmd == 'fan':
            self.status['fan'] = int(val)
        elif cmd == 'direction':
            self.status['direction'] = str(val)
        elif cmd == 'apply_ai':
            self.status['target'] = self.status['suggestion']
        
        self.status['last_cmd'] = cmd
        self.status['cmd_ack'] = 1
        
        return True
    
    def get_full_status(self):
        """获取完整状态"""
        status = self.status.copy()
        status['feedback'] = {
            'status': 'success',
            'cmd': self.status['last_cmd'],
            'reason': ''
        }
        return status

class WebHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/status':
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.send_header('Cache-Control', 'no-cache')
            self.end_headers()
            
            status = bridge.get_full_status()
            self.wfile.write(json.dumps(status).encode())
        else:
            self.send_response(404)
            self.end_headers()
    
    def do_POST(self):
        if self.path == '/command':
            length = int(self.headers['Content-Length'])
            data = self.rfile.read(length)
            command = json.loads(data.decode())
            
            print(f"[WEB] Command received: {command}")
            
            # 处理命令
            success = bridge.process_command(command)
            
            self.send_response(200 if success else 500)
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            self.wfile.write(b'OK' if success else b'FAILED')
        else:
            self.send_response(404)
            self.end_headers()
    
    def do_OPTIONS(self):
        self.send_response(200)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        self.end_headers()
    
    def log_message(self, format, *args):
        pass

if __name__ == '__main__':
    print("=" * 60)
    print("  独立Web桥接服务器 (Standalone Web Bridge)")
    print("  智能空调控制演示 - 无需QEMU")
    print("=" * 60)
    
    bridge = StandaloneWebBridge()
    
    # 启动天气获取线程
    threading.Thread(target=bridge.fetch_weather, daemon=True).start()
    
    # 启动Web服务器
    print("[BRIDGE] Web API server on http://localhost:8080")
    print("[BRIDGE] Simulating AC control without QEMU")
    print("[BRIDGE] Web interface: web/index_simple.html")
    print()
    print("Features:")
    print("✅ Real-time weather API")
    print("✅ Smart temperature control")
    print("✅ Simulated sensor data")
    print("✅ Full AC control interface")
    print()
    
    server = HTTPServer(('localhost', 8080), WebHandler)
    
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n[BRIDGE] Shutting down...")
        bridge.running = False