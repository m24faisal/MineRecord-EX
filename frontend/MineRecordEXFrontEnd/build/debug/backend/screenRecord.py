# backend/screenRecord.py
import os
import time
import subprocess
import sys
import msvcrt  # For detecting a keypress on Windows

def record_screen(output_file="output.mp4", fps=60):
    """
    Records the screen by starting a process and terminating it on 'q'.
    This function is now designed to be called from another script.
    """
    ffmpeg_cmd = [
        "ffmpeg",
        "-f", "gdigrab",  # Use gdigrab for Windows
        "-framerate", str(fps),
        "-i", "desktop",
        "-vcodec", "libx264",
        "-preset", "fast",
        "-y",  # Overwrite output file if it exists
        output_file
    ]
    
    print(f"Starting FFmpeg process. Output will be saved to: {output_file}")
    try:
        # Use CREATE_NO_WINDOW to hide the console window on Windows
        process = subprocess.Popen(ffmpeg_cmd, creationflags=subprocess.CREATE_NO_WINDOW)
        return process # Return the process object so it can be managed
    except FileNotFoundError:
        print("\n!!! CRITICAL ERROR: 'ffmpeg' command not found. !!!")
        print("Please ensure FFmpeg is installed and in your system's PATH.")
        return None
    except Exception as e:
        print(f"\n!!! CRITICAL ERROR: Failed to launch FFmpeg: {e} !!!")
        return None

if __name__ == "__main__":
    # This block allows the script to be run standalone for testing
    if len(sys.argv) < 2:
        print("Usage: python screenRecord.py <output_file.mp4>")
        sys.exit(1)
        
    output_file = sys.argv[1]
    process = record_screen(output_file)
    
    if process:
        print("Recording started. Press 'q' to stop and save the recording.")
        try:
            while True:
                if msvcrt.kbhit():
                    if msvcrt.getwch().lower() == 'q':
                        print("\n'q' detected. Terminating FFmpeg process...")
                        break
                time.sleep(0.1)
        except KeyboardInterrupt:
            print("\nCtrl+C detected. Terminating FFmpeg process...")
        finally:
            process.terminate()
            try:
                process.wait(timeout=10)
                print("FFmpeg terminated gracefully. File saved.")
            except subprocess.TimeoutExpired:
                print("FFmpeg did not terminate in time. Forcing it to close.")
                process.kill()
                print("FFmpeg killed. File may be corrupted, but most of it should be saved.")