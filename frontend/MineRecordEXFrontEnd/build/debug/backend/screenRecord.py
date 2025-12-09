# backend/screenRecord.py
import subprocess
import sys

def start_ffmpeg_process(output_file, fps=30):
    """
    Starts an FFmpeg process for screen recording.
    The process must be controlled externally by sending 'q' to stdin.
    """
    # A robust FFmpeg command for gdigrab
    ffmpeg_cmd = [
        "ffmpeg",
        "-f", "gdigrab",
        "-framerate", str(fps),
        "-probesize", "10M",
        "-i", "desktop",
        "-c:v", "libx264",
        "-preset", "ultrafast",
        "-crf", "23",
        "-pix_fmt", "yuv420p",
        "-y",
        output_file
    ]
    
    print(f"[*] Starting FFmpeg with command: {' '.join(ffmpeg_cmd)}")
    
    try:
        # Start the process. We must open stdin so we can send it 'q' later.
        process = subprocess.Popen(
            ffmpeg_cmd,
            stdin=subprocess.PIPE,         # CRITICAL: Open stdin for writing
            stdout=subprocess.DEVNULL,     # Suppress FFmpeg's console output
            stderr=subprocess.DEVNULL,     # Suppress FFmpeg's error output
            creationflags=subprocess.CREATE_NO_WINDOW
        )
        return process
    except FileNotFoundError:
        print("\n!!! CRITICAL ERROR: 'ffmpeg' command not found. !!!")
        print("Please ensure FFmpeg is installed and in your system's PATH.")
        return None
    except Exception as e:
        print(f"\n!!! CRITICAL ERROR: Failed to launch FFmpeg: {e} !!!")
        return None

if __name__ == "__main__":
    # This block allows the script to be run standalone for easy testing.
    if len(sys.argv) < 2:
        print("Usage: python screenRecord.py <output_file.mp4>")
        sys.exit(1)
        
    output_file = sys.argv[1]
    process = start_ffmpeg_process(output_file)
    
    if process:
        print("Recording started. Press Ctrl+C to stop.")
        try:
            process.wait() # Wait for the process to be interrupted
        except KeyboardInterrupt:
            print("\nCtrl+C detected. Terminating FFmpeg process...")
            process.terminate()
            try:
                process.wait(timeout=5)
                print("FFmpeg terminated gracefully.")
            except subprocess.TimeoutExpired:
                print("FFmpeg did not terminate, killing it forcefully.")
                process.kill()
                print("FFmpeg killed.")