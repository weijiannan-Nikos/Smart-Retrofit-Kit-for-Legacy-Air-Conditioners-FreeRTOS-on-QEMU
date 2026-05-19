#!/usr/bin/env python3
import json
import time
import threading
import urllib.request
from http.server import HTTPServer, BaseHTTPRequestHandler

class WorkingBridge:
    def __init__(self):
        self.status = {
            'temp': 25.5,
            'humidity': 55.0,
            'target': 24,
            'power': 0,
            'mode': 'auto',
            'fan': 2,
            'direction': 'auto',
            'forecast_temp': 28,
            'forecast_hum': 60,
            'suggestion': 26
        }
        threading.Thread(target=self.update_weather, daemon=True).start()
        threading.Thread(target=self.simulate_ac, daemon=True).start()
    
    def update_weather(self):
        while True:
            try:
                url = 'https://api.open-meteo.com/v1/forecast?latitude=22.20&longitude=113.54&current_weather=true&hourly=relativehumidity_2m'
                with urllib.request.urlopen(url, timeout=5) as response:
                    data = json.loads(response.read().decode())
                    self.status['forecast_temp'] = int(data['current_weather']['temperature'])
                    self.status['forecast_hum'] = int(data['hourly']['relativehumidity_2m'][0])
                    
                    outdoor = self.status['forecast_temp']
                    if outdoor > 28:
                        self.status['suggestion'] = 24
                    elif outdoor < 18:
                        self.status['suggestion'] = 22
                    else:
                        self.status['suggestion'] = 26
                        
                    print(f"[WEATHER] {outdoor}°C, {self.status['forecast_hum']}%")
            except:
                pass
            time.sleep(300)
    
    def simulate_ac(self):
        while True:
            if self.status['power']:
                target = self.status['target']
                current = self.status['temp']
                
                if self.status['mode'] == 'cool' and current > target:
                    self.status['temp'] = max(target, current - 0.1)
                elif self.status['mode'] == 'heat' and current < target:
                    self.status['temp'] = min(target, current + 0.1)
                elif self.status['mode'] == 'auto':
                    if abs(current - target) > 0.1:
                        if current > target:
                            self.status['temp'] -= 0.1
                        else:
                            self.status['temp'] += 0.1
            time.sleep(3)

class Handler(BaseHTTPRequestHandler):
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
            
            cmd = data.get('cmd')
            val = data.get('val')
            
            print(f"[CMD] {cmd} = {val}")
            
            if cmd == 'power':
                bridge.status['power'] = int(val)
            elif cmd == 'temp':
                bridge.status['target'] = int(val)
            elif cmd == 'mode':
                bridge.status['mode'] = str(val)
            elif cmd == 'fan':
                bridge.status['fan'] = int(val)
            elif cmd == 'direction':
                bridge.status['direction'] = str(val)
            elif cmd == 'apply_ai':
                bridge.status['target'] = bridge.status['suggestion']
            
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
    bridge = WorkingBridge()
    print("Working Bridge Server on http://localhost:8080")
    HTTPServer(('localhost', 8080), Handler).serve_forever()