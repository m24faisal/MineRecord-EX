# backend/receive.py
import traceback
import pika
import dataFormat as df
import json
import re
import time
from datetime import datetime
from dbManage import Database

def main():
    """Main function to listen for messages and save them to the database."""
    # Ensure the database and tables exist
    Database.create_database()

    # Connect to RabbitMQ server
    connection = pika.BlockingConnection(pika.ConnectionParameters('localhost'))
    channel = connection.channel()

    # Declare the same queue
    channel.queue_declare(queue='data_gametracker')

    # Callback function to handle received messages
    def callback(ch, method, properties, body):
        try:
            # Read data from the received message queue
            p = re.compile('(?<!\\\\)\'')
            raw_data = body.decode('ascii')
            raw_data = p.sub('\"', raw_data)
            data_dict = json.loads(raw_data)
            dataframe_instance = df.decrypt(data_dict)
            if dataframe_instance is None:
                print("Decryption failed")
                return
            
            print(f"Received data for player: {dataframe_instance.plyrName}")
            # Save the data to the database
            Database.save_ddataframe(dataframe_instance)
            
        except Exception as e:
            traceback.print_exc()
            print(f"Could not process message: {e}")

    print('[*] Waiting for messages. Will shut down after 10 seconds of inactivity.')

    try:
        # --- ROBUST TIMEOUT LOOP ---
        timeout_seconds = 10
        print(f'[*] Waiting for messages. Will shut down after {timeout_seconds} seconds of inactivity.')

        # Loop indefinitely until a timeout or interrupt
        while True:
            # Check for a message. This is a non-blocking call.
            method_frame, header_frame, body = channel.basic_get(queue='data_gametracker', auto_ack=True)
            
            if method_frame:
                # If a message was received, process it and continue the loop
                print(" [x] Received message")
                callback(channel, method_frame, header_frame, body)
                # The loop continues, resetting the inactivity timer implicitly
            else:
                # No message received in this poll, so check for timeout
                # This check is a bit redundant with the time.sleep but is explicit
                # A better way is to check the time since the last message
                # For simplicity, we'll just rely on the time.sleep to prevent a tight loop
                # and the user to break it with Ctrl+C
                pass
            
            # Wait a short period before polling again to avoid a tight loop
            # This prevents the script from using 100% CPU
            time.sleep(1)

    except KeyboardInterrupt:
        print("\n[!] Interrupted by user. Shutting down.")
    finally:
        # This block ensures the connection is always closed properly
        try:
            # Check if the connection object exists and is open
            if 'connection' in locals() and connection.is_open:
                connection.close()
                print("Connection to RabbitMQ closed.")
        except Exception as e:
            print(f"Error while closing connection: {e}")

if __name__ == "__main__":
    main()