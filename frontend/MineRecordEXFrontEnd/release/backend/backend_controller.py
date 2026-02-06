# backend/backend_controller.py
import sys
import os
import uuid
from datetime import datetime
import pandas as pd
from database import Database

# ONLY handle data collection - NO SCREEN RECORDING
active_recordings = {}

def _is_shutting_down():
    """Check if Python is in shutdown state"""
    return getattr(sys, '_shutting_down', False)

def start_recording(game_name, game_path, recording_path, enable_data_collection=False):
    """Start data collection only (screen recording handled by C++)."""
    if _is_shutting_down():
        return "Error: Python is shutting down"
    recording_id = "data_" + datetime.now().strftime("%Y%m%d_%H%M%S")
    active_recordings[recording_id] = {
        "game_name": game_name,
        "recording_path": recording_path,
        "status": "active",
        "enable_data_collection": enable_data_collection
    }
    return recording_id

def stop_recording(recording_id):
    """Stop data collection only."""
    if _is_shutting_down():
        return "Error: Python is shutting down"
    if recording_id in active_recordings:
        del active_recordings[recording_id]
    return "Data collection stopped"

def export_player_data(player_name, export_path):
    """Export player data to CSV."""
    if _is_shutting_down():
        return "Error: Python is shutting down"
    print(f"[*] export_player_data called with player='{player_name}', path='{export_path}'")
    db = Database()
    if not db.connect():
        return "Error: Could not connect to the database."
    raw_data = db.get_all_detailed_player_data(player_name)
    db.close()
    if not raw_data:
        return f"Info: No data found for player '{player_name}'."
    try:
        os.makedirs(export_path, exist_ok=True)
        df = pd.DataFrame(raw_data, columns=[
            'player_name', 'timestamp', 'fps', 'plyrLocation', 'plyrHealth', 'plyrInventory', 
            'plyrArmor', 'plyrOffhand', 'plyrStatus', 'plyrHunger', 'plyrSat', 
            'plyrView', 'plyrFacing', 'plyrSelectedSlot', 'plyrSelectedItem', 
            'plyrRideState', 'plyrRideVehicle', 'plyrMomentum'
        ])
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        output_filename = f"{player_name}_detailed_data_{timestamp}.csv"
        output_path_full = os.path.join(export_path, output_filename)
        df.to_csv(output_path_full, index=False)
        if os.path.exists(output_path_full):
            return f"Successfully exported detailed data for {player_name} to {output_path_full}"
        else:
            return f"Error: CSV file not found after save operation. Expected at: {output_path_full}"
    except Exception as e:
        return f"Error: An unexpected error occurred while saving CSV: {e}"

def shutdown_all():
    """Safely clear recordings during shutdown"""
    global active_recordings
    if not _is_shutting_down():
        active_recordings.clear()