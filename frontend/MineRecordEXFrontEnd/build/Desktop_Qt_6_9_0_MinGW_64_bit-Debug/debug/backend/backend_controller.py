# backend_controller.py (Temporary Test Version)

print("SUCCESS: backend_controller.py was loaded by Python!")

def start_recording(game_name, game_path, recording_path, enable_data_collection=False):
    """A simple test function."""
    print("C++ successfully called start_recording!")
    return "test_recording_id_123"

def stop_recording(recording_id):
    """A simple test function."""
    print("C++ successfully called stop_recording!")
    return "Recording stopped for test ID."