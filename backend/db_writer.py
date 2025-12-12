# backend/db_writer.py
import socket
import psycopg2 # pip install psycopg2-binary
import json
import time

# --- Configuration ---
SERVER_HOST = '127.0.0.1'
SERVER_PORT = 9999

# --- Database Configuration ---
DB_NAME = "playerData"
DB_USER = "postgres"
DB_PASS = "your_password" # IMPORTANT: Change this
DB_HOST = "localhost"
DB_PORT = "5432"

def get_db_connection():
    """Establishes a connection to the PostgreSQL database."""
    try:
        conn = psycopg2.connect(
            dbname=DB_NAME,
            user=DB_USER,
            password=DB_PASS,
            host=DB_HOST,
            port=DB_PORT
        )
        return conn
    except psycopg2.OperationalError as e:
        print(f"[DB] Could not connect to database: {e}")
        return None

def setup_database():
    """Creates database and table if they don't exist."""
    conn = get_db_connection()
    if not conn: return

    try:
        # Create table if it doesn't exist
        cursor = conn.cursor()
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS player_stats (
                id SERIAL PRIMARY KEY,
                player_name VARCHAR(255),
                timestamp TIMESTAMP,
                x FLOAT,
                y FLOAT,
                z FLOAT,
                health INTEGER,
                level INTEGER,
                experience INTEGER
            );
        """)
        conn.commit()
        print("[DB] Database and table are ready.")
        cursor.close()
    except psycopg2.Error as e:
        print(f"[DB ERROR] Setup failed: {e}")
    finally:
        if conn:
            conn.close()

def write_to_database(data):
    """Writes a single data point to the database."""
    conn = get_db_connection()
    if not conn:
        return

    try:
        cursor = conn.cursor()
        # IMPORTANT: Adjust column names and data access to match your JSON structure
        insert_query = """
            INSERT INTO player_stats (player_name, timestamp, x, y, z, health, level, experience)
            VALUES (%s, %s, %s, %s, %s, %s, %s);
        """
        values = (
            data.get("playerName"),
            data.get("timestamp"),
            data.get("pos", {}).get("x"),
            data.get("pos", {}).get("y"),
            data.get("pos", {}).get("z"),
            data.get("health"),
            data.get("level"),
            data.get("experience")
        )
        cursor.execute(insert_query, values)
        conn.commit()
        # print(f"[DB] Wrote data for {data.get('playerName')}")
    except psycopg2.Error as e:
        print(f"[DB ERROR] Could not write data: {e}")
    finally:
        if conn:
            cursor.close()
            conn.close()

def start_writer():
    """Connects to the server and continuously writes received data to the database."""
    setup_database()
    print("[WRITER] Database writer is starting...")
    
    while True:
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((SERVER_HOST, SERVER_PORT))
                print(f"[WRITER] Connected to server at {SERVER_HOST}:{SERVER_PORT}.")
                
                # This simple writer just connects and receives data in a loop.
                # A more advanced version could be a proper TCP client that handles disconnections.
                while True:
                    data = s.recv(4096)
                    if not data:
                        print("[WRITER] Server disconnected. Reconnecting...")
                        break
                    
                    try:
                        decoded_data = json.loads(data.decode('utf-8'))
                        write_to_database(decoded_data)
                    except (json.JSONDecodeError, UnicodeDecodeError) as e:
                        print(f"[WRITER ERROR] Could not decode data: {e}")

        except ConnectionRefusedError:
            print("[WRITER] Could not connect to server. Retrying in 5 seconds...")
            time.sleep(5)

if __name__ == "__main__":
    start_writer()