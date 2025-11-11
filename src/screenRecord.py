import os
import time
import subprocess
import signal
import sys

FINAL_OUTPUT = "../saves/" + f"output_{time.time()}.mp4"
FPS_TARGET = 60  # Frames per second

# We'll store the process in a global variable so the signal handler can access it
ffmpeg_process = None

def signal_handler(sig, frame):
    """
    This function will be called when the user presses Ctrl+C
    """
    global ffmpeg_process
    print("\nStopping recording... Please wait, this may take a moment.")
    if ffmpeg_process:
        # Send the 'q' command to ffmpeg's stdin, which tells it to quit gracefully
        ffmpeg_process.communicate(input=b'q')
    print("Recording stopped and file saved.")
    sys.exit(0)

def record_screen(output_file="output.mp4", fps=FPS_TARGET):
    """
    Records the screen indefinitely until the user presses Ctrl+C.
    """
    global ffmpeg_process

    ffmpeg_cmd = [
        "ffmpeg",
        "-f", "gdigrab",  # Windows capture method
        "-framerate", str(fps),  # Capture at desired FPS
        "-i", "desktop",  # Capture the entire desktop
        "-vcodec", "libx264",  # Use the x264 codec for compression
        "-preset", "fast",  # Encoding speed (trade-off for size and quality)
        output_file
    ]
    
    # Register our signal handler for Ctrl+C
    signal.signal(signal.SIGINT, signal_handler)

    print("Recording started. Press Ctrl+C to stop and save the recording.")
    
    # Start the process
    # We need to open stdin so we can send the 'q' command later
    ffmpeg_process = subprocess.Popen(ffmpeg_cmd, stdin=subprocess.PIPE)
    
    # Wait for the process to complete. It will only complete after our
    # signal handler sends the 'q' command.
    ffmpeg_process.wait()

if __name__ == "__main__":
    # Make sure the saves directory exists
    os.makedirs(os.path.dirname(FINAL_OUTPUT), exist_ok=True)
    record_screen(FINAL_OUTPUT, fps=FPS_TARGET)