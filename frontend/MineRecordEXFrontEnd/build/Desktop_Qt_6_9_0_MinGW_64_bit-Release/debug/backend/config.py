# backend/config.py
import os
import json
from typing import Dict, Any

def load_db_config() -> Dict[str, Any]:
    """
    Loads database configuration from 'db_config.json' in the same directory.
    Falls back to hardcoded defaults if the file is missing or invalid.
    """
    config_path = os.path.join(os.path.dirname(__file__), 'db_config.json')
    default_config = {
        "host": "127.0.0.1",
        "port": "5432",
        "database": "playerdata",
        "user": "postgres",
        "password": "postgres"
    }

    if os.path.exists(config_path):
        try:
            with open(config_path, 'r', encoding='utf-8') as f:
                user_config = json.load(f)
            # Ensure all required keys are present
            for key in default_config:
                if key not in user_config:
                    print(f"[CONFIG] Warning: '{key}' missing in db_config.json. Using default.")
                    user_config[key] = default_config[key]
            return user_config
        except Exception as e:
            print(f"[CONFIG] Error reading db_config.json: {e}. Using defaults.")
    else:
        print("[CONFIG] db_config.json not found. Using default DB credentials.")

    return default_config

# Global DB configuration used by database.py
DB_CONFIG: Dict[str, Any] = load_db_config()

# --- Data Collection Server Configuration (unchanged) ---
DATA_SERVER_CONFIG: Dict[str, Any] = {
    "host": "localhost",
    "port": 9999
}