# backend/data_receiver.py
import sys
import os
import socketserver
import threading
import traceback
import json
import time

def log(msg):
    print(f"[DATA_RECEIVER] {msg}")
    sys.stdout.flush()

try:
    from config import DATA_SERVER_CONFIG
    log("Config loaded successfully")
except Exception as e:
    log(f"Failed to load config: {e}")
    sys.exit(1)

Database = None
try:
    from database import Database
    log("Database module loaded successfully")
except Exception as e:
    log(f"Failed to load database module: {e}")
    traceback.print_exc()

class GameDataTCPHandler(socketserver.BaseRequestHandler):
    def handle(self):
        try:
            data = self.request.recv(4096).strip()
            if not data: 
                return
                
            data_dict = json.loads(data.decode('utf-8'))
            player_name = data_dict.get("plyrName", "Unknown")
            log(f"Processing data for: {player_name}")

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
                try:
                    db = Database()
                    if db.connect():
                        db.insert_detailed_data(player_name, db_data)
                        db.close()
                except Exception as e:
                    log(f"Database error: {e}")
                    traceback.print_exc()
        except Exception as e:
            log(f"Handler error: {e}")
            traceback.print_exc()

def check_shutdown_file():
    try:
        exists = os.path.exists("data_receiver.shutdown")
        if exists:
            log("Shutdown file detected!")
            os.remove("data_receiver.shutdown")
        return exists
    except Exception as e:
        log(f"Error checking shutdown file: {e}")
        return False

def start_socket_server():
    log("Starting data receiver server...")
    
    # ENSURE DATABASE AND TABLE EXIST BEFORE STARTING SERVER
    if Database:
        try:
            db_setup = Database()
            if db_setup.setup_database_and_table():
                log("Database setup completed successfully")
            else:
                log("Database setup failed - server will continue without database support")
        except Exception as e:
            log(f"Database setup error: {e}")
            traceback.print_exc()
    else:
        log("Running without database support")

    # Create server
    try:
        server = socketserver.ThreadingTCPServer((DATA_SERVER_CONFIG['host'], DATA_SERVER_CONFIG['port']), GameDataTCPHandler)
        server.daemon_threads = True
        server.allow_reuse_address = True
        
        # Start server thread
        server_thread = threading.Thread(target=server.serve_forever)
        server_thread.daemon = True
        server_thread.start()
        
        log("Server started successfully. Waiting for shutdown signal...")
        
        # Main shutdown detection loop
        while True:
            if check_shutdown_file():
                break
            time.sleep(0.2)
            
        log("Shutting down server...")
        server.shutdown()
        server.server_close()
        log("Server stopped gracefully.")
        
    except Exception as e:
        log(f"Server error: {e}")
        traceback.print_exc()

if __name__ == "__main__":
    log("Script starting...")
    try:
        start_socket_server()
    except Exception as e:
        log(f"Fatal error: {e}")
        traceback.print_exc()
    log("Script exiting...")