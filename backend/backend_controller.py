import sys
import os
import uuid
import threading
from datetime import datetime
import subprocess

# Add the parent directory to the path to import backend modules
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# Import your existing modules
from dataFormat import DataSnap
from dbManage import Database
from screenRecord import record_screen

# Global dictionary to track active recordings
active_recordings = {}

def start_recording(game_name, game_path, recording_path, enable_data_collection=False):
    """Start recording game data and screen. Called from C++."""
    try:
        recording_id = str(uuid.uuid4())
        
        recording_dir = os.path.join(recording_path, game_name)
        os.makedirs(recording_dir, exist_ok=True)
        
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        video_filename = f"{game_name}_{timestamp}.mp4"
        video_path = os.path.join(recording_dir, video_filename)
        
        active_recordings[recording_id] = {
            "game_name": game_name,
            "game_path": game_path,
            "recording_path": recording_dir,
            "video_path": video_path,
            "start_time": datetime.now(),
            "status": "active",
            "screen_process": None,
            "data_thread": None,
            "enable_data_collection": enable_data_collection
        }
        
        # Start screen recording in a separate thread to not block the C++ call
        screen_thread = threading.Thread(target=start_screen_recording, args=(recording_id,))
        screen_thread.daemon = True
        screen_thread.start()
        
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
        
        # Stop screen recording
        if recording.get("screen_process") and recording["screen_process"].poll() is None:
            recording["screen_process"].terminate()
            try:
                recording["screen_process"].wait(timeout=10)
            except subprocess.TimeoutExpired:
                recording["screen_process"].kill()
        
        return f"Recording stopped for {recording['game_name']}"
    except Exception as e:
        return f"Error: Failed to stop recording: {str(e)}"

def start_screen_recording(recording_id):
    """Starts the FFmpeg screen recording process."""
    recording = active_recordings[recording_id]
    
    ffmpeg_cmd = [
        "ffmpeg",
        "-f", "gdigrab",  # Use gdigrab for Windows
        "-framerate", "60",
        "-i", "desktop",
        "-vcodec", "libx264",
        "-preset", "fast",
        "-y",  # Overwrite output file if it exists
        recording["video_path"]
    ]
    
    try:
        # Use CREATE_NO_WINDOW to hide the console window on Windows
        process = subprocess.Popen(ffmpeg_cmd, creationflags=subprocess.CREATE_NO_WINDOW)
        recording["screen_process"] = process
    except Exception as e:
        print(f"Error starting screen recording: {e}")
        
def start_data_collection(recording_id):
    """Starts collecting game data. This is a placeholder for your RabbitMQ logic."""
    recording = active_recordings[recording_id]
    print(f"Starting data collection for {recording['game_name']} (ID: {recording_id})")
    
    # This is where you would adapt the logic from your `receive.py` file.
    # For now, it's a placeholder that just waits.
    while active_recordings.get(recording_id, {}).get("status") == "active":
        # In a real implementation, you would:
        # 1. Connect to RabbitMQ
        # 2. Listen for messages
        # 3. Decrypt them using dataFormat.decrypt()
        # 4. Save them using dbManage.save_ddataframe()
        import time
        time.sleep(5) # Wait 5 seconds before checking status again
        
    print(f"Data collection stopped for {recording['game_name']}.")