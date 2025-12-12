# backend/run_server_standalone.py
import sys
import os

# Add the backend directory to the path
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

try:
    # Import and call the function directly
    from backend_controller import start_data_collection_service
    print("[*] Standalone server script started. Starting data collection service.")
    # Start the service. This will block until the process is killed.
    start_data_collection_service(True) 
except Exception as e:
    print(f"[!!!] Standalone server script failed: {e}")
    import traceback
    traceback.print_exc()
    # Exit with a non-zero code to indicate failure
    sys.exit(1)