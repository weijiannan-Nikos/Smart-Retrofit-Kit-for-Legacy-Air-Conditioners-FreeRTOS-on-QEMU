#!/usr/bin/env python3
"""
桥接服务器 - 连接QEMU固件和Web界面
Bridge Server - Connects QEMU firmware with Web interface
Author: jiannan WEI (MC555577)
"""

import threading
import json
import time
import socket
import urllib.request
from http.server import HTTPServer, BaseHTTPRequestHandler

class BridgeServer:
    def __init__(self):
        self.firmware_data = {
            'temp': 25.0,
            'humidity': 50.0,
            'target': 24,
            'power': 0,
            'mode': 'auto',
            'fan': 2,
            'swing': 0,
            'last_update': time.time()
        }
        
        self.weather_data = {
            'forecast_temp': 25,
            'forecast_hum': 50,
            'condition': 'Unknown'
        }
        
        self.command_feedback = {
            'status': 'idle',
            'cmd': '',
            'reason': ''
        }
        
        self.running = True
    
    def listen_to_qemu(self):
        """监听QEMU固件数据"""
        print("[BRIDGE] Listening for QEMU firmware on port 9999...")
        
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
                    
                    while self.running:
                        try:
                            data = client_socket.recv(1024).decode('utf-8')
                            if not data:
                                break
                            
                            # 解析QEMU发送的JSON数据
                            try:
                                qemu_data = json.loads(data)
                                self.firmware_data.update(qemu_data)
                                self.firmware_data['last_update'] = time.time()
                                print(f"[BRIDGE] From QEMU: {qemu_data}")
                            except json.JSONDecodeError:
                                print(f"[BRIDGE] Invalid JSON from QEMU: {data}")
                        
                        except socket.timeout:
                            continue
                        except Exception as e:
                            print(f"[BRIDGE] Error receiving from QEMU: {e}")
                            break
                    
                    client_socket.close()
                    print("[BRIDGE] QEMU disconnected")
                
                except socket.timeout:
                    continue
                except Exception as e:
                    if self.running:
                        print(f"[BRIDGE] Socket error: {e}")
                        time.sleep(1)
        
        except Exception as e:
            print(f"[BRIDGE] Failed to start QEMU listener: {e}")
    
    def send_to_qemu(self, command):
        """发送命令到QEMU固件"""
        try:
            client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            client_socket.settimeout(2.0)
            client_socket.connect(('localhost', 9999))
            
            cmd_json = json.dumps(command)
            client_socket.send(cmd_json.encode('utf-8'))
            
            # 等待确认
            response = client_socket.recv(1024).decode('utf-8')
            print(f"[BRIDGE] QEMU response: {response}")
            
            client_socket.close()
            return True
        
        except Exception as e:
            print(f"[BRIDGE] Failed to send to QEMU: {e}")
            return False
    
    def fetch_weather(self):
        """获取天气数据"""
        while self.running:
            try:
                url = 'https://api.open-meteo.com/v1/forecast?latitude=22.20&longitude=113.54&current_weather=true&hourly=relativehumidity_2m'
                with urllib.request.urlopen(url, timeout=10) as response:
                    data = json.loads(response.read().decode())
                    self.weather_data['forecast_temp'] = int(data['current_weather']['temperature'])
                    self.weather_data['forecast_hum'] = int(data['hourly']['relativehumidity_2m'][0])
                    
                    # 简单的天气状况判断
                    temp = self.weather_data['forecast_temp']
                    if temp > 30:
                        self.weather_data['condition'] = 'Hot'
                    elif temp > 25:
                        self.weather_data['condition'] = 'Warm'
                    elif temp > 20:
                        self.weather_data['condition'] = 'Mild'
                    else:
                        self.weather_data['condition'] = 'Cool'
                    
                    print(f"[WEATHER] {self.weather_data['forecast_temp']}°C, {self.weather_data['forecast_hum']}%, {self.weather_data['condition']}")
            
            except Exception as e:
                print(f"[WEATHER] API error: {e}")
            
            time.sleep(300)  # 5分钟更新一次
    
    def get_full_status(self):
        """获取完整状态"""
        status = self.firmware_data.copy()
        status.update(self.weather_data)
        status['feedback'] = self.command_feedback
        
        # 智能温度建议
        outdoor_temp = self.weather_data['forecast_temp']
        indoor_temp = self.firmware_data['temp']
        
        if outdoor_temp > 28:
            suggestion = min(26, indoor_temp - 2)
        elif outdoor_temp < 18:
            suggestion = max(22, indoor_temp + 2)
        else:
            suggestion = 24
        
        status['suggestion'] = suggestion
        status['last_cmd'] = self.command_feedback.get('cmd', '')
        status['cmd_ack'] = 1 if self.command_feedback['status'] == 'success' else 0
        
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
            if bridge.send_to_qemu(command):
                bridge.command_feedback = {
                    'status': 'success',
                    'cmd': command.get('cmd', ''),
                    'reason': ''
                }
            else:
                bridge.command_feedback = {
                    'status': 'failed',
                    'cmd': command.get('cmd', ''),
                    'reason': 'QEMU connection failed'
                }
            
            self.send_response(200)
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            self.wfile.write(b'OK')
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
    print("  桥接服务器 (Bridge Server)")
    print("  连接QEMU固件和Web界面")
    print("=" * 60)
    
    bridge = BridgeServer()
    
    # 启动QEMU监听线程
    threading.Thread(target=bridge.listen_to_qemu, daemon=True).start()
    
    # 启动天气获取线程
    threading.Thread(target=bridge.fetch_weather, daemon=True).start()
    
    # 启动Web服务器
    print("[BRIDGE] HTTP server on http://localhost:8080")
    print("[BRIDGE] Waiting for QEMU connection on port 9999")
    print("[BRIDGE] Web interface: web/index_simple.html")
    print()
    
    server = HTTPServer(('localhost', 8080), WebHandler)
    
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n[BRIDGE] Shutting down...")
        bridge.running = False