#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
WebSocket Bridge Server for Real-time AC Control
Author: jiannan WEI (MC555577)
"""

import json
import time
import threading
import asyncio
import websockets
from http.server import HTTPServer, BaseHTTPRequestHandler
import urllib.request

class WebSocketBridge:
    def __init__(self):
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
            'last_update': time.time(),
            'connected': True
        }
        
        self.clients = set()
        self.running = True
        
    async def register_client(self, websocket):
        """Register new WebSocket client"""
        self.clients.add(websocket)
        print(f"[WS] Client connected: {websocket.remote_address}")
        
        # Send initial status
        await websocket.send(json.dumps({
            'type': 'status',
            'data': self.get_full_status()
        }))
    
    async def unregister_client(self, websocket):
        """Unregister WebSocket client"""
        self.clients.discard(websocket)
        print(f"[WS] Client disconnected: {websocket.remote_address}")
    
    async def broadcast_status(self):
        """Broadcast status to all connected clients"""
        if self.clients:
            message = json.dumps({
                'type': 'status',
                'data': self.get_full_status()
            })
            
            # Send to all clients
            disconnected = set()
            for client in self.clients:
                try:
                    await client.send(message)
                except websockets.exceptions.ConnectionClosed:
                    disconnected.add(client)
            
            # Remove disconnected clients
            for client in disconnected:
                self.clients.discard(client)
    
    async def handle_websocket(self, websocket, path):
        """Handle WebSocket connections"""
        await self.register_client(websocket)
        
        try:
            async for message in websocket:
                try:
                    data = json.loads(message)
                    
                    if data['type'] == 'command':
                        command = data['data']
                        success = self.process_command(command)
                        
                        # Send command response
                        await websocket.send(json.dumps({
                            'type': 'command_response',
                            'success': success,
                            'command': command
                        }))
                        
                        # Broadcast updated status
                        await self.broadcast_status()
                        
                except json.JSONDecodeError:
                    print(f"[WS] Invalid JSON received: {message}")
                    
        except websockets.exceptions.ConnectionClosed:
            pass
        finally:
            await self.unregister_client(websocket)
    
    def simulate_sensor_data(self):
        """Simulate sensor data changes"""
        def update_sensors():
            while self.running:
                # Simulate temperature changes
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
                
                # Simulate humidity changes
                if self.status['mode'] == 'dry':
                    self.status['humidity'] = max(30, self.status['humidity'] - 0.2)
                else:
                    import random
                    self.status['humidity'] += random.uniform(-0.5, 0.5)
                    self.status['humidity'] = max(30, min(80, self.status['humidity']))
                
                self.status['last_update'] = time.time()
                
                # Broadcast status update
                asyncio.run_coroutine_threadsafe(
                    self.broadcast_status(), 
                    asyncio.get_event_loop()
                )
                
                time.sleep(2)  # Update every 2 seconds
        
        threading.Thread(target=update_sensors, daemon=True).start()
    
    def fetch_weather(self):
        """Fetch weather data"""
        def weather_updater():
            while self.running:
                try:
                    url = 'https://api.open-meteo.com/v1/forecast?latitude=22.20&longitude=113.54&current_weather=true&hourly=relativehumidity_2m'
                    with urllib.request.urlopen(url, timeout=10) as response:
                        data = json.loads(response.read().decode())
                        
                        self.status['forecast_temp'] = int(data['current_weather']['temperature'])
                        self.status['forecast_hum'] = int(data['hourly']['relativehumidity_2m'][0])
                        
                        # Weather condition
                        temp = self.status['forecast_temp']
                        if temp > 30:
                            self.status['condition'] = 'Hot'
                        elif temp > 25:
                            self.status['condition'] = 'Warm'
                        elif temp > 20:
                            self.status['condition'] = 'Mild'
                        else:
                            self.status['condition'] = 'Cool'
                        
                        # Smart suggestion
                        outdoor_temp = self.status['forecast_temp']
                        if outdoor_temp > 28:
                            self.status['suggestion'] = 26
                        elif outdoor_temp < 18:
                            self.status['suggestion'] = 22
                        else:
                            self.status['suggestion'] = 24
                        
                        print(f"[WEATHER] {self.status['forecast_temp']}C, {self.status['forecast_hum']}%, {self.status['condition']}")
                        
                        # Broadcast weather update
                        asyncio.run_coroutine_threadsafe(
                            self.broadcast_status(), 
                            asyncio.get_event_loop()
                        )
                
                except Exception as e:
                    print(f"[WEATHER] API error: {e}")
                
                time.sleep(300)  # Update every 5 minutes
        
        threading.Thread(target=weather_updater, daemon=True).start()
    
    def process_command(self, command):
        """Process control commands"""
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
        """Get full status"""
        status = self.status.copy()
        status['feedback'] = {
            'status': 'success',
            'cmd': self.status['last_cmd'],
            'reason': ''
        }
        return status

# HTTP fallback handler
class HTTPHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/status':
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*')
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

async def main():
    global bridge
    bridge = WebSocketBridge()
    
    print("=" * 60)
    print("  WebSocket Bridge Server")
    print("  Real-time Smart AC Control")
    print("=" * 60)
    
    # Start simulation threads
    bridge.simulate_sensor_data()
    bridge.fetch_weather()
    
    # Start HTTP server in thread
    def start_http():
        server = HTTPServer(('localhost', 8080), HTTPHandler)
        server.serve_forever()
    
    threading.Thread(target=start_http, daemon=True).start()
    
    print("[BRIDGE] WebSocket server on ws://localhost:8765")
    print("[BRIDGE] HTTP API server on http://localhost:8080")
    print("[BRIDGE] Web interface: web/index_simple.html")
    print()
    print("Features:")
    print("- Real-time WebSocket connection")
    print("- Live sensor data updates")
    print("- Instant command feedback")
    print("- Weather API integration")
    print()
    print("Server is running... Press Ctrl+C to stop")
    
    # Start WebSocket server
    async with websockets.serve(bridge.handle_websocket, "localhost", 8765):
        await asyncio.Future()  # Run forever

if __name__ == '__main__':
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n[BRIDGE] Shutting down...")