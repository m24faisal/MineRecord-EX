# backend/config.py
from typing import Dict, Any

# --- Database Configuration ---
DB_CONFIG: Dict[str, Any] = {
    "host": "127.0.0.1",
    "port": "5432",  # Keep as string for psycopg2
    "database": "playerdata",
    "user": "postgres",
    "password": "a"
}

# --- Data Collection Server Configuration ---
DATA_SERVER_CONFIG: Dict[str, Any] = {
    "host": "localhost",
    "port": 9999  # FIX: Change from "9999" (string) to 9999 (integer)
}