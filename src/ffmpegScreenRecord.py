import os
import time
import subprocess
import signal
import sys

FINAL_OUTPUT = "../saves/" + f"output_{time.time()}.mp4"
FPS_TARGET = 60  # Frames per second

def record_screen(output_file="output.mp4", fps=FPS_TARGET):
    ffmpeg_cmd = [
        "ffmpeg",
        "-f", "gdigrab",  # Windows capture method
        "-framerate", str(fps),  # Capture at desired FPS
        "-i", "desktop",  # Capture the entire desktop
        "-vcodec", "libx264",  # Use the x264 codec for compression
        "-preset", "fast",  # Encoding speed (trade-off for size and quality)
        output_file
    ]
    
    # Start the process
    process = subprocess.Popen(ffmpeg_cmd)
    
    print("Recording started. Press 'q' and Enter to stop recording.")
    
    # Wait for user input to stop recording
    while True:
        if input().lower() == 'q':
            print("Stopping recording...")
            process.terminate()  # Send SIGTERM signal
            process.wait()  # Wait for the process to finish
            break

# Example usage
record_screen(FINAL_OUTPUT, fps=FPS_TARGET)