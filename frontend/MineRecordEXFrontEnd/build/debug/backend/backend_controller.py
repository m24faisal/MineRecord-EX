# backend/backend_controller.py
import sys
import os
import uuid
import threading
import traceback
from datetime import datetime
import pandas as pd
# Import your existing modules
from screenRecord import start_ffmpeg_process
from database import Database

# --- NOTE: This file no longer manages the data server or database directly ---
# It now calls the functions from the new, unified scripts.

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
    """
    Fetches detailed data for a player using the Database class and exports it to a CSV file.
    This function handles the CSV creation logic.
    """
    print(f"[*] export_player_data called with player='{player_name}', path='{export_path}'")
    # Create a temporary database instance for this operation
    db = Database()
    # Connect to the database
    if not db.connect():
        return "Error: Could not connect to the database."
    # --- CHANGE: Fetch the raw data using the NEW detailed method ---
    raw_data = db.get_all_detailed_player_data(player_name)
    # Close the connection
    db.close()
    # --- CSV EXPORT LOGIC ---
    if not raw_data:
        return f"Info: No data found for player '{player_name}'."
    print(f"[*] Successfully retrieved {len(raw_data)} rows of data for '{player_name}'.")
    try:
        # Ensure the export directory exists
        os.makedirs(export_path, exist_ok=True)
        # Create a pandas DataFrame from the raw data
        # --- CHANGE: The column names must match the detailed table ---
        df = pd.DataFrame(raw_data, columns=[
            'player_name', 'timestamp', 'fps', 'plyrLocation', 'plyrHealth', 'plyrInventory', 
            'plyrArmor', 'plyrOffhand', 'plyrStatus', 'plyrHunger', 'plyrSat', 
            'plyrView', 'plyrFacing', 'plyrSelectedSlot', 'plyrSelectedItem', 
            'plyrRideState', 'plyrRideVehicle', 'plyrMomentum'
        ])
        # Create a unique filename
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        output_filename = f"{player_name}_detailed_data_{timestamp}.csv"
        output_path_full = os.path.join(export_path, output_filename)
        # Save the DataFrame to a CSV file
        df.to_csv(output_path_full, index=False)
        # Verify the file was created
        if os.path.exists(output_path_full):
            success_msg = f"Successfully exported detailed data for {player_name} to {output_path_full}"
            print(f"[*] SUCCESS: {success_msg}")
            return success_msg
        else:
            error_msg = f"Error: CSV file not found after save operation. Expected at: {output_path_full}"
            print(f"[EXPORT CRITICAL ERROR: {error_msg}")
            return error_msg
    except Exception as e:
        error_msg = f"Error: An unexpected error occurred while saving CSV: {e}"
        print(f"[EXPORT CRITICAL ERROR: {error_msg}")
        return error_msg

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