# backend/database.py
import psycopg2
from config import DB_CONFIG

class Database:
    """A unified class to manage PostgreSQL database operations for detailed player stats only."""
    
    def __init__(self):
        """Initializes the Database class. Does not connect yet."""
        self.conn = None
        print("[DB] Database class initialized. Connection not yet made.")

    def connect(self):
        """Establishes a connection to the PostgreSQL database using a DSN."""
        dsn = (
            f"dbname={DB_CONFIG['database']} "
            f"user={DB_CONFIG['user']} "
            f"password={DB_CONFIG['password']} "
            f"host={DB_CONFIG['host']} "
            f"port={DB_CONFIG['port']}"
        )
        try:
            self.conn = psycopg2.connect(dsn)
            print("[*] Database connection successful.")
            return True
        except psycopg2.OperationalError as e:
            print(f"[!!!] Could not connect to database: {e}")
            self.conn = None
            return False

    def setup_database_and_table(self):
        """Creates the database and only the detailed player stats table if they don't exist."""
        db_name = DB_CONFIG['database']
        
        # Step 1: Connect to 'postgres' database to create our target database
        try:
            conn_default = psycopg2.connect(
                dbname="postgres",
                user=DB_CONFIG['user'],
                password=DB_CONFIG['password'],
                host=DB_CONFIG['host'],
                port=DB_CONFIG['port']
            )
            conn_default.autocommit = True
            cursor = conn_default.cursor()
            
            # Check if database exists
            cursor.execute("SELECT 1 FROM pg_database WHERE datname = %s", (db_name,))
            if not cursor.fetchone():
                print(f"[*] Database '{db_name}' does not exist. Creating it...")
                cursor.execute(f'CREATE DATABASE "{db_name}"')
                print(f"[*] Database '{db_name}' created successfully.")
            else:
                print(f"[*] Database '{db_name}' already exists.")
                
            cursor.close()
            conn_default.close()
            
        except psycopg2.Error as e:
            print(f"[!!!] Could not check/create database '{db_name}': {e}")
            return False

        # Step 2: Connect to the target database and create the table
        if not self.connect():
            print(f"[!!!] Failed to connect to database '{db_name}' after creation.")
            return False

        try:
            conn = self.conn
            assert conn is not None
            cursor = conn.cursor()
            cursor.execute("""
                CREATE TABLE IF NOT EXISTS player_detailed_stats (
                    id SERIAL PRIMARY KEY,
                    player_name VARCHAR(255),
                    timestamp TIMESTAMP,
                    fps FLOAT,
                    plyrLocation VARCHAR(255),
                    plyrHealth FLOAT,
                    plyrInventory TEXT,
                    plyrArmor VARCHAR(255),
                    plyrOffhand VARCHAR(255),
                    plyrStatus TEXT,
                    plyrHunger FLOAT,
                    plyrSat FLOAT,
                    plyrView VARCHAR(255),
                    plyrFacing VARCHAR(255),
                    plyrSelectedSlot INTEGER,
                    plyrSelectedItem VARCHAR(255),
                    plyrRideState BOOLEAN,
                    plyrRideVehicle VARCHAR(255),
                    plyrMomentum FLOAT
                );
            """)
            conn.commit()
            print("[*] Table 'player_detailed_stats' is ready.")
            cursor.close()
            return True
            
        except psycopg2.Error as e:
            print(f"[DB ERROR] Could not create table: {e}")
            return False
        finally:
            if self.conn:
                self.conn.close()
            self.conn = None

    def insert_detailed_data(self, player_name, data_dict):
        """Inserts a full data point into the player_detailed_stats table."""
        if not self.conn or self.conn.closed:
            print("[!!!] insert_detailed_ No valid database connection. Cannot insert data.")
            return
        try:
            conn = self.conn
            assert conn is not None
            cursor = conn.cursor()
            insert_query = """
                INSERT INTO player_detailed_stats (
                    player_name, timestamp, fps, plyrLocation, plyrHealth, plyrInventory,
                    plyrArmor, plyrOffhand, plyrStatus, plyrHunger, plyrSat, plyrView,
                    plyrFacing, plyrSelectedSlot, plyrSelectedItem, plyrRideState,
                    plyrRideVehicle, plyrMomentum
                )
                VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s);
            """
            values = (
                player_name,
                data_dict.get("timestamp"),
                data_dict.get("fps"),
                data_dict.get("plyrLocation"),
                data_dict.get("plyrHealth"),
                data_dict.get("plyrInventory"),
                data_dict.get("plyrArmor"),
                data_dict.get("plyrOffhand"),
                data_dict.get("plyrStatus"),
                data_dict.get("plyrHunger"),
                data_dict.get("plyrSat"),
                data_dict.get("plyrView"),
                data_dict.get("plyrFacing"),
                data_dict.get("plyrSelectedSlot"),
                data_dict.get("plyrSelectedItem"),
                data_dict.get("plyrRideState"),
                data_dict.get("plyrRideVehicle"),
                data_dict.get("plyrMomentum")
            )
            cursor.execute(insert_query, values)
            conn.commit()
        except psycopg2.Error as e:
            print(f"[DB ERROR] Could not write data to player_detailed_stats: {e}")
        finally:
            if 'cursor' in locals() and cursor:
                cursor.close()

    def get_all_detailed_player_data(self, player_name):
        """Fetches all detailed data for a given player from the detailed table."""
        if not self.conn or self.conn.closed:
            print("[!!!] get_all_detailed_player_ No valid database connection. Cannot fetch data.")
            return None
        try:
            conn = self.conn
            assert conn is not None
            cursor = conn.cursor()
            query = """
                SELECT player_name, timestamp, fps, plyrLocation, plyrHealth, plyrInventory,
                       plyrArmor, plyrOffhand, plyrStatus, plyrHunger, plyrSat, plyrView,
                       plyrFacing, plyrSelectedSlot, plyrSelectedItem, plyrRideState,
                       plyrRideVehicle, plyrMomentum
                FROM player_detailed_stats 
                WHERE player_name = %s 
                ORDER BY timestamp;
            """
            cursor.execute(query, (player_name,))
            data = cursor.fetchall()
            cursor.close()
            return data
        except psycopg2.Error as e:
            print(f"[DB ERROR] Could not fetch data from player_detailed_stats: {e}")
            return None
        finally:
            if 'cursor' in locals() and cursor:
                cursor.close()

    def close(self):
        """Closes the database connection."""
        if self.conn is not None:
            print("[*] Closing database connection.")
            if hasattr(self.conn, 'closed') and self.conn.closed == 0:
                self.conn.close()
            else:
                print("[*] Connection was already closed.")
            self.conn = None