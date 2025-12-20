# backend/config.py
from typing import Dict, Any

# --- Database Configuration ---
DB_CONFIG: Dict[str, Any] = {
    "host": "127.0.0.1",
    "port": "5432",
    "database": "playerdata",
    "user": "postgres",
    "password": "a"
}

# --- Data Collection Server Configuration ---
DATA_SERVER_CONFIG: Dict[str, Any] = {
    "host": "localhost",
    "port": 9999
}