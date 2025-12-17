# backend/db_writer.py
import socket
import psycopg2
import json
import time
from .config import DB_CONFIG, DATA_SERVER_CONFIG

def get_db_connection():
    """Establishes a connection to the PostgreSQL database."""
    try:
        dsn = (
            f"dbname={DB_CONFIG['database']} "
            f"user={DB_CONFIG['user']} "
            f"password={DB_CONFIG['password']} "
            f"host={DB_CONFIG['host']} "
            f"port={DB_CONFIG['port']}"
        )
        
        conn = psycopg2.connect(dsn)
        return conn
    except psycopg2.OperationalError as e:
        print(f"[DB] Could not connect to database: {e}")
        return None

def setup_database():
    """Creates the database and table if they don't exist."""
    print("[SETUP] Creating database and table...")
    conn = get_db_connection()
    if not conn:
        print("[SETUP ERROR] Could not get a database connection.")
        return

    try:
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
        print("[SETUP] Table 'player_stats' is ready.")
        cursor.close()
        return conn
    except Exception as e:
        print(f"[SETUP ERROR] An error occurred: {e}")
        return None

def write_to_database(data):
    """Writes a single data point to the database."""
    conn = get_db_connection()
    if not conn:
        print("[!!!] Cannot insert data: No database connection.")
        return

    try:
        cursor = conn.cursor()
        insert_query = """
            INSERT INTO player_stats (player_name, timestamp, x, y, z, health, level, experience)
                VALUES (%s, %s, %s, %s, %s, %s, %s, %s);
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
        if cursor:
            cursor.close()

def start_writer():
    """Connects to the server and continuously writes received data to the database."""
    setup_database()
    print("[WRITER] Database writer is starting...")
    
    while True:
        conn = None
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((DATA_SERVER_CONFIG['host'], DATA_SERVER_CONFIG['port']))
                print(f"[WRITER] Connected to server at {DATA_SERVER_CONFIG['host']}:{DATA_SERVER_CONFIG['port']}.")
                
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
        finally:
            if 's' in locals():
                s.close()
                print("[WRITER] Socket closed.")

if __name__ == "__main__":
    start_writer()