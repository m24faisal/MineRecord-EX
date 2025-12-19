## backend/backend_controller.py
import sys
import os
import uuid
import threading
import traceback
from datetime import datetime
import pandas as pd
import psycopg2 # <-- IMPORTANT: IMPORT DIRECTLY

# Import your existing modules
from dataFormat import DataSnap
from screenRecord import start_ffmpeg_process
# Import the global database instance directly
from .receive import db_instance
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
    """
    A direct, no-nonsense export function to bypass all caching issues.
    """
    print(f"[*] --- DIRECT EXPORT FUNCTION CALLED ---")
    print(f"[*] Player: {player_name}, Path: {export_path}")

    # --- HARDCODED CONNECTION STRING TO BYPASS ALL CONFIG FILES ---
    # This is the only way to be 100% sure what port we are using.
    # I am using your correct credentials here.
    conn_string = "dbname='playerData' user='postgres' password='a' host='localhost' port=5432"
    print(f"[*] Attempting to connect with hardcoded string: {conn_string}")

    conn = None
    try:
        # This is the ONLY line that can generate that error message.
        conn = psycopg2.connect(conn_string)
        print("[*] CONNECTION SUCCESSFUL!")

        # If we get here, the connection worked. Now do the export.
        query = "SELECT * FROM player_stats WHERE player_name = %s"
        df = pd.read_sql_query(query, conn, params=(player_name,))

        if df.empty:
            return "Info: No data found for player."
        
        # Save to CSV
        os.makedirs(export_path, exist_ok=True)
        output_path = os.path.join(export_path, f"{player_name}_data.csv")
        df.to_csv(output_path, index=False)
        
        return f"Success: Exported data to {output_path}"

    except psycopg2.OperationalError as e:
        # THIS is the block that will catch the error.
        print(f"[!!!] DIRECT CONNECTION FAILED: {e}")
        return f"Error: Could not connect to database. Details: {e}"
    except Exception as e:
        print(f"[!!!] An unexpected error occurred: {e}")
        return f"Error: An unexpected error occurred. Details: {e}"
    finally:
        if conn:
            conn.close()
            print("[*] Connection closed.")

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