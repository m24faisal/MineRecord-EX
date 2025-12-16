# backend/db_export.py
import psycopg2
import pandas as pd
import os
from dbManage import Database # Use the Database class

def export_player_data_to_csv(player_name, export_path):
    """
    Queries the database for a player's data and saves it to a CSV file.
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

    # --- STEP 3: Connect to Database and Query Data ---
    db = Database() # Instantiate the Database class
    if not db.conn:
        print("[DB] Could not connect to database. See db_manage.py for errors.")
        return None

    try:
        # The key is to print the EXACT query you are sending.
        query = f"SELECT * FROM player_stats WHERE player_name = '{player_name}'"
        print(f"[*] Executing query: {query}")
        
        df = pd.read_sql_query(query, db.conn)
        
        # --- STEP 4: Debug the DataFrame ---
        # This is the most important debugging step.
        print(f"[*] DataFrame head:")
        print(df.head())
        
        # Use the correct exception for an empty DataFrame
        if df.empty:
            print(f"[EXPORT WARNING] No data found for player '{player_name}'. A CSV file will not be created.")
            return None
            
        print(f"[*] Successfully retrieved {len(df)} rows of data for '{player_name}'.")
        
    except Exception as e:
        print(f"[DB ERROR] Failed to query the database: {e}")
        return None
    finally:
        if db.conn:
            db.close()

    # --- STEP 5: Define Output Path and Save ---
    output_filename = f"{player_name}_data.csv"
    # Use os.path.join to create a platform-independent path
    output_path_full = os.path.join(export_path, output_filename)
    
    print(f"[*] Attempting to save CSV to: {output_path_full}")

    try:
        # --- STEP 6: Save to CSV with Error Handling ---
        # The 'index=False' argument prevents pandas from writing a row index.
        df.to_csv(output_path_full, index=False)
        
        # --- STEP 7: Verify File was Actually Created ---
        # This is the most important check.
        if os.path.exists(output_path_full):
            print(f"[*] SUCCESS: Data for '{player_name}' successfully exported to '{output_path_full}'.")
            # Return the full, absolute path to C++ so it can show a message box with the correct location.
            return os.path.abspath(output_path_full)
        else:
            # This case should be rare, but we handle it.
            print(f"[EXPORT CRITICAL ERROR: CSV file not found after save operation. Expected at: {output_path_full}")
            return None
            
    except Exception as e:
        # Catch any other exceptions during the save process.
        print(f"[EXPORT CRITICAL ERROR: An unexpected error occurred while saving CSV: {e}")
        return None