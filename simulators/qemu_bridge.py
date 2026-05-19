#!/usr/bin/env python3
import json
import socket
import threading
import time
import requests
from http.server import HTTPServer, BaseHTTPRequestHandler

class QEMUBridge:
    def __init__(self):
        self.qemu_socket = None
        self.qemu_connected = False
        self.status = {
            'temp': 25.2,
            'humidity': 52.0,
            'target': 24,
            'power': 1,
            'mode': 'auto',
            'fan': 2,
            'direction': 'auto',
            'forecast_temp': 28,
            'forecast_hum': 60,
            'suggestion': 26
        }
        self.start_weather_updates()
        
    def start_qemu_server(self):
        """Listen for QEMU connection on port 9999"""
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server_socket.bind(('localhost', 9999))
        server_socket.listen(1)
        print("[BRIDGE] Waiting for QEMU on port 9999...")
        
        while True:
            try:
                client_socket, addr = server_socket.accept()
                print(f"[BRIDGE] QEMU connected from {addr}")
                self.qemu_socket = client_socket
                self.qemu_connected = True
                self.handle_qemu_connection(client_socket)
            except Exception as e:
                print(f"[BRIDGE] QEMU connection error: {e}")
                self.qemu_connected = False
    
    def handle_qemu_connection(self, client_socket):
        """Handle QEMU data"""
        buffer = ""
        while self.qemu_connected:
            try:
                data = client_socket.recv(1024).decode('utf-8')
                if not data:
                    break
                
                buffer += data
                while '\n' in buffer:
                    line, buffer = buffer.split('\n', 1)
                    if line.strip():
                        try:
                            qemu_data = json.loads(line.strip())
                            self.update_from_qemu(qemu_data)
                        except json.JSONDecodeError:
                            print(f"[BRIDGE] Invalid JSON: {line}")
            
            except Exception as e:
                print(f"[BRIDGE] Error: {e}")
                break
        
        self.qemu_connected = False
        client_socket.close()
        print("[BRIDGE] QEMU disconnected")
    
    def update_from_qemu(self, data):
        """Update status from QEMU"""
        if 'temp' in data:
            self.status['temp'] = data['temp']
        if 'humidity' in data:
            self.status['humidity'] = data['humidity']
        if 'target' in data:
            self.status['target'] = data['target']
        if 'power' in data:
            self.status['power'] = data['power']
        if 'mode' in data:
            self.status['mode'] = data['mode']
        if 'fan' in data:
            self.status['fan'] = data['fan']
        
        print(f"[QEMU] T={self.status['temp']:.1f}°C H={self.status['humidity']:.1f}% Target={self.status['target']}°C")
        self.update_ai_suggestion()
    
    def send_to_qemu(self, command):
        """Send command to QEMU"""
        if not self.qemu_connected or not self.qemu_socket:
            return False
        
        try:
            cmd_json = json.dumps(command) + '\n'
            self.qemu_socket.send(cmd_json.encode('utf-8'))
            print(f"[BRIDGE] Sent to QEMU: {command}")
            return True
        except Exception as e:
            print(f"[BRIDGE] Send error: {e}")
            return False
    
    def start_weather_updates(self):
        """Start weather API updates"""
        def update_weather():
            while True:
                try:
                    # 澳门天气API
                    url = "https://api.open-meteo.com/v1/forecast?latitude=22.1987&longitude=113.5439&current=temperature_2m,relative_humidity_2m&timezone=Asia/Shanghai"
                    response = requests.get(url, timeout=5)
                    if response.status_code == 200:
                        data = response.json()
                        current = data.get('current', {})
                        self.status['forecast_temp'] = int(current.get('temperature_2m', 28))
                        self.status['forecast_hum'] = int(current.get('relative_humidity_2m', 60))
                        print(f"[WEATHER] Updated: {self.status['forecast_temp']}°C, {self.status['forecast_hum']}%")
                except Exception as e:
                    print(f"[WEATHER] API error: {e}")
                    self.status['forecast_temp'] = 28
                    self.status['forecast_hum'] = 60
                
                time.sleep(300)  # 5分钟更新一次
        
        threading.Thread(target=update_weather, daemon=True).start()
    
    def update_ai_suggestion(self):
        """Update AI temperature suggestion"""
        outdoor_temp = self.status['forecast_temp']
        indoor_temp = self.status['temp']
        
        if outdoor_temp > 30:
            suggestion = 22
        elif outdoor_temp > 25:
            suggestion = 24
        elif outdoor_temp < 15:
            suggestion = 26
        else:
            suggestion = 25
        
        temp_diff = abs(outdoor_temp - indoor_temp)
        if temp_diff > 10:
            suggestion = int((outdoor_temp + indoor_temp) / 2)
        
        self.status['suggestion'] = max(18, min(30, suggestion))

class WebHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/status':
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            self.wfile.write(json.dumps(bridge.status).encode())
    
    def do_POST(self):
        if self.path == '/command':
            length = int(self.headers['Content-Length'])
            data = json.loads(self.rfile.read(length).decode())
            
            if data.get('cmd') == 'apply_ai':
                bridge.status['target'] = bridge.status['suggestion']
                success = bridge.send_to_qemu({'cmd': 'temp', 'val': bridge.status['suggestion']})
            else:
                success = bridge.send_to_qemu(data)
            
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            
            response = {'success': success, 'message': 'OK' if success else 'QEMU not connected'}
            self.wfile.write(json.dumps(response).encode())
    
    def do_OPTIONS(self):
        self.send_response(200)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        self.end_headers()
    
    def log_message(self, format, *args):
        pass

if __name__ == '__main__':
    bridge = QEMUBridge()
    
    # Start QEMU server thread
    threading.Thread(target=bridge.start_qemu_server, daemon=True).start()
    
    print("QEMU Bridge Server")
    print("Web API: http://localhost:8080")
    print("QEMU Port: 9999")
    
    # Start web server
    HTTPServer(('localhost', 8080), WebHandler).serve_forever()