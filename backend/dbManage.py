# PostgreSQL Database Integration with Python 3
from dataclasses import asdict, fields
import psycopg2
from psycopg2 import sql
import json

from dataFormat import Effect, Item, DEFAULT_DATA_SNAP
from typing import TypedDict

class DBConfig(TypedDict):
    host: str
    database: str
    user: str
    password: str
    port: str

class Database:

    DB_NAME = "playerData"
    #PosrgeSQL Server Connection Settings
    APP_DB_CONFIG: DBConfig = {
        "host": "localhost",
        "database":  DB_NAME,
        "user": "postgres",
        "password": "Faiz256",
        "port": "5432"
    }

    MASTER_DB_CONFIG: DBConfig = {
        "host": "localhost",
        "database":  "postgres",
        "user": "postgres",
        "password": "Faiz256",
        "port": "5432"
    }

    DEFAULT_DDATAFRAME = None # convert_dataframe_to_ddataframe(DEFAULT_DATA_SNAP)
    
    # Create Database

    @classmethod
    def create_database(cls):
        connection = None
        try:
            # Connect to the PostgreSQL server (not a specific database yet)
            connection = psycopg2.connect(**cls.MASTER_DB_CONFIG)
            connection.autocommit = True  # Enable autocommit to execute CREATE DATABASE

            # Create a cursor
            cursor = connection.cursor()
            # Check if the database already exists
            cursor.execute(
                "SELECT 1 FROM pg_catalog.pg_database WHERE datname = %s;", (cls.DB_NAME,)
            )
            exists = cursor.fetchone()

            
            if exists:
                print(f"Database '{cls.DB_NAME}' already exists.")
            else:
                # Create the new database
                cursor.execute(sql.SQL("CREATE DATABASE {};").format(sql.Identifier(cls.DB_NAME)))
                print(f"Database '{cls.DB_NAME}' created successfully.")

            cursor.execute(
                sql.SQL("GRANT ALL PRIVILEGES ON DATABASE {} TO {};").format(
                    sql.Identifier(cls.DB_NAME),
                    sql.Identifier(cls.APP_DB_CONFIG.get('user'))
            )
            )

        except Exception as e:
            print("Error while creating the database:", e)
        if connection:
            cursor.close()
            connection.close()
            print("Server connection closed.")
            

    @staticmethod
    def get_postgresql_type(value):
        if isinstance(value, str):
            return "VARCHAR(255)"
        elif isinstance(value, bool):
            return "BOOLEAN"
        elif isinstance(value, int):
            return "INTEGER"
        elif isinstance(value, float):
            return "REAL"
        elif isinstance(value, list):
            return "JSONB"  # Use JSONB for storing lists
        elif isinstance(value, dict):
            return "JSONB"  # Use JSONB for nested dictionaries
        elif hasattr(value, "__dict__"):
            return "JSONB"  # Serialize Python objects as JSONB
        else:
            return "TEXT"
    @classmethod
    def create_table(cls, table_name, data):
        """
        Ensures the table exists and has all required columns.
        This version is fully isolated to prevent transaction errors.
        """
        # --- THE FIX: Get a fresh connection for this specific operation ---
        conn = None
        try:
            conn = psycopg2.connect(**cls.APP_DB_CONFIG)
            # Set autocommit to True to make this check a self-contained transaction.
            # This prevents it from being affected by any previous failed operations.
            conn.autocommit = True
            
            with conn.cursor() as cursor:
                # --- The Critical Check ---
                # This query is now guaranteed to run in a clean context.
                print(f"DIAGNOSTIC: Checking if table '{table_name}' exists...")
                cursor.execute(
                    "SELECT COUNT(*) FROM information_schema.tables WHERE table_name = %s;",
                    (table_name,)
                )
                
                # --- Defensive Programming ---
                result = cursor.fetchone()
                if result is None:
                    # This should be impossible with COUNT(*), but we check to be 100% safe.
                    raise Exception(f"CRITICAL: Database check for table '{table_name}' returned an unexpected result.")
                
                table_exists = result[0] > 0
                print(f"DIAGNOSTIC: Table '{table_name}' exists? {table_exists}")

                # 2. If table does not exist, create it
                if not table_exists:
                    print(f"Table '{table_name}' does not exist. Creating it...")
                    column_definitions: list[sql.Composable] = [sql.SQL("id SERIAL PRIMARY KEY")]
                    for key, value in data.items():
                        column_definitions.append(
                            sql.SQL("{} {}").format(sql.Identifier(key), sql.SQL(cls.get_postgresql_type(value)))
                        )
                    if table_name in ("ITEMS", "EFFECTS"):
                        column_definitions.append(sql.SQL("data_id INTEGER NOT NULL"))

                    columns_sql = sql.SQL(", ").join(column_definitions)
                    create_table_query = sql.Composed([
                        sql.SQL("CREATE TABLE "),
                        sql.Identifier(table_name),
                        sql.SQL(" ("),
                        columns_sql,
                        sql.SQL(");")
                    ])
                    cursor.execute(create_table_query)
                    print(f"Table '{table_name}' created successfully.")
                
                # 3. If table exists, check for and add missing columns
                else:
                    print(f"Table '{table_name}' exists. Checking for missing columns...")
                    cursor.execute("""
                        SELECT column_name FROM information_schema.columns 
                        WHERE table_name = %s;
                    """, (table_name,))
                    existing_columns = {row[0] for row in cursor.fetchall()}

                    for key, value in data.items():
                        if key not in existing_columns:
                            print(f"Column '{key}' not found in table '{table_name}'. Adding it...")
                            alter_query = sql.SQL("ALTER TABLE {} ADD COLUMN {} {};").format(
                                sql.Identifier(table_name),
                                sql.Identifier(key),
                                sql.SQL(cls.get_postgresql_type(value))
                            )
                            cursor.execute(alter_query)
                            print(f"Column '{key}' added successfully.")
                    
                    if table_name in ("ITEMS", "EFFECTS") and "data_id" not in existing_columns:
                        print(f"Column 'data_id' not found in table '{table_name}'. Adding it...")
                        alter_query = sql.SQL("ALTER TABLE {} ADD COLUMN data_id INTEGER NOT NULL;").format(
                            sql.Identifier(table_name)
                        )
                        cursor.execute(alter_query + sql.SQL(" DEFAULT 0;"))
                        print(f"Column 'data_id' added successfully.")

        except Exception as e:
            print(f"Error in create_table for '{table_name}': {e}")
            # No rollback needed because we are using autocommit
        finally:
            if conn:
                conn.close()
        

    @staticmethod
    def serialize_value(value):
        """
        Serialize Python objects, lists, and dictionaries for insertion into PostgreSQL.
        """
        if isinstance(value, (list, dict, tuple)) or hasattr(value, "__dict__"):
            return json.dumps(value, default=lambda o: o.__dict__) # Convert to JSON
        return value # Return to original value for simple data types
    
    @classmethod
    def serialize_multiple_values(cls, values):
        return [cls.serialize_value(val) for val in values]
    
    # creates  ddataframe (database dataframe) => tuple(DATA, [EFFECT], [ITEM]) from a Dataframe Object
    @classmethod
    def convert_dataframe_to_ddataframe(cls, dataframe):
        """Converts a complex dataframe object into a tuple of dicts for DB insertion."""
        datadict = asdict(dataframe)
        itemList = []
        effectList = []

        for invItem in dataframe.plyrInventory:
            itemList.append(asdict(invItem))
        datadict.pop("plyrInventory", None)
        datadict.pop("plyrArmor", None) # Discard for now
        datadict.pop("plyrOffhand", None)
        
        for effect in dataframe.plyrStatus:
            effectList.append(asdict(effect))
        datadict.pop("plyrStatus", None)
        
        return (datadict, itemList, effectList)
    

    #assumes dataframe
    # separate out schema creation
    # accepts ddataframe (database dataframe) => tuple(DATA, [EFFECT], [ITEM])
    @classmethod
    def save_ddataframe(cls, dataframe):
        """
        Saves a dataframe object to the database.
        """
        ddataframe = cls.convert_dataframe_to_ddataframe(dataframe)
        
        if cls.DEFAULT_DDATAFRAME is None:
            cls.DEFAULT_DDATAFRAME = cls.convert_dataframe_to_ddataframe(DEFAULT_DATA_SNAP)
            cls.create_table("DATA", cls.DEFAULT_DDATAFRAME[0])
            sample_item = cls.DEFAULT_DDATAFRAME[1][0].copy()
            sample_item['data_id'] = 0
            sample_effect = cls.DEFAULT_DDATAFRAME[2][0].copy()
            sample_effect['data_id'] = 0
            cls.create_table("ITEMS", sample_item)
            cls.create_table("EFFECTS", sample_effect)

        connection = None
        try:
            connection = psycopg2.connect(**cls.APP_DB_CONFIG)
            with connection.cursor() as cursor:
                # --- Insert into DATA table ---
                serialized_data = {key: cls.serialize_value(value) for key, value in ddataframe[0].items()}
                columns = list(serialized_data.keys())
                values = list(serialized_data.values())
                
                query = sql.Composed([
                    sql.SQL("INSERT INTO "),
                    sql.Identifier("DATA"),
                    sql.SQL(" ("),
                    sql.SQL(", ").join(map(sql.Identifier, columns)),
                    sql.SQL(") VALUES ("),
                    sql.SQL(", ").join(sql.Placeholder() * len(values)),
                    sql.SQL(") RETURNING id;")
                ])
                
                # --- ENHANCED ERROR HANDLING ---
                try:
                    print(f"Executing Query: {cursor.mogrify(query, values).decode('utf-8')}")
                    cursor.execute(query, values)
                    result = cursor.fetchone()
                    if result is None:
                        # This should not happen if execute succeeds, but we check anyway.
                        raise Exception("INSERT succeeded but did not return an ID.")
                    inserted_data_id = result[0]
                    print(f"Successfully inserted into DATA table with new ID: {inserted_data_id}")
                except psycopg2.Error as db_error:
                    # Catch the specific database error and print it.
                    print(f"DATABASE ERROR during DATA insert: {db_error}")
                    print("This is likely a schema mismatch. Check the printed query against your table structure.")
                    raise # Re-raise the exception to stop execution

                # ... (rest of the method for ITEMS and EFFECTS is the same) ...
                # --- Insert into ITEMS table ---
                item_columns = [field.name for field in fields(Item)] + ["data_id"]
                query_items = sql.Composed([
                    sql.SQL("INSERT INTO "),
                    sql.Identifier("ITEMS"),
                    sql.SQL(" ("),
                    sql.SQL(", ").join(map(sql.Identifier, item_columns)),
                    sql.SQL(") VALUES ("),
                    sql.SQL(", ").join(sql.Placeholder() * len(item_columns)),
                    sql.SQL(");")
                ])

                for item_dict in ddataframe[1]:
                    item_values = cls.serialize_multiple_values(item_dict.values())
                    item_values.append(inserted_data_id)
                    cursor.execute(query_items, item_values)

                # --- Insert into EFFECTS table ---
                effect_columns = [field.name for field in fields(Effect)] + ["data_id"]
                query_effects = sql.Composed([
                    sql.SQL("INSERT INTO "),
                    sql.Identifier("EFFECTS"),
                    sql.SQL(" ("),
                    sql.SQL(", ").join(map(sql.Identifier, effect_columns)),
                    sql.SQL(") VALUES ("),
                    sql.SQL(", ").join(sql.Placeholder() * len(effect_columns)),
                    sql.SQL(");")
                ])

                for effect_dict in ddataframe[2]:
                    effect_values = cls.serialize_multiple_values(effect_dict.values())
                    effect_values.append(inserted_data_id)
                    cursor.execute(query_effects, effect_values)

            connection.commit()
            print("All data inserted successfully!")

        except Exception as e:
            print(f"An error occurred during DDATAFRAME SAVE: {e}")
            if connection:
                connection.rollback()
            raise
        finally:
            if connection:
                connection.close()
    @classmethod
    def custom_command(cls, query):
        """Executes a raw SQL command. Use with caution."""
        connection = None
        try:
            connection = psycopg2.connect(**cls.APP_DB_CONFIG)
            with connection.cursor() as cursor:
                cursor.execute(query)
                # If it's a SELECT query, fetch and return results
                if cursor.description:
                    return cursor.fetchall()
                # Otherwise, commit the command (e.g., for UPDATE, DELETE)
                else:
                    connection.commit()
                    return cursor.rowcount
        except Exception as e:
            print(f"Error at Custom Command: {e}")
            if connection:
                connection.rollback()
            return None
        finally:
            if connection:
                connection.close()
                print("Database connection closed.")