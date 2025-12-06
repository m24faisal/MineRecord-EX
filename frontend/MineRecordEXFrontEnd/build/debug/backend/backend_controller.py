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

# Import the simple FFmpeg launcher
from screenRecord import start_ffmpeg_process

# Global dictionary to track active recordings
active_recordings = {}

def export_player_data(player_name, export_path):
    """Exports all data for a given player from the database to a CSV file."""
    try:
        # Import the new export script
        from db_export import export_player_data_to_csv
        
        # Ensure the export directory exists
        os.makedirs(export_path, exist_ok=True)
        
        # Call the export function
        result_path = export_player_data_to_csv(player_name, export_path)
        
        if result_path:
            return f"Successfully exported data for {player_name} to {result_path}"
        else:
            return f"Failed to export data for {player_name}. No data was found."
            
    except Exception as e:
        return f"Error during export: {str(e)}"


def shutdown_all():
    """Gracefully stops all active recordings and cleans up processes."""
    print("!!! SHUTDOWN: Shutdown signal received. Stopping all active recordings...")
    
    if not active_recordings:
        print("!!! SHUTDOWN: No active recordings to stop.")
        return

    # Create a list of IDs to avoid modifying the dictionary while iterating
    recording_ids_to_stop = list(active_recordings.keys())
    print(f"!!! SHUTDOWN: Found {len(recording_ids_to_stop)} recordings to stop.")
    
    for recording_id in recording_ids_to_stop:
        print(f"!!! SHUTDOWN: Stopping recording ID: {recording_id}")
        stop_recording(recording_id)
    
    print("!!! SHUTDOWN: All recordings have been stopped.")

def start_recording(game_name, game_path, recording_path, enable_data_collection=False):
    """Start recording game data and screen. Called from C++."""
    try:
        recording_id = str(uuid.uuid4())
        
        recording_dir = os.path.join(recording_path, game_name)
        os.makedirs(recording_dir, exist_ok=True)
        
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        video_filename = f"{game_name}_{timestamp}.mp4"
        video_path = os.path.join(recording_dir, video_filename)
        
        # --- Launch FFmpeg using our new launcher ---
        print(f"Starting screen recording for {game_name}...")
        screen_process = start_ffmpeg_process(video_path)
        
        if not screen_process:
            return "Error: Failed to start FFmpeg process."

        active_recordings[recording_id] = {
            "game_name": game_name,
            "game_path": game_path,
            "recording_path": recording_dir,
            "video_path": video_path,
            "start_time": datetime.now(),
            "status": "active",
            "screen_process": screen_process,
            "data_thread": None,
            "enable_data_collection": enable_data_collection
        }
        
        
        # Start data collection in a separate thread if enabled
        if enable_data_collection:
            data_thread = threading.Thread(target=start_data_collection, args=(recording_id,))
            data_thread.daemon = True
            data_thread.start()
            active_recordings[recording_id]["data_thread"] = data_thread
        
        # --- DEBUG: Print the state of the dictionary ---
        print(f"PYTHON: After starting, active_recordings contains: {list(active_recordings.keys())}")
        
        return recording_id
    except Exception as e:
        return f"Error: Failed to start recording: {str(e)}"
        
def stop_recording(recording_id):
    """Stop recording game data and screen. Called from C++."""
     # --- DEBUG: Print the state of the dictionary BEFORE checking ---
    print(f"PYTHON: stop_recording called with ID: {recording_id}")
    print(f"PYTHON: Before checking, active_recordings contains: {list(active_recordings.keys())}")

    try:
        if recording_id not in active_recordings:
            print(f"PYTHON: ERROR: Recording ID '{recording_id}' not found in dictionary!")
            return "Error: Recording not found"
            
        recording = active_recordings[recording_id]

        # --- THIS IS THE FIX ---
        # If the recording is already marked as stopped, just return success.
        # This prevents errors if the function is called multiple times.
        if recording.get("status") == "stopped":
            print(f"Python: Recording {recording_id} is already stopped. Ignoring redundant stop request.")
            return f"Recording stopped for {recording['game_name']}"

        # --- Original logic to terminate the process ---
        recording["status"] = "stopped"
        recording["end_time"] = datetime.now()
        
        screen_process = recording.get("screen_process")
        if screen_process and screen_process.poll() is None:
            print(f"Python: Stopping screen recording for {recording['game_name']}...")
            # Send 'q' followed by a newline to the process's stdin
            screen_process.communicate(input='q\n'.encode())
            print("Python: Screen recording process stopped gracefully.")
        
        return f"Recording stopped for {recording['game_name']}"
    except Exception as e:
        return f"Error: Failed to stop recording: {str(e)}"
def start_data_collection(recording_id):
    """Starts collecting game data. This is a placeholder for your RabbitMQ logic."""
    recording = active_recordings[recording_id]
    print(f"Starting data collection for {recording['game_name']} (ID: {recording_id})")
    
    # This is where you would adapt the logic from your `receive.py` file.
    while active_recordings.get(recording_id, {}).get("status") == "active":
        # ... your data collection logic here ...
        import time
        time.sleep(5)
        
    print(f"Data collection stopped for {recording['game_name']}.")