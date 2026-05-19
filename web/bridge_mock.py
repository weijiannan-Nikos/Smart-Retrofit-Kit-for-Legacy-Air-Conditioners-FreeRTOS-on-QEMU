#!/usr/bin/env python3
"""Mock Bridge - Returns simulated data without QEMU"""
import threading
import json
from http.server import HTTPServer, BaseHTTPRequestHandler
import time
import urllib.request

# Global state
latest_status = {
    'temp': 25,
    'humidity': 50,
    'target': 24,
    'power': 0,
    'mode': 'auto',
    'fan': 2,
    'swing': 0,
    'direction': 'auto',
    'forecast_temp': 25,
    'forecast_hum': 50,
    'suggestion': 24,
    'condition': 'Unknown',
    'last_cmd': '',
    'cmd_ack': 1
}
latest_feedback = {'status': 'idle', 'cmd': '', 'reason': ''}
real_weather = {'temp': 25, 'humidity': 50}

def fetch_real_weather():
    global real_weather
    try:
        url = 'https://api.open-meteo.com/v1/forecast?latitude=22.20&longitude=113.54&current_weather=true&hourly=relativehumidity_2m'
        with urllib.request.urlopen(url, timeout=10) as response:
            data = json.loads(response.read().decode())
            real_weather['temp'] = int(data['current_weather']['temperature'])
            real_weather['humidity'] = int(data['hourly']['relativehumidity_2m'][0])
            print(f"[Weather API] {real_weather['temp']}°C, {real_weather['humidity']}%")
    except Exception as e:
        print(f"[Weather API] Failed: {e}")
    
    while True:
        time.sleep(300)
        try:
            with urllib.request.urlopen(url, timeout=10) as response:
                data = json.loads(response.read().decode())
                real_weather['temp'] = int(data['current_weather']['temperature'])
                real_weather['humidity'] = int(data['hourly']['relativehumidity_2m'][0])
                print(f"[Weather API] Updated: {real_weather['temp']}°C, {real_weather['humidity']}%")
        except:
            pass

class BridgeHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/status':
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.send_header('Cache-Control', 'no-cache')
            self.end_headers()
            
            # Update with real weather
            latest_status['forecast_temp'] = real_weather['temp']
            latest_status['forecast_hum'] = real_weather['humidity']
            latest_status['feedback'] = latest_feedback
            
            self.wfile.write(json.dumps(latest_status).encode())
        else:
            self.send_response(404)
            self.end_headers()
    
    def do_POST(self):
        if self.path == '/command':
            length = int(self.headers['Content-Length'])
            data = self.rfile.read(length)
            command = json.loads(data.decode())
            
            print(f"[Command] {command}")
            
            # Process command
            cmd = command.get('cmd')
            val = command.get('val')
            
            if cmd == 'power':
                latest_status['power'] = val
            elif cmd == 'temp':
                latest_status['target'] = val
            elif cmd == 'mode':
                latest_status['mode'] = val
            elif cmd == 'fan':
                latest_status['fan'] = val
            
            # Send success feedback
            global latest_feedback
            latest_feedback = {'status': 'success', 'cmd': cmd, 'reason': ''}
            
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
    print("=" * 50)
    print("  Mock Bridge (No QEMU - Simulated Data)")
    print("=" * 50)
    print()
    print("[Mock] Using simulated indoor data")
    print("[Mock] Fetching real outdoor weather...")
    print()
    
    # Start weather thread
    threading.Thread(target=fetch_real_weather, daemon=True).start()
    
    # Start HTTP server
    print("[Bridge] HTTP server on http://localhost:8080")
    print("[Bridge] Open web interface:")
    print("         web/index_simple.html (Chinese)")
    print("         web/index_en.html (English)")
    print()
    print("NOTE: This is MOCK mode - no firmware running")
    print("      Indoor temp is simulated at 25°C")
    print("      Outdoor weather is real from API")
    print()
    
    server = HTTPServer(('localhost', 8080), BridgeHandler)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n[Bridge] Shutting down...")
