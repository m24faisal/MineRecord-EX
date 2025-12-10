# backend/receive.py
import socketserver
import threading
import traceback
import dataFormat as df
import json
import re
from dbManage import Database

# --- FIX: A global event to signal the server to shut down ---
# This is a simple way to control the server thread from the main thread.
shutdown_server_event = threading.Event()

class GameDataTCPHandler(socketserver.BaseRequestHandler):
    """
    The request handler class for our server.
    It is instantiated once per connection to the server.
    """
    def handle(self):
        # self.request is the TCP socket connected to the client
        print(f"[*] Connection from {self.client_address[0]}:{self.client_address[1]}")

        try:
            # Receive the data in small chunks. We assume the client sends all data at once.
            # A newline character can be a good delimiter for messages.
            data = self.request.recv(4096).strip()
            
            if not data:
                print(f"[*] No data received from {self.client_address[0]}. Closing connection.")
                return

            print(f"[*] Received {len(data)} bytes from {self.client_address[0]}")

            # The data processing logic remains the same
            p = re.compile(r'(?<!\\)\'')
            raw_data = data.decode('ascii')
            raw_data = p.sub(r'\"', raw_data)
            data_dict = json.loads(raw_data)
            dataframe_instance = df.decrypt(data_dict)

            if dataframe_instance is None:
                print("[!] Decryption failed for received data.")
                return
            
            print(f"[*] Successfully decrypted data for player: {dataframe_instance.plyrName}")
            # Save the data to the database
            Database.save_ddataframe(dataframe_instance)
            
        except json.JSONDecodeError:
            print(f"[!] Failed to decode JSON from {self.client_address[0]}. Raw data: {data}")
        except Exception as e:
            traceback.print_exc()
            print(f"[!] Could not process message from {self.client_address[0]}: {e}")
        finally:
            print(f"[*] Closing connection with {self.client_address[0]}")
            # No explicit close needed here, BaseRequestHandler handles it.


def start_socket_server(host='localhost', port=9999):
    """
    Starts the TCP socket server in a blocking call.
    This function is intended to be run in a separate thread.
    """
    print(f"[*] Starting data collection server on {host}:{port}...")
    
    # Ensure the database and tables exist
    Database.create_database()

    # Create the server, binding to localhost on port 9999
    # Using ThreadingMixIn allows the server to handle multiple connections
    server = socketserver.ThreadingTCPServer((host, port), GameDataTCPHandler)

    # Start a thread that will check for the shutdown event
    def server_shutdown_checker():
        # This loop will run until the main thread signals shutdown
        while not shutdown_server_event.is_set():
            # Sleep for a short time to avoid busy-waiting
            shutdown_server_event.wait(1)
        
        print("[*] Shutdown event received. Shutting down server...")
        # This will unblock the server.serve_forever() call
        server.shutdown()

    shutdown_thread = threading.Thread(target=server_shutdown_checker)
    shutdown_thread.daemon = True
    shutdown_thread.start()

    try:
        # Activate the server; this will keep running until shutdown() is called
        server.serve_forever()
    except (KeyboardInterrupt, SystemExit):
        print("\n[!] Server interrupted.")
    finally:
        print("[*] Closing server socket.")
        server.server_close()
        print("[*] Server stopped.")

def stop_socket_server():
    """
    Signals the socket server to shut down.
    """
    print("[*] Signaling socket server to stop...")
    shutdown_server_event.set()
