# backend/screenRecord.py
import subprocess
import sys
import os

def start_ffmpeg_process(output_file, fps=30):
    """
    Starts an FFmpeg process to a temporary file.
    Returns a tuple of (process_object, temp_file_path).
    """
    # Create a temporary file name in the same directory as the final output
    dir_name = os.path.dirname(output_file)
    base_name = os.path.splitext(os.path.basename(output_file))[0]
    temp_file = os.path.join(dir_name, f"{base_name}_temp.mp4")

    # A robust FFmpeg command for gdigrab
    ffmpeg_cmd = [
        "ffmpeg",
        "-f", "gdigrab",
        "-framerate", str(fps),
        "-probesize", "10M",
        "-i", "desktop",
        "-c:v", "libx264",
        "-preset", "ultrafast",
        "-y", # Overwrite output file if it exists
        temp_file
    ]
    
    print(f"Starting FFmpeg capture to temporary file: {temp_file}")
    
    try:
        # Start the process without a console window.
        process = subprocess.Popen(ffmpeg_cmd, creationflags=subprocess.CREATE_NO_WINDOW)
        # Return both the process and the path to the temp file
        return process, temp_file
    except FileNotFoundError:
        print("\n!!! CRITICAL ERROR: 'ffmpeg' command not found. !!!")
        print("Please ensure FFmpeg is installed and in your system's PATH.")
        # Return None for both to indicate failure
        return None, None
    except Exception as e:
        print(f"\n!!! CRITICAL ERROR: Failed to launch FFmpeg: {e} !!!")
        # Return None for both to indicate failure
        return None, None

def finalize_video(temp_file, final_file):
    """
    Uses a separate FFmpeg command to safely convert the temp file to the final file.
    This is much more reliable and produces a clean, playable video.
    """
    print(f"Finalizing video from {temp_file} to {final_file}...")
    
    # A robust command for finalizing the video
    finalize_cmd = [
        "ffmpeg",
        "-i", temp_file,      # Input is the temporary file
        "-c", "copy",      # Stream copy, no re-encoding, very fast
        "-y",              # Overwrite final file if it exists
        final_file
    ]
    
    try:
        # Run this command to completion. It's fast and won't hang.
        subprocess.run(finalize_cmd, check=True, creationflags=subprocess.CREATE_NO_WINDOW)
        print("Video finalized successfully.")
        
        # Clean up the temporary file
        os.remove(temp_file)
        print("Temporary file removed.")
        return True
    except FileNotFoundError:
        print("\n!!! CRITICAL ERROR: 'ffmpeg' command not found during finalization. !!!")
        return False
    except subprocess.CalledProcessError as e:
        print(f"\n!!! ERROR during video finalization: {e} !!!")
        return False
    except Exception as e:
        print(f"\n!!! UNEXPECTED ERROR during video finalization: {e} !!!")
        return False

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python screenRecord.py <output_file.mp4>")
        sys.exit(1)
        
    output_file = sys.argv[1]
    process, temp_file = start_ffmpeg_process(output_file)
    
    # Check if the process started successfully
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
        
        # After the process is one way or another stopped, finalize the video
        if temp_file and os.path.exists(temp_file):
            finalize_video(temp_file, output_file)
        else:
            print("Temporary file not found, cannot finalize video.")