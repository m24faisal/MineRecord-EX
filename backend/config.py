# backend/config.py
from typing import Dict, Any

# --- Database Configuration ---
DB_CONFIG: Dict[str, Any] = {
    "host": "localhost",
    "port": "5432",
    "database": "playerData",
    "user": "postgres",
    "password": "a"
}

# --- Data Collection Server Configuration ---
DATA_SERVER_CONFIG: Dict[str, Any] = {
    "host": "localhost",
    "port": 9999
}