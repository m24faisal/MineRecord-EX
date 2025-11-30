# backend/screenRecord.py
import subprocess
import sys

def start_ffmpeg_process(output_file, fps=60):
    """
    Starts the FFmpeg process and returns the process object.
    The parent script is responsible for stopping it.
    """
    # A more robust FFmpeg command for gdigrab on Windows
    ffmpeg_cmd = [
        "ffmpeg",
        "-f", "gdigrab",
        "-framerate", str(fps),
        "-thread_queue_size", "1024", # Handle buffering
        "-probesize", "10M",       # Probe for larger streams
        "-i", "desktop",
        "-vcodec", "libx264",
        "-preset", "fast",
        "-crf", "23",             # Constant Rate Factor (quality)
        "-pix_fmt", "yuv420p",      # Ensure compatibility
        "-y", # Overwrite output file if it exists
        output_file
    ]
    
    print(f"Starting FFmpeg with command: {' '.join(ffmpeg_cmd)}")
    
    try:
        # Start the process. We don't need stdin anymore.
        process = subprocess.Popen(ffmpeg_cmd, creationflags=subprocess.CREATE_NO_WINDOW)
        return process
    except FileNotFoundError:
        print("\n!!! CRITICAL ERROR: 'ffmpeg' command not found. !!!")
        print("Please ensure FFmpeg is installed and in your system's PATH.")
        return None
    except Exception as e:
        print(f"\n!!! CRITICAL ERROR: Failed to launch FFmpeg: {e} !!!")
        return None

if __name__ == "__main__":
    # This allows the script to be run standalone for testing
    if len(sys.argv) < 2:
        print("Usage: python screenRecord.py <output_file.mp4>")
        sys.exit(1)
        
    output_file = sys.argv[1]
    process = start_ffmpeg_process(output_file)
    
    if process:
        print("Recording started. Press Ctrl+C to stop.")
        try:
            process.wait() # Wait for the process to finish
        except KeyboardInterrupt:
            print("\nCtrl+C detected. Terminating FFmpeg process...")
            process.terminate()
            process.wait()