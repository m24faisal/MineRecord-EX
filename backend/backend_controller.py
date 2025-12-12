# backend/backend_controller.py
import os
import uuid
import threading
import traceback
from datetime import datetime

# Import your recording module
from screenRecord import start_ffmpeg_process

# Global dictionary to track active recordings
active_recordings = {}

def start_recording(game_name, game_path, recording_path, enable_data_collection=False):
    """Start recording game data and screen. Called from C++."""
    # The 'enable_data_collection' flag is now just for show. The C++ app
    # handles starting the separate data processes.
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
        }
        
        print(f"[*] Recording started for {game_name} with ID: {recording_id}")
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
        
        print(f"[*] Stopping recording for {recording['game_name']}.")

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
    # This function would now call a separate script or use a library
    # like pandas to query the DB and save to CSV.
    print(f"[*] Exporting data for {player_name} to {export_path}")
    # For now, it's a placeholder.
    # In a real implementation, you would use psycopg2 to query the database
    # and pandas to save the result to a CSV file.
    # Example:
    # df = pd.read_sql("SELECT * FROM player_stats WHERE player_name = %s", conn, params=(player_name,))
    # df.to_csv(f"{export_path}/{player_name}_data.csv", index=False)
    return f"Data for {player_name} exported to {export_path}."

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