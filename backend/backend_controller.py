# backend/backend_controller.py
import sys
import os
import uuid
import threading
import subprocess
from datetime import datetime

# Add the parent directory to the path to import backend modules
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# Import your existing modules
from dataFormat import DataSnap
from dbManage import Database

# --- CHANGE: Import the new socket server module ---
from receive import start_socket_server, stop_socket_server
from screenRecord import start_ffmpeg_process
from db_export import export_player_data_to_csv

# Global dictionary to track active recordings
active_recordings = {}

# --- CHANGE: A global variable for the server thread ---
# We no longer need to track stop events for each recording, just one for the server.
server_thread = None

def start_recording(game_name, game_path, recording_path, enable_data_collection=False):
    """Start recording game data and screen. Called from C++."""
    global server_thread # Use the global server_thread variable

    try:
        recording_id = str(uuid.uuid4())
        
        recording_dir = os.path.join(recording_path, game_name)
        os.makedirs(recording_dir, exist_ok=True)
        
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        video_filename = f"{game_name}_{timestamp}.mp4"
        video_path = os.path.join(recording_dir, video_filename)
        
        # Launch screen recording
        screen_process = start_ffmpeg_process(video_path)
        if not screen_process:
            return f"Error: Failed to start FFmpeg process."

        active_recordings[recording_id] = {
            "game_name": game_name,
            "game_path": game_path,
            "recording_path": recording_dir,
            "video_path": video_path,
            "start_time": datetime.now(),
            "status": "active",
            "screen_process": screen_process,
            "enable_data_collection": enable_data_collection
        }
        
        # --- CHANGE: Start the socket server if data collection is enabled ---
        if enable_data_collection:
            if server_thread is None or not server_thread.is_alive():
                print(f"[*] Data collection enabled. Starting socket server thread.")
                server_thread = threading.Thread(target=start_socket_server)
                server_thread.daemon = True # Allows main program to exit even if thread is running
                server_thread.start()
            else:
                print("[*] Socket server is already running.")
        else:
            print(f"[*] Data collection disabled for {game_name}.")
        
        return recording_id
    except Exception as e:
        return f"Error: Failed to start recording: {str(e)}"
        
def stop_recording(recording_id):
    """Stop recording game data and screen. Called from C++."""
    try:
        if recording_id not in active_recordings:
            return "Error: Recording not found"
            
        recording = active_recordings[recording_id]
        recording["status"] = "stopped"
        recording["end_time"] = datetime.now()
        
        # --- CHANGE: Check if we should stop the server ---
        # We only stop the server if there are no other active recordings that need it.
        should_stop_server = True
        for other_rec_id, other_rec in active_recordings.items():
            if other_rec_id != recording_id and other_rec.get("enable_data_collection", False):
                should_stop_server = False
                break
        
        if should_stop_server:
            print("[*] No other recordings need data collection. Stopping socket server.")
            stop_socket_server()
            if server_thread and server_thread.is_alive():
                server_thread.join(timeout=5) # Wait for the server to finish

        # --- Terminate the screen recording process (this part is the same) ---
        screen_process = recording.get("screen_process")
        if screen_process and screen_process.poll() is None:
            print(f"[*] Stopping screen recording for {recording['game_name']}...")
            screen_process.communicate(input='q\n'.encode())
            print("[*] Screen recording process stopped gracefully.")
        
        return f"Recording stopped for {recording['game_name']}"
    except Exception as e:
        return f"Error: Failed to stop recording: {str(e)}"

def export_player_data(player_name, export_path):
    """Exports all data for a given player from the database to a CSV file."""
    try:
        # Ensure the export directory exists
        os.makedirs(export_path, exist_ok=True)
        
        # Call the function from our new export script
        result_path = export_player_data_to_csv(player_name, export_path)
        
        if result_path:
            return f"Successfully exported data for {player_name} to {result_path}"
        else:
            return f"Failed to export data for {player_name}. No data was found."
            
    except Exception as e:
        return f"Error during export: {str(e)}"

def shutdown_all():
    """Gracefully stops all active recordings and cleans up processes."""
    print("[*] SHUTDOWN: Shutdown signal received. Stopping all active recordings...")
    
    # --- CHANGE: Stop the socket server first ---
    print("[*] SHUTDOWN: Stopping data collection server...")
    stop_socket_server()
    global server_thread
    if server_thread and server_thread.is_alive():
        server_thread.join(timeout=5)
        print("[*] SHUTDOWN: Server thread has stopped.")

    # Then, stop all recordings
    if not active_recordings:
        print("[*] SHUTDOWN: No active recordings to stop.")
        return
    
    # Create a list of IDs to avoid modifying the dictionary while iterating
    recording_ids_to_stop = list(active_recordings.keys())
    print(f"[*] SHUTDOWN: Found {len(recording_ids_to_stop)} recordings to stop.")
    
    for recording_id in recording_ids_to_stop:
        print(f"[*] SHUTDOWN: Stopping recording ID: {recording_id}")
        # We don't need to call stop_recording here to avoid redundant server checks
        # Just stop the FFmpeg process directly
        if recording_id in active_recordings:
            recording = active_recordings[recording_id]
            screen_process = recording.get("screen_process")
            if screen_process and screen_process.poll() is None:
                print(f"[*] SHUTDOWN: Terminating FFmpeg for {recording['game_name']}.")
                screen_process.terminate()

    print("[*] SHUTDOWN: All recordings have been stopped.")