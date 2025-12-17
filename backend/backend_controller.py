# backend/backend_controller.py
import sys
import os
import uuid
import threading
import traceback
from datetime import datetime

# Import your existing modules
from dataFormat import DataSnap
from screenRecord import start_ffmpeg_process
from db_export import export_player_data_to_csv

# --- NOTE: This file no longer manages the data server or database directly ---
# It now calls the functions from db_writer and db_export.

active_recordings = {}
server_thread = None
stop_server_event = threading.Event()

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
        
        print(f"[*] Recording started for {game_name} with ID: {recording_id}")
        if enable_data_collection:
            print(f"[*] Data is being collected by global server for {game_name}.")
        else:
            print(f"[*] Data collection is disabled for {game_name}.")
        
        return recording_id
        
    except Exception as e:
        return f"Error: Failed to start recording: {str(e)}"
        
def stop_recording(recording_id):
    """Stop recording game data and screen. Called from C++."""
    try:
        if recording_id not in active_recordings:
            return f"Error: Recording with ID {recording_id} not found."
            
        recording = active_recordings[recording_id]
        recording["status"] = "stopped"
        recording["end_time"] = datetime.now()
        
        print(f"[*] Stopping recording for {recording['game_name']}. Data collection is unaffected.")

        # Terminate screen recording process
        screen_process = recording.get("screen_process")
        if screen_process and screen_process.poll() is None:
            print(f"[*] Stopping screen recording for {recording['game_name']}...")
            screen_process.communicate(input=b'q')
            print("[*] Screen recording process stopped gracefully.")
        
        del active_recordings[recording_id]
        return f"Recording {recording_id} for {recording['game_name']} stopped."
    except Exception as e:
        return f"Error: Failed to stop recording: {str(e)}"

def export_player_data(player_name, export_path):
    """Exports all data for a given player from the database to a CSV file."""
    print(f"[*] Exporting data for {player_name} to {export_path}")
    # Call the function from our new export script
    result_path = export_player_data_to_csv(player_name, export_path)
    
    if result_path:
        return f"Successfully exported data for {player_name} to {result_path}"
    else:
        return f"Failed to export data for {player_name}. No data was found."

def shutdown_all():
    """Gracefully stops all active recordings and cleans up processes."""
    print("[*] SHUTDOWN: Shutdown signal received. Stopping all active recordings...")
    
    if not active_recordings:
        print("[*] SHUTDOWN: No active recordings to stop.")
        return
    
    recording_ids_to_stop = list(active_recordings.keys())
    print(f"[*] SHUTDOWN: Found {len(recording_ids_to_stop)} recordings to stop.")
    
    for recording_id in recording_ids_to_stop:
        print(f"[*] SHUTDOWN: Stopping recording ID: {recording_id}")
        if recording_id in active_recordings:
            recording = active_recordings[recording_id]
            screen_process = recording.get("screen_process")
            if screen_process and screen_process.poll() is None:
                print(f"[*] SHUTDOWN: Terminating FFmpeg for {recording['game_name']}.")
                screen_process.terminate()
    
    active_recordings.clear()
    print("[*] SHUTDOWN: All recordings have been stopped.")