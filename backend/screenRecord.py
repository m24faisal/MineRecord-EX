import os
import time
import subprocess
import sys
import msvcrt  # Still the best way to detect a keypress on Windows

FINAL_OUTPUT = "../saves/" + f"output_{time.time()}.mp4"
FPS_TARGET = 60

def record_screen(output_file="output.mp4", fps=FPS_TARGET):
    """
    Records the screen by starting a process and terminating it on 'q'.
    This method is more robust than writing to stdin.
    """
    ffmpeg_cmd = [
        "ffmpeg",
        "-f", "gdigrab",
        "-framerate", str(fps),
        "-i", "desktop",
        "-vcodec", "libx264",
        "-preset", "fast",
        "-y",  # Overwrite output file if it exists
        output_file
    ]
    
    print("Starting FFmpeg process...")
    # --- KEY CHANGE: We do NOT ask for stdin, stdout, or stderr ---
    # This avoids the pipe creation errors entirely.
    try:
        process = subprocess.Popen(ffmpeg_cmd, creationflags=subprocess.CREATE_NO_WINDOW)
    except FileNotFoundError:
        print("\n!!! CRITICAL ERROR: 'ffmpeg' command not found. !!!")
        print("Please ensure FFmpeg is installed and in your system's PATH.")
        return
    except Exception as e:
        print(f"\n!!! CRITICAL ERROR: Failed to launch FFmpeg: {e} !!!")
        return

    # Give it a moment to start
    time.sleep(1.5)
    
    # Check if it's still running. If not, it failed.
    if process.poll() is not None:
        print(f"\nERROR: FFmpeg process terminated with exit code {process.poll()}.")
        print("It may have failed to start. Check your FFmpeg installation and 'gdigrab' support.")
        return

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
        # --- KEY CHANGE: Gracefully terminate the process ---
        process.terminate() # Sends a SIGTERM signal, asking it to quit
        try:
            # Wait up to 10 seconds for it to close
            process.wait(timeout=10)
            print("FFmpeg terminated gracefully. File saved.")
        except subprocess.TimeoutExpired:
            print("FFmpeg did not terminate in time. Forcing it to close.")
            process.kill() # Sends a SIGKILL signal, forcing it to quit
            print("FFmpeg killed. File may be corrupted, but most of it should be saved.")

if __name__ == "__main__":
    os.makedirs(os.path.dirname(FINAL_OUTPUT), exist_ok=True)
    record_screen(FINAL_OUTPUT, fps=FPS_TARGET)