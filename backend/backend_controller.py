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

def shutdown_all():
    """Gracefully stops all active recordings and cleans up processes."""
    print("Shutdown signal received. Stopping all active recordings...")
    # Create a list of IDs to avoid modifying the dictionary while iterating
    recording_ids_to_stop = list(active_recordings.keys())
    
    for recording_id in recording_ids_to_stop:
        stop_recording(recording_id)
    print("All recordings stopped.")

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
        
        # --- Gracefully stop the screen recording by sending 'q' ---
        screen_process = recording.get("screen_process")
        if screen_process and screen_process.poll() is None:
            print(f"Stopping screen recording for {recording['game_name']}...")
            # Send 'q' followed by a newline to the process's stdin
            screen_process.communicate(input='q\n'.encode())
            print("Sent 'q' command to FFmpeg. It should exit gracefully.")
        
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