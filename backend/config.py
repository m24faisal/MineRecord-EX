# backend/config.py
import os
import sys
import json
from typing import Dict, Any

def load_db_config() -> Dict[str, Any]:
    """
    Loads database configuration from 'db_config.json'.
    
    Search order:
    1. Same directory as this config.py file
    2. Parent directory (if launched from C++ wrapper in root)
    3. Falls back to hardcoded defaults if not found
    """
    # Get directory where config.py is located
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    # Candidate paths (in order of preference)
    candidate_paths = [
        os.path.join(script_dir, 'db_config.json'),           # backend/db_config.json
        os.path.join(script_dir, '..', 'db_config.json'),     # ../db_config.json (if backend/ is subfolder)
        os.path.join(os.getcwd(), 'backend', 'db_config.json'), # current_working_dir/backend/
        os.path.join(os.getcwd(), 'db_config.json')           # current_working_dir/
    ]
    
    default_config = {
        "host": "127.0.0.1",
        "port": "5432",
        "database": "playerdata",
        "user": "postgres",
        "password": "postgres"
    }

    # Try each candidate path
    for config_path in candidate_paths:
        config_path = os.path.abspath(config_path)
        if os.path.exists(config_path):
            try:
                with open(config_path, 'r', encoding='utf-8') as f:
                    user_config = json.load(f)
                
                # Validate required keys
                for key in default_config:
                    if key not in user_config:
                        print(f"[CONFIG] Warning: '{key}' missing in {config_path}. Using default.")
                        user_config[key] = default_config[key]
                
                print(f"[CONFIG] Successfully loaded config from: {config_path}")
                return user_config
                
            except Exception as e:
                print(f"[CONFIG] Error reading {config_path}: {e}. Trying next location...")

    # If none found, use defaults
    print("[CONFIG] db_config.json not found in any expected location. Using default DB credentials.")
    print(f"[CONFIG] Searched paths: {candidate_paths}")
    return default_config

# Global DB configuration used by database.py
DB_CONFIG: Dict[str, Any] = load_db_config()

# --- Data Collection Server Configuration ---
DATA_SERVER_CONFIG: Dict[str, Any] = {
    "host": "localhost",
    "port": 9999
}