# backend/data_receiver.py
import socketserver
import threading
import traceback
import json
import os
import time
from config import DATA_SERVER_CONFIG
from database import Database

def parse_location(location_str):
    """Safely parse location string like '[x, y, z]'"""
    try:
        if location_str and isinstance(location_str, str) and location_str.startswith('[') and location_str.endswith(']'):
            coords = location_str[1:-1].split(',')
            return [float(c.strip()) for c in coords[:3]]
        return [0.0, 0.0, 0.0]
    except (ValueError, IndexError, AttributeError):
        return [0.0, 0.0, 0.0]

class GameDataTCPHandler(socketserver.BaseRequestHandler):
    """The request handler class for our server."""
    def handle(self):
        print(f"[*] Connection from {self.client_address[0]}:{self.client_address[1]}")
        try:
            data = self.request.recv(4096).strip()
            if not data:
                print(f"[*] No data received from {self.client_address[0]}. Closing connection.")
                return
            print(f"[*] Received {len(data)} bytes from {self.client_address[0]}")
            
            # Parse the JSON from Java
            data_dict = json.loads(data.decode('utf-8'))
            player_name = data_dict.get("plyrName", "Unknown")
            print(f"[*] Successfully processed data for player: {player_name}")
            
            # Create the final data dictionary that the database expects
            db_data = {
                "player_name": player_name,
                "timestamp": f"{data_dict.get('date', '')}T{data_dict.get('time', '')}",
                "fps": data_dict.get("fps"),
                "plyrLocation": data_dict.get("plyrLocation"),
                "plyrHealth": data_dict.get("plyrHealth"),
                "plyrInventory": data_dict.get("plyrInventory"),
                "plyrArmor": data_dict.get("plyrArmor"),
                "plyrOffhand": data_dict.get("plyrOffhand"),
                "plyrStatus": data_dict.get("plyrStatus"),
                "plyrHunger": data_dict.get("plyrHunger"),
                "plyrSat": data_dict.get("plyrSat"),
                "plyrView": data_dict.get("plyrView"),
                "plyrFacing": data_dict.get("plyrFacing"),
                "plyrSelectedSlot": data_dict.get("plyrSelectedSlot"),
                "plyrSelectedItem": data_dict.get("plyrSelectedItem"),
                "plyrRideState": data_dict.get("plyrRideState"),
                "plyrRideVehicle": data_dict.get("plyrRideVehicle"),
                "plyrMomentum": data_dict.get("plyrMomentum")
            }
            
            # Save the FULL data to the database with fresh connection
            db = Database()
            if db.connect():
                db.insert_detailed_data(player_name, db_data)
                db.close()
                print(f"[HEARTBEAT] SUCCESS: All data for player '{player_name}' saved to detailed database.")
            else:
                print("[!!!] Could not connect to database. Cannot save data.")
                
        except json.JSONDecodeError:
            print(f"[!] Failed to decode JSON from {self.client_address[0]}. Raw data: {data}")
        except Exception as e:
            traceback.print_exc()
            print(f"[!] Could not process message from {self.client_address[0]}: {e}")
        finally:
            print(f"[*] Closing connection with {self.client_address[0]}")

def check_shutdown_file():
    """Check for external shutdown file created by C++ wrapper."""
    shutdown_file = "data_receiver.shutdown"
    if os.path.exists(shutdown_file):
        print(f"[*] Shutdown file detected: {shutdown_file}")
        try:
            os.remove(shutdown_file)
        except OSError:
            pass
        return True
    return False

def start_socket_server():
    """Starts the TCP socket server with graceful shutdown support."""
    print(f"[*] Starting data collection server on {DATA_SERVER_CONFIG['host']}:{DATA_SERVER_CONFIG['port']}...")
    
    # Ensure database and table exist before starting server
    db_setup = Database()
    if not db_setup.setup_database_and_table():
        print("[!!!] Failed to set up database. Server will not start.")
        return
    
    server = socketserver.ThreadingTCPServer((DATA_SERVER_CONFIG['host'], DATA_SERVER_CONFIG['port']), GameDataTCPHandler)
    server.daemon_threads = True
    
    # Start server in background thread
    server_thread = threading.Thread(target=server.serve_forever)
    server_thread.daemon = True
    server_thread.start()
    
    print("[*] Data receiver server started successfully.")
    
    # Wait for shutdown signal (file-based from C++ wrapper)
    try:
        while True:
            if check_shutdown_file():
                break
            time.sleep(0.5)
    except KeyboardInterrupt:
        print("\n[!] Server interrupted by user.")
    finally:
        print("[*] Shutting down server...")
        server.shutdown()
        server.server_close()
        print("[*] Server stopped gracefully.")

def stop_socket_server():
    """No-op since server uses file-based shutdown from C++ wrapper."""
    pass

if __name__ == "__main__":
    start_socket_server()