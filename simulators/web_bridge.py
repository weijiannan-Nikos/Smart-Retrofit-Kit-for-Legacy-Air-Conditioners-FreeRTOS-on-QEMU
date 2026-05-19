#!/usr/bin/env python3
"""
Web桥接服务器 - 连接Web界面与QEMU固件
Web Bridge Server - Connects Web interface with QEMU firmware
Author: jiannan WEI (MC555577)
"""

import threading
import json
import time
import socket
import urllib.request
from http.server import HTTPServer, BaseHTTPRequestHandler

class WebBridge:
    def __init__(self):
        self.qemu_socket = None
        self.qemu_connected = False
        
        # 系统状态
        self.status = {
            'temp': 25.0,
            'humidity': 50.0,
            'target': 24,
            'power': 0,
            'mode': 'auto',
            'fan': 2,
            'swing': 0,
            'forecast_temp': 25,
            'forecast_hum': 50,
            'condition': 'Unknown',
            'suggestion': 24,
            'last_cmd': '',
            'cmd_ack': 1,
            'last_update': time.time()
        }
        
        self.running = True
    
    def start_qemu_server(self):
        """启动QEMU通信服务器"""
        print("[BRIDGE] Starting QEMU server on port 9999...")
        
        try:
            server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server_socket.bind(('localhost', 9999))
            server_socket.listen(1)
            server_socket.settimeout(1.0)
            
            while self.running:
                try:
                    client_socket, addr = server_socket.accept()
                    print(f"[BRIDGE] QEMU connected from {addr}")
                    self.qemu_socket = client_socket
                    self.qemu_connected = True
                    
                    # 处理QEMU连接
                    self.handle_qemu_connection(client_socket)
                    
                except socket.timeout:
                    continue
                except Exception as e:
                    if self.running:
                        print(f"[BRIDGE] QEMU server error: {e}")
        
        except Exception as e:
            print(f"[BRIDGE] Failed to start QEMU server: {e}")
    
    def handle_qemu_connection(self, client_socket):
        """处理QEMU连接"""
        buffer = ""
        
        while self.running and self.qemu_connected:
            try:
                data = client_socket.recv(1024).decode('utf-8')
                if not data:
                    break
                
                buffer += data
                
                # 处理完整的JSON消息
                while '\n' in buffer:
                    line, buffer = buffer.split('\n', 1)
                    if line.strip():
                        try:
                            qemu_data = json.loads(line.strip())
                            self.update_status_from_qemu(qemu_data)
                        except json.JSONDecodeError:
                            print(f"[BRIDGE] Invalid JSON from QEMU: {line}")
            
            except socket.timeout:
                continue
            except Exception as e:
                print(f"[BRIDGE] Error handling QEMU: {e}")
                break
        
        self.qemu_connected = False
        self.qemu_socket = None
        client_socket.close()
        print("[BRIDGE] QEMU disconnected")
    
    def update_status_from_qemu(self, qemu_data):
        """更新来自QEMU的状态数据"""
        if 'temp' in qemu_data:
            self.status['temp'] = qemu_data['temp']
        if 'humidity' in qemu_data:
            self.status['humidity'] = qemu_data['humidity']
        if 'target' in qemu_data:
            self.status['target'] = qemu_data['target']
        if 'power' in qemu_data:
            self.status['power'] = qemu_data['power']
        if 'mode' in qemu_data:
            self.status['mode'] = qemu_data['mode']
        if 'fan' in qemu_data:
            self.status['fan'] = qemu_data['fan']
        
        self.status['last_update'] = time.time()
        
        print(f"[BRIDGE] Status from QEMU: T={self.status['temp']:.1f}°C, "
              f"H={self.status['humidity']:.1f}%, Target={self.status['target']}°C")
    
    def send_command_to_qemu(self, command):
        """发送命令到QEMU"""
        if not self.qemu_connected or not self.qemu_socket:
            print("[BRIDGE] QEMU not connected")
            return False
        
        try:
            cmd_json = json.dumps(command) + '\n'
            self.qemu_socket.send(cmd_json.encode('utf-8'))
            
            print(f"[BRIDGE] Command sent to QEMU: {command}")
            
            # 更新命令反馈
            self.status['last_cmd'] = command.get('cmd', '')
            self.status['cmd_ack'] = 1
            
            return True
        
        except Exception as e:
            print(f"[BRIDGE] Failed to send command to QEMU: {e}")
            self.status['cmd_ack'] = 0
            return False
    
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
    
    def get_full_status(self):
        """获取完整状态"""
        status = self.status.copy()
        status['feedback'] = {
            'status': 'success' if self.status['cmd_ack'] else 'failed',
            'cmd': self.status['last_cmd'],
            'reason': '' if self.qemu_connected else 'QEMU not connected'
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
            
            # 发送到QEMU
            success = bridge.send_command_to_qemu(command)
            
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
    print("  Web桥接服务器 (Web Bridge Server)")
    print("  连接Web界面与QEMU固件")
    print("=" * 60)
    
    bridge = WebBridge()
    
    # 启动QEMU服务器线程
    threading.Thread(target=bridge.start_qemu_server, daemon=True).start()
    
    # 启动天气获取线程
    threading.Thread(target=bridge.fetch_weather, daemon=True).start()
    
    # 启动Web服务器
    print("[BRIDGE] Web API server on http://localhost:8080")
    print("[BRIDGE] QEMU server on port 9999")
    print("[BRIDGE] Web interface: web/index_simple.html")
    print()
    print("Waiting for connections...")
    print("1. Start QEMU firmware")
    print("2. Open web interface")
    print()
    
    server = HTTPServer(('localhost', 8080), WebHandler)
    
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n[BRIDGE] Shutting down...")
        bridge.running = False