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

# Import the new library file
from receive import listen_for_messages
from screenRecord import start_ffmpeg_process
from db_export import export_player_data_to_csv

# Global dictionary to track active recordings
active_recordings = {}

# Global dictionary to track data collection threads
data_collection_threads = {}

def start_recording(game_name, game_path, recording_path, enable_data_collection=False):
    """Start recording game data and screen. Called from C++."""
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
        
        # Start data collection in a separate thread if enabled
        if enable_data_collection:
            print(f"[*] Data collection enabled for {game_name}. Starting listener thread.")
            # Create a threading.Event to signal the thread to stop
            stop_event = threading.Event()
            
            # Create and start the thread
            data_thread = threading.Thread(target=listen_for_messages, args=(stop_event,))
            data_thread.daemon = True
            data_thread.start()
            
            # Store the thread and its stop event in our global dictionary
            data_collection_threads[recording_id] = {
                "thread": data_thread,
                "stop_event": stop_event
            }
            active_recordings[recording_id]["data_thread"] = data_thread
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
        
        # --- Gracefully terminate the data collection thread ---
        if recording_id in data_collection_threads:
            collection_info = data_collection_threads[recording_id]
            print(f"[*] Signaling data collection thread for {recording['game_name']} to stop.")
            collection_info["stop_event"].set() # Signal the event to stop the loop
            # Wait for the thread to finish its work
            collection_info["thread"].join(timeout=10)
            print(f"[*] Data collection thread for {recording['game_name']} has stopped.")
            # Remove it from our tracking dictionary
            del data_collection_threads[recording_id]
        
        # --- Terminate the screen recording process ---
        screen_process = recording.get("screen_process")
        if screen_process and screen_process.poll() is None:
            print(f"[*] Stopping screen recording for {recording['game_name']}...")
            # Send 'q' followed by a newline to the process's stdin
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
    
    # First, stop all data collection threads
    if data_collection_threads:
        print(f"[*] SHUTDOWN: Found {len(data_collection_threads)} data collection threads to stop.")
        for recording_id, collection_info in list(data_collection_threads.items()):
            print(f"[*] SHUTDOWN: Stopping data collection thread for recording {recording_id}.")
            collection_info["stop_event"].set()
            collection_info["thread"].join(timeout=10)
            print(f"[*] SHUTDOWN: Data collection thread for recording {recording_id} has stopped.")
    
    # Then, stop all recordings
    if not active_recordings:
        print("[*] SHUTDOWN: No active recordings to stop.")
        return
    
    # Create a list of IDs to avoid modifying the dictionary while iterating
    recording_ids_to_stop = list(active_recordings.keys())
    print(f"[*] SHUTDOWN: Found {len(recording_ids_to_stop)} recordings to stop.")
    
    for recording_id in recording_ids_to_stop:
        print(f"[*] SHUTDOWN: Stopping recording ID: {recording_id}")
        stop_recording(recording_id)
    
    print("[*] SHUTDOWN: All recordings have been stopped.")