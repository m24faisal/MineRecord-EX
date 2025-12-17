# backend/db_export.py
import psycopg2
import pandas as pd
import os
import traceback
from datetime import datetime

# --- THE FIX ---
# Instead of creating a new connection, we import the global 'db_instance'
# that is already connected and managed by the receive.py server.
# This avoids the "Connection refused" error.
from .receive import db_instance

def export_player_data_to_csv(player_name, export_path):
    """
    Queries the database for a player's data and saves it to a CSV file.
    This version uses the existing global database connection.
    Returns the absolute path of the created file on success, or None on failure.
    """
    print(f"[*] Export function called for player: '{player_name}', to path: '{export_path}'")

    # --- STEP 1: Validate Inputs ---
    if not player_name or not export_path:
        print("[EXPORT ERROR] Player name or export path is empty.")
        return None

    # --- STEP 2: Ensure Export Directory Exists ---
    try:
        os.makedirs(export_path, exist_ok=True)
        print(f"[*] Export directory verified/created at: {export_path}")
    except OSError as e:
        print(f"[EXPORT ERROR] Could not create export directory: {e}")
        return None

    # --- STEP 3: Use the Existing Database Connection ---
    # --- THE FIX ---
    # We no longer create a new connection. We use the one from the global db_instance.
    db = db_instance

    # --- DEFENSIVE CHECK ---
    # Ensure the database connection is actually valid before we try to use it.
    if not db or not db.conn or db.conn.closed:
        error_msg = "Error: Database is not connected. Please ensure the data collection service is running before exporting."
        print(f"[EXPORT ERROR] {error_msg}")
        return None

    # Use the existing connection from the db_instance
    conn = db.conn
    df = None # Initialize df to None for the finally block

    try:
        # Use a parameterized query to prevent SQL injection
        query = "SELECT * FROM player_stats WHERE player_name = %s"
        print(f"[*] Executing parameterized query for player: '{player_name}'")
        
        df = pd.read_sql_query(query, conn, params=(player_name,))
        
        if df.empty:
            print(f"[*] No data found for player '{player_name}'. A CSV file will not be created.")
            return None
            
        print(f"[*] Successfully retrieved {len(df)} rows of data for '{player_name}'.")
        
    except Exception as e:
        print(f"[EXPORT ERROR] Failed to query the database: {e}")
        traceback.print_exc() # Print the full traceback for debugging
        return None
    # Note: We don't close the connection here, as it's managed globally.
    # The 'finally' block for closing the connection has been removed.

    # --- STEP 4: Define Output Path and Save ---
    # Added a timestamp to the filename to avoid overwriting previous exports
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    output_filename = f"{player_name}_data_{timestamp}.csv"
    output_path_full = os.path.join(export_path, output_filename)
    
    print(f"[*] Attempting to save CSV to: {output_path_full}")

    try:
        df.to_csv(output_path_full, index=False)
        
        # --- STEP 5: Verify File was Actually Created ---
        if os.path.exists(output_path_full):
            print(f"[*] SUCCESS: Data for '{player_name}' successfully exported to '{output_path_full}'.")
            return os.path.abspath(output_path_full)
        else:
            print(f"[EXPORT CRITICAL ERROR: CSV file not found after save operation. Expected at: {output_path_full}")
            return None
            
    except Exception as e:
        print(f"[EXPORT CRITICAL ERROR: An unexpected error occurred while saving CSV: {e}")
        return None