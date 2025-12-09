# backend/receive.py
import traceback
import pika
import dataFormat as df
import json
import re
from datetime import datetime
from dbManage import Database

def listen_for_messages(stop_event):
    """
    Listens for RabbitMQ messages and saves them to the database.
    This function is designed to be run in a separate thread.
    The 'stop_event' is a threading.Event that can be used to signal the loop to exit.
    """
    print("[*] Starting data collection listener...")
    
    # Ensure the database and tables exist
    Database.create_database()

    # Connect to RabbitMQ server
    try:
        connection = pika.BlockingConnection(pika.ConnectionParameters('localhost'))
        channel = connection.channel()
    except Exception as e:
        print(f"[!] CRITICAL: Could not connect to RabbitMQ: {e}")
        return

    # Declare the same queue
    channel.queue_declare(queue='data_gametracker')

    # Callback function to handle received messages
    def callback(ch, method, properties, body):
        try:
            # Read data from the received message queue
            p = re.compile(r'(?<!\\)\'')
            raw_data = body.decode('ascii')
            raw_data = p.sub(r'\"', raw_data)
            data_dict = json.loads(raw_data)
            dataframe_instance = df.decrypt(data_dict)
            if dataframe_instance is None:
                print("[!] Decryption failed")
                return
            
            print(f"[*] Received data for player: {dataframe_instance.plyrName}")
            # Save the data to the database
            Database.save_ddataframe(dataframe_instance)
            
        except Exception as e:
            traceback.print_exc()
            print(f"[!] Could not process message: {e}")

    try:
        # Start consuming messages
        channel.basic_consume(queue='data_gametracker',
                                  auto_ack=True,
                                  on_message_callback=callback)
        
        # Loop to keep the main thread alive, but check the stop event
        while not stop_event.is_set():
            # process_data_events() is a non-blocking way to handle network events
            connection.process_data_events(time_limit=1)
    except KeyboardInterrupt:
        print("\n[!] Interrupted by user. Shutting down.")
    except Exception as e:
        print(f"\n[!] An unexpected error occurred: {e}")
    finally:
        # This block ensures the connection is always closed properly
        try:
            if 'connection' in locals() and connection.is_open:
                connection.close()
                print("[*] Connection to RabbitMQ closed.")
        except Exception as e:
            print(f"[!] Error while closing connection: {e}")