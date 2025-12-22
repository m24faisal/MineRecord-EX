# backend/config.py
import os
import json
from typing import Dict, Any

def load_db_config() -> Dict[str, Any]:
    """Load database config from db_config.json. Fall back to defaults if missing."""
    config_path = os.path.join(os.path.dirname(__file__), 'db_config.json')
    default_config = {
        "host": "127.0.0.1",
        "port": "5432",
        "database": "playerdata",
        "user": "postgres",
        "password": "a"
    }
    if os.path.exists(config_path):
        try:
            with open(config_path, 'r', encoding='utf-8') as f:
                user_config = json.load(f)
            # Validate required keys
            for key in default_config.keys():
                if key not in user_config:
                    print(f"[CONFIG] Warning: '{key}' missing in db_config.json. Using default.")
                    user_config[key] = default_config[key]
            return user_config
        except Exception as e:
            print(f"[CONFIG] Error reading db_config.json: {e}. Using defaults.")
    else:
        print("[CONFIG] db_config.json not found. Using default DB credentials.")
    return default_config

# Global config used by database.py
DB_CONFIG: Dict[str, Any] = load_db_config()

# --- Data Collection Server Configuration (unchanged) ---
DATA_SERVER_CONFIG: Dict[str, Any] = {
    "host": "localhost",
    "port": 9999
}