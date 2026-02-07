import socketserver
import threading
import time
import os
import traceback
import json
from config import DATA_SERVER_CONFIG
from database import Database

# Global server reference
_server = None

class GameDataTCPHandler(socketserver.BaseRequestHandler):
    def handle(self):
        print(f"[*] Connection from {self.client_address[0]}:{self.client_address[1]}")
        try:
            data = self.request.recv(4096).strip()
            if not data: 
                return
            data_dict = json.loads(data.decode('utf-8'))
            player_name = data_dict.get("plyrName", "Unknown")
            print(f"[*] Processed data for: {player_name}")

            if Database:
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
                db = Database()
                if db.connect():
                    db.insert_detailed_data(player_name, db_data)
                    db.close()
        except Exception as e:
            traceback.print_exc()

def check_shutdown_file():
    return os.path.exists("data_receiver.shutdown")

def start_socket_server():
    global _server
    print(f"[*] Starting server on {DATA_SERVER_CONFIG['host']}:{DATA_SERVER_CONFIG['port']}...")

    # Setup DB
    db_setup = Database()
    db_setup.setup_database_and_table()

    _server = socketserver.ThreadingTCPServer((DATA_SERVER_CONFIG['host'], DATA_SERVER_CONFIG['port']), GameDataTCPHandler)
    _server.daemon_threads = True
    _server.allow_reuse_address = True

    # Start server in background thread
    server_thread = threading.Thread(target=_server.serve_forever)
    server_thread.daemon = True
    server_thread.start()

    print("[*] Server running. Waiting for shutdown signal...")

    # Main thread: wait for shutdown file
    while True:
        if check_shutdown_file():
            print("[*] Shutdown file detected.")
            os.remove("data_receiver.shutdown")
            break
        time.sleep(0.2)

    # Graceful shutdown
    print("[*] Shutting down server...")
    _server.shutdown()
    _server.server_close()
    print("[*] Server stopped gracefully.")