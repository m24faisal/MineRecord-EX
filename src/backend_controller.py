# backend_controller.py
import sys
import os
import uuid
from datetime import datetime
import subprocess

# Add the parent directory to the path to import backend modules
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from dataFormat import DataSnap
from dbManage import Database
from screenRecord import record_screen

# Global dictionary to track active recordings
active_recordings = {}

def start_recording(game_name, game_path, recording_path):
    """Start recording game data and screen"""
    try:
        # Generate a unique recording ID
        recording_id = str(uuid.uuid4())
        
        # Create recording directory if it doesn't exist
        recording_dir = os.path.join(recording_path, game_name)
        os.makedirs(recording_dir, exist_ok=True)
        
        # Generate video filename with timestamp
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        video_filename = f"{game_name}_{timestamp}.mp4"
        video_path = os.path.join(recording_dir, video_filename)
        
        # Store recording info
        active_recordings[recording_id] = {
            "game_name": game_name,
            "game_path": game_path,
            "recording_path": recording_dir,
            "video_path": video_path,
            "start_time": datetime.now(),
            "status": "active",
            "screen_process": None
        }
        
        # Start screen recording
        start_screen_recording(recording_id)
        
        # Start data collection
        start_data_collection(recording_id)
        
        return recording_id
    except Exception as e:
        return f"Error: Failed to start recording: {str(e)}"
        
def stop_recording(recording_id):
    """Stop recording game data and screen"""
    try:
        if recording_id not in active_recordings:
            return "Error: Recording not found"
            
        recording = active_recordings[recording_id]
        recording["status"] = "stopped"
        recording["end_time"] = datetime.now()
        
        # Stop screen recording if it's running
        if recording["screen_process"] and recording["screen_process"].poll() is None:
            recording["screen_process"].terminate()
            try:
                recording["screen_process"].wait(timeout=10)
            except subprocess.TimeoutExpired:
                recording["screen_process"].kill()
        
        return f"Recording stopped for {recording['game_name']}"
    except Exception as e:
        return f"Error: Failed to stop recording: {str(e)}"
        
def export_data(game_name, export_path, date_range=None):
    """Export game data to a spreadsheet"""
    try:
        # Create export directory if it doesn't exist
        os.makedirs(export_path, exist_ok=True)
        
        # Generate filename with timestamp
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"{game_name}_stats_{timestamp}.csv"
        file_path = os.path.join(export_path, filename)
        
        # Query database for the game data
        # This is a simplified example - you'd need to implement the actual query
        # based on your database schema and requirements
        
        # For now, let's assume we have a function to get the data
        # game_data = Database.get_game_data(game_name, date_range)
        
        # Export to CSV
        # save_to_csv(game_data, file_path)
        
        return f"Data exported successfully to {filename}"
    except Exception as e:
        return f"Error: Export failed: {str(e)}"
        
def start_screen_recording(recording_id):
    """Start screen recording using FFmpeg"""
    recording = active_recordings[recording_id]
    
    ffmpeg_cmd = [
        "ffmpeg",
        "-f", "gdigrab",
        "-framerate", "60",
        "-i", "desktop",
        "-vcodec", "libx264",
        "-preset", "fast",
        "-y",  # Overwrite output file if it exists
        recording["video_path"]
    ]
    
    try:
        process = subprocess.Popen(ffmpeg_cmd, creationflags=subprocess.CREATE_NO_WINDOW)
        recording["screen_process"] = process
    except Exception as e:
        print(f"Error starting screen recording: {e}")
        
def start_data_collection(recording_id):
    """Start collecting game data via RabbitMQ"""
    # This would implement the data collection logic from receive.py
    # but adapted to work with the recording system
    pass