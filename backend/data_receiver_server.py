# backend/data_receiver_server.py
import socket
import threading
import json
import sys
from .config import DB_CONFIG, DATA_SERVER_CONFIG

# --- Configuration ---
HOST = '127.0.0.1'
PORT = 9999

# --- List of connected clients ---
clients = []
lock = threading.Lock()

def handle_client(conn, addr):
    """Handles a single client connection."""
    print(f"[NEW CONNECTION] {addr} connected.")
    with lock:
        clients.append(conn)

    try:
        while True:
            data = conn.recv(4096)
            if not data:
                break
            
            try:
                decoded_data = json.loads(data.decode('utf-8'))
                player_info = json.loads(decoded_data)
                print(f"[DATA BROADCAST] {player_info}")
                
            except (ConnectionResetError, json.JSONDecodeError, UnicodeDecodeError) as e:
                print(f"[{addr}] Error decoding data: {e}")

    except ConnectionResetError:
        pass
    finally:
        with lock:
            if conn in clients:
                clients.remove(conn)
        conn.close()
        print(f"[DISCONNECT] {addr} disconnected.")

def start_server():
    """Main function to start the server."""
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    try:
        server_socket.bind((HOST, PORT))
        server_socket.listen()
        print(f"[*] Standalone Data Server is listening on {HOST}:{PORT}")
        
        while True:
            conn, addr = server_socket.accept()
            thread = threading.Thread(target=handle_client, args=(conn, addr), daemon=True)
            thread.start()
            
    except KeyboardInterrupt:
        print("\n[*] Server is shutting down.")
    finally:
        server_socket.close()