# backend/db_export.py
import psycopg2
import csv
import os
from datetime import datetime
from dbManage import Database

def export_player_data_to_csv(player_name, output_path):
    """
    Queries the database for all data related to a player and writes it to a CSV file.
    """
    print(f"Starting export for player: {player_name}")
    
    # Construct the output filename
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    csv_filename = f"{player_name}_{timestamp}.csv"
    full_path = os.path.join(output_path, csv_filename)

    try:
        # Connect to the database
        conn = psycopg2.connect(**Database.APP_DB_CONFIG)
        cursor = conn.cursor()

        # --- Query for Player Data ---
        # This joins the DATA, ITEMS, and EFFECTS tables to get a complete view
        query = """
            SELECT 
                d.date, d.time, d.plyrname, d.plyrlocation, d.plyrhealth, 
                d.plyrhunger, d.plyrsat, d.plyrview, d.plyrfacing, 
                d.plyrselectedslot, d.plyrselecteditem, d.plyrridestate, d.plyrridevehicle,
                d.plyrmomentum, i.plyrarmor, i.plyroffhand
            FROM DATA d
            WHERE d.plyrname = %s
            ORDER BY d.date, d.time;
        """
        
        cursor.execute(query, (player_name,))
        player_data = cursor.fetchall()

        if not player_data:
            print(f"No data found for player: {player_name}")
            return None

        # --- Write to CSV ---
        # Define the headers for the CSV file
        headers = [
            'date', 'time', 'plyrname', 'plyrlocation', 'plyrhealth',
            'plyrhunger', 'plyrsat', 'plyrview', 'plyrfacing',
            'plyrselectedslot', 'plyrselecteditem', 'plyrridestate', 'plyrridevehicle', 'plyrmomentum',
            'plyrarmor', 'plyroffhand'
        ]

        print(f"Writing data to CSV: {full_path}")
        with open(full_path, 'w', newline='', encoding='utf-8') as csvfile:
            writer = csv.writer(csvfile)
            writer.writerow(headers)
            writer.writerows(player_data)

        print(f"Successfully exported {len(player_data)} records for {player_name}.")
        return full_path

    except psycopg2.Error as e:
        print(f"Database error: {e}")
        return None
    except Exception as e:
        print(f"An unexpected error occurred: {e}")
        return None
    finally:
        if 'conn' in locals() and conn is not None:
            cursor.close()
            conn.close()
            print("Database connection closed.")

if __name__ == "__main__":
    # Example usage for testing
    # This would be called from your C++ backend
    export_player_data_to_csv("Playername", "C:/path/to/your/exports")