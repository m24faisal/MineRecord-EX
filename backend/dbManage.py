# backend/dbManage.py
from .config import DB_CONFIG
import psycopg2
import os

class Database:
    """A simple class to manage PostgreSQL database connections and queries."""
    
    def __init__(self):
        """Initializes the Database class. Does not connect yet."""
        self.conn = None
        print("[DB] Database class initialized. Connection not yet made.")

    def connect(self):
        """Establishes a connection to the PostgreSQL database using a DSN."""
        # --- THE FIX: Build a DSN string from the config dictionary ---
        # This is the standard, type-safe way to pass parameters.
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
            return self.conn
        except psycopg2.OperationalError as e:
            print(f"[!!!] Could not connect to database: {e}")
            print("[!!!] Please ensure PostgreSQL is running and your credentials in config.py are correct.")
            return None

    def setup_database_and_table(self):
        """Creates the database and table if they don't exist."""
        conn = self.connect() # Use the class method to get a valid connection
        if not conn:
            print("[!!!] setup_database_and_table: Failed to get a valid database connection.")
            return
            
        # Connect to the default 'postgres' db to create a new one
        conn_default = psycopg2.connect(dbname="postgres", user="postgres", password="your_password", host="localhost")
        conn_default.autocommit = True
        cursor = conn_default.cursor()
        
        db_name = "playerData"
        cursor.execute(f"SELECT 1 FROM pg_database WHERE datname = '{db_name}'")
        if not cursor.fetchone():
            cursor.execute(f"CREATE DATABASE {db_name}")
            print(f"[*] Database '{db_name}' created.")
        else:
            print(f"[*] Database '{db_name}' already exists.")
        
        cursor.close()
        conn_default.close()

        # Now connect to the new database to create the table
        self.conn = self.connect() # Re-connect to the new database
        if not self.conn:
            print("[!!!] setup_database_and_table: Failed to reconnect to the new database.")
            return

        cursor = self.conn.cursor()
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
        self.conn.commit()
        print("[*] Table 'player_stats' is ready.")
        cursor.close()

    def insert_data(self, player_name, data):
        """Inserts a new data point into the player_stats table."""
        # --- DEFENSIVE CHECK: Ensure we have a valid connection ---
        if not self.conn or self.conn.closed:
            print("[!!!] insert_data: No valid database connection. Cannot insert data.")
            return
            
        try:
            cursor = self.conn.cursor()
            # IMPORTANT: Adjust column names and data access to match your JSON structure
            insert_query = """
                INSERT INTO player_stats (player_name, timestamp, x, y, z, health, level, experience)
                VALUES (%s, %s, %s, %s, %s, %s);
            """
            values = (
                player_name,
                data.get("timestamp"),
                data.get("pos", {}).get("x"),
                data.get("pos", {}).get("y"),
                data.get("pos", {}).get("z"),
                data.get("health"),
                data.get("level"),
                data.get("experience")
            )
            cursor.execute(insert_query, values)
            self.conn.commit()
            # print(f"[DB] Wrote data for {player_name}")
        except psycopg2.Error as e:
            print(f"[DB ERROR] Could not write data: {e}")
        finally:
            # --- DEFENSIVE CHECK: Ensure cursor is closed ---
            if 'cursor' in locals() and cursor:
                cursor.close()

    def close(self):
        """Closes the database connection."""
        # --- DEFENSIVE CHECK: Ensure we don't try to close a non-existent connection ---
        if self.conn is not None:
            print("[*] Closing database connection.")
            if self.conn.closed == 0: # 0 means it's open
                self.conn.close()
            else:
                print("[*] Connection was already closed.")