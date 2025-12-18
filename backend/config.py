# backend/config.py
from typing import Dict, Any

# --- Database Configuration ---
DB_CONFIG: Dict[str, Any] = {
    "host": "localhost",
    "port": "5433",
    "database": "playerData",
    "user": "postgres",
    "password": "Faiz256!"
}

# --- Data Collection Server Configuration ---
DATA_SERVER_CONFIG: Dict[str, Any] = {
    "host": "127.0.0.1",
    "port": "9999"
}