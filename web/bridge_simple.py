#!/usr/bin/env python3
"""Simple HTTP Bridge - No websockets library needed"""
import subprocess
import threading
import json
from http.server import HTTPServer, BaseHTTPRequestHandler
import time
import urllib.request
import urllib.error

# Global state
latest_status = {}
latest_feedback = {'status': 'idle', 'cmd': '', 'reason': ''}
command_queue = []
real_weather = {'temp': 25, 'humidity': 50}  # Real weather from API

def fetch_real_weather():
    """Fetch real weather from Open-Meteo API for Macau"""
    global real_weather
    # Try to fetch immediately on startup
    try:
        url = 'https://api.open-meteo.com/v1/forecast?latitude=22.20&longitude=113.54&current_weather=true&hourly=relativehumidity_2m'
        with urllib.request.urlopen(url, timeout=10) as response:
            data = json.loads(response.read().decode())
            real_weather['temp'] = int(data['current_weather']['temperature'])
            real_weather['humidity'] = int(data['hourly']['relativehumidity_2m'][0])
            print(f"[Weather API] Initial fetch: {real_weather['temp']}°C, {real_weather['humidity']}%")
    except Exception as e:
        print(f"[Weather API] Initial fetch failed: {e}")
        print(f"[Weather API] Using default: {real_weather['temp']}°C, {real_weather['humidity']}%")
    
    # Continue periodic updates
    while True:
        time.sleep(300)  # Update every 5 minutes
        try:
            url = 'https://api.open-meteo.com/v1/forecast?latitude=22.20&longitude=113.54&current_weather=true&hourly=relativehumidity_2m'
            with urllib.request.urlopen(url, timeout=10) as response:
                data = json.loads(response.read().decode())
                real_weather['temp'] = int(data['current_weather']['temperature'])
                real_weather['humidity'] = int(data['hourly']['relativehumidity_2m'][0])
                print(f"[Weather API] Updated: {real_weather['temp']}°C, {real_weather['humidity']}%")
        except Exception as e:
            print(f"[Weather API] Update failed: {e}, keeping previous values")

class BridgeHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/status':
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.send_header('Cache-Control', 'no-cache')
            self.end_headers()
            response = latest_status.copy() if latest_status else {
                'temp': 0, 'humidity': 0, 'target': 24, 'power': 0,
                'mode': 'auto', 'fan': 2, 'swing': 0, 'direction': 'auto',
                'forecast_temp': real_weather['temp'], 'forecast_hum': real_weather['humidity'],
                'suggestion': 24, 'condition': 'Unknown', 'last_cmd': '', 'cmd_ack': 0
            }
            response['feedback'] = latest_feedback
            self.wfile.write(json.dumps(response).encode())
        else:
            self.send_response(404)
            self.end_headers()
    
    def do_POST(self):
        if self.path == '/command':
            length = int(self.headers['Content-Length'])
            data = self.rfile.read(length)
            command = json.loads(data.decode())
            command_queue.append(command)
            
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
        pass  # Suppress HTTP logs

def firmware_thread():
    """Run QEMU and handle I/O"""
    global latest_status, command_queue
    
    import os
    import shutil
    qemu_exe = shutil.which('qemu-system-arm')
    if not qemu_exe:
        for path in ['F:/QEMU/qemu-system-arm.exe', 'C:/Program Files/qemu/qemu-system-arm.exe']:
            if os.path.exists(path):
                qemu_exe = path
                break
    
    if not qemu_exe:
        print("[Bridge] ERROR: qemu-system-arm not found!")
        return
    
    firmware_path = 'build/firmware.elf'
    if not os.path.exists(firmware_path):
        firmware_path = '../build/firmware.elf'
    if not os.path.exists(firmware_path):
        print("[Bridge] ERROR: build/firmware.elf not found!")
        return
    
    # Use file-based communication for Windows
    output_file = 'qemu_output.txt'
    if os.path.exists(output_file):
        os.remove(output_file)
    
    qemu_cmd = [
        qemu_exe,
        '-M', 'mps2-an386',
        '-kernel', firmware_path,
        '-nographic',
        '-semihosting',
        '-monitor', 'none'
    ]
    
    print(f"[Bridge] Starting QEMU: {qemu_exe}")
    print(f"[Bridge] Output file: {output_file}")
    
    with open(output_file, 'w') as f:
        process = subprocess.Popen(
            qemu_cmd,
            stdin=subprocess.PIPE,
            stdout=f,
            stderr=subprocess.STDOUT,
            bufsize=0
        )
    
    def read_output():
        global latest_status, latest_feedback, real_weather
        last_pos = 0
        while True:
            try:
                if os.path.exists(output_file):
                    with open(output_file, 'r', encoding='utf-8', errors='ignore') as f:
                        f.seek(last_pos)
                        lines = f.readlines()
                        last_pos = f.tell()
                        
                        for line in lines:
                            line = line.strip()
                            if not line:
                                continue
                            if line.startswith('{"type":"status"'):
                                try:
                                    latest_status = json.loads(line)
                                    latest_status['forecast_temp'] = real_weather['temp']
                                    latest_status['forecast_hum'] = real_weather['humidity']
                                    if int(time.time()) % 10 == 0:
                                        print(f"[Status] {latest_status.get('temp')}°C/{latest_status.get('humidity')}% -> {latest_status.get('target')}°C")
                                except:
                                    pass
                            elif line.startswith('{"type":"feedback"'):
                                try:
                                    feedback = json.loads(line)
                                    latest_feedback = {'status': feedback.get('status', 'unknown'), 'cmd': feedback.get('cmd', ''), 'reason': feedback.get('reason', '')}
                                    print(f"[Feedback] {feedback.get('cmd')} -> {feedback.get('status')}")
                                except:
                                    pass
                            else:
                                if any(kw in line for kw in ['[System]', '[Init]', '[WiFi]', '[ERROR]']):
                                    print(f"[FW] {line}")
                time.sleep(0.1)
            except:
                time.sleep(0.1)
    
    threading.Thread(target=read_output, daemon=True).start()
    
    # Send commands to firmware
    while True:
        if command_queue:
            cmd = command_queue.pop(0)
            try:
                cmd_str = json.dumps(cmd) + '\n'
                process.stdin.write(cmd_str.encode('utf-8'))
                process.stdin.flush()
                print(f"[Command] {cmd}")
            except:
                print(f"[Error] Failed to send command: {cmd}")
        time.sleep(0.1)

if __name__ == '__main__':
    print("=" * 50)
    print("  Simple HTTP Bridge (No websockets needed)")
    print("=" * 50)
    print()
    
    # Start weather fetching thread
    threading.Thread(target=fetch_real_weather, daemon=True).start()
    
    # Start firmware thread
    threading.Thread(target=firmware_thread, daemon=True).start()
    
    # Start HTTP server
    print("[Bridge] HTTP server on http://localhost:8080")
    print("[Bridge] Open web interface:")
    print("         English: web/index_en.html (default)")
    print("         Chinese: web/index_simple.html")
    print()
    
    server = HTTPServer(('localhost', 8080), BridgeHandler)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n[Bridge] Shutting down...")
