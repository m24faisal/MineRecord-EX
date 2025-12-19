# backend/data_receiver.py
import socketserver
import threading
import traceback
import json
from .config import DATA_SERVER_CONFIG
from .database import Database

# --- Global variables for server management ---
shutdown_server_event = threading.Event()
db_instance = None

def initialize_database():
    """Initializes the global database instance and sets up the database/tables."""
    global db_instance
    if db_instance is None:
        print("[SERVER] Initializing global database instance...")
        db_instance = Database()
        db_instance.connect()
        db_instance.setup_database_and_table()
        print("[SERVER] Database instance is ready.")

def shutdown_database():
    """Closes the global database connection."""
    global db_instance
    if db_instance:
        print("[SERVER] Shutting down database connection...")
        db_instance.close()
        db_instance = None

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

            # --- Parse the JSON from Java ---
            data_dict = json.loads(data.decode('utf-8'))
            
            player_name = data_dict.get("plyrName", "Unknown")
            print(f"[*] Successfully processed data for player: {player_name}")
            
            # --- Create the final data dictionary that the database expects ---
            # This now includes EVERY field from your Java application.
            db_data = {
                "player_name": player_name,
                "timestamp": f"{data_dict.get('date', '')}T{data_dict.get('time', '')}",
                "x": float(data_dict.get("plyrLocation", "[0, 0, 0]").strip("[]").split(',')[0]),
                "y": float(data_dict.get("plyrLocation", "[0, 0, 0]").strip("[]").split(',')[1]),
                "z": float(data_dict.get("plyrLocation", "[0, 0, 0]").strip("[]").split(',')[2]),
                "health": data_dict.get("plyrHealth"),
                "level": data_dict.get("plyrHunger"),
                "experience": 0, # Default as Java doesn't send this
                "fps": data_dict.get("fps"),
                "plyrLocation": data_dict.get("plyrLocation"),
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
            
            # --- Save the FULL data to the database ---
            if db_instance:
                db_instance.insert_detailed_data(player_name, db_data)
                print(f"[HEARTBEAT] SUCCESS: All data for player '{player_name}' saved to detailed database.")
            else:
                print("[!!!] Database instance is not available. Cannot save data.")
            
        except json.JSONDecodeError:
            print(f"[!] Failed to decode JSON from {self.client_address[0]}. Raw data: {data}")
        except Exception as e:
            traceback.print_exc()
            print(f"[!] Could not process message from {self.client_address[0]}: {e}")
        finally:
            print(f"[*] Closing connection with {self.client_address[0]}")

def start_socket_server():
    """Starts the TCP socket server in a blocking call."""
    print(f"[*] Starting data collection server on {DATA_SERVER_CONFIG['host']}:{DATA_SERVER_CONFIG['port']}...")
    initialize_database()
    
    server = socketserver.ThreadingTCPServer((DATA_SERVER_CONFIG['host'], DATA_SERVER_CONFIG['port']), GameDataTCPHandler)

    def server_shutdown_checker():
        while not shutdown_server_event.is_set():
            shutdown_server_event.wait(1)
        print("[*] Shutdown event received. Shutting down server...")
        server.shutdown()

    shutdown_thread = threading.Thread(target=server_shutdown_checker)
    shutdown_thread.daemon = True
    shutdown_thread.start()

    try:
        server.serve_forever()
    except (KeyboardInterrupt, SystemExit):
        print("\n[!] Server interrupted.")
    finally:
        print("[*] Closing server socket.")
        server.server_close()
        shutdown_database()
        print("[*] Server stopped.")

def stop_socket_server():
    """Signals the socket server to shut down."""
    print("[*] Signaling socket server to stop...")
    shutdown_server_event.set()

if __name__ == "__main__":
    start_socket_server()