#!/usr/bin/env python3
import json
import time
import threading
from http.server import HTTPServer, BaseHTTPRequestHandler

class ACServer:
    def __init__(self):
        self.status = {
            'temp': 25.5,
            'humidity': 55.0,
            'target': 24,
            'power': 1,
            'mode': 'auto',
            'fan': 2,
            'direction': 'auto',
            'forecast_temp': 28,
            'forecast_hum': 60,
            'suggestion': 26,
            'smart_config': True,
            'auto_adjust': False
        }
        self.running = True
        threading.Thread(target=self.simulate, daemon=True).start()
        threading.Thread(target=self.smart_control, daemon=True).start()
    
    def smart_control(self):
        """Smart configuration logic"""
        while self.running:
            if self.status['smart_config'] and self.status['power']:
                outdoor = self.status['forecast_temp']
                indoor = self.status['temp']
                
                # Auto mode selection
                if outdoor > 28 and indoor > 26:
                    self.status['mode'] = 'cool'
                    self.status['suggestion'] = 24
                elif outdoor < 18 and indoor < 20:
                    self.status['mode'] = 'heat'
                    self.status['suggestion'] = 22
                elif self.status['humidity'] > 70:
                    self.status['mode'] = 'dry'
                else:
                    self.status['mode'] = 'auto'
                
                # Auto fan speed
                temp_diff = abs(indoor - self.status['target'])
                if temp_diff > 3:
                    self.status['fan'] = 3  # High
                elif temp_diff > 1:
                    self.status['fan'] = 2  # Medium
                else:
                    self.status['fan'] = 1  # Low
            
            time.sleep(15)  # Reduced to 15 seconds for better responsiveness
    
    def simulate(self):
        """Simulate sensor data with proper task scheduling"""
        while self.running:
            try:
                if self.status['power']:
                    target = self.status['target']
                    current = self.status['temp']
                    
                    # Simulate temperature change based on mode
                    if self.status['mode'] == 'cool' and current > target:
                        self.status['temp'] = max(target, current - 0.2)
                    elif self.status['mode'] == 'heat' and current < target:
                        self.status['temp'] = min(target, current + 0.2)
                    elif self.status['mode'] == 'auto':
                        if abs(current - target) > 0.1:
                            if current > target:
                                self.status['temp'] -= 0.1
                            else:
                                self.status['temp'] += 0.1
                    
                    # Simulate humidity changes
                    if self.status['mode'] == 'dry':
                        self.status['humidity'] = max(30, self.status['humidity'] - 0.5)
                    else:
                        import random
                        self.status['humidity'] += random.uniform(-0.3, 0.3)
                        self.status['humidity'] = max(30, min(80, self.status['humidity']))
                
                # Prevent task blocking
                time.sleep(2)  # Reduced from 5 to 2 seconds
                
            except Exception as e:
                print(f"Simulation error: {e}")
                time.sleep(1)

class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/status':
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            self.wfile.write(json.dumps(server.status).encode())
    
    def do_POST(self):
        if self.path == '/command':
            length = int(self.headers['Content-Length'])
            data = json.loads(self.rfile.read(length).decode())
            
            cmd = data.get('cmd')
            val = data.get('val')
            
            if cmd == 'power':
                server.status['power'] = int(val)
            elif cmd == 'temp':
                server.status['target'] = int(val)
            elif cmd == 'mode':
                server.status['mode'] = str(val)
            elif cmd == 'fan':
                server.status['fan'] = int(val)
            elif cmd == 'smart_config':
                server.status['smart_config'] = bool(val)
            elif cmd == 'apply_ai':
                server.status['target'] = server.status['suggestion']
                server.status['auto_adjust'] = True
            
            self.send_response(200)
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            self.wfile.write(b'OK')
    
    def do_OPTIONS(self):
        self.send_response(200)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        self.end_headers()
    
    def log_message(self, format, *args):
        pass

if __name__ == '__main__':
    server = ACServer()
    print("AC Control Server running on http://localhost:8080")
    HTTPServer(('localhost', 8080), Handler).serve_forever()