import traceback
import pika
import dataFormat as df
import json
import re
from datetime import datetime
import os
from dbManage import Database as db
import time

dataSnaps = []
db.create_database()
timeStamp = datetime.now().strftime("%m_%d_%Y_%H_%M_%S")
direct = "../saves/"
csvName = "playerData_" + timeStamp + ".csv"
fName = os.path.join(direct, csvName)
os.makedirs(direct, exist_ok=True)
tName = "playerData_" + timeStamp 
# Connect to RabbitMQ server
connection = pika.BlockingConnection(pika.ConnectionParameters('localhost'))
channel = connection.channel()

# Declare the same queue (in case it doesn’t exist)
channel.queue_declare(queue='data_gametracker')

# Callback function to handle received messages
def callback(ch, method, properties, body):
    # Read data from the received message queue
    p = re.compile('(?<!\\\\)\'')
    try:
        #print(f"Received data: {body}")
        raw_data = body.decode('ascii')
        raw_data = p.sub('\"', raw_data)
        data_dict = json.loads(raw_data)
        dataframe_instance = df.decrypt(data_dict)
        if dataframe_instance is None:
            print("Decryption failed")
            return
        #print("decrypt done")
        dataSnaps.append(dataframe_instance)
        df.save_to_csv(dataframe_instance, fName)
        db.save_ddataframe(dataframe_instance)
        
        #for data in dataSnaps:
            #df.save_to_csv(data, fName)
    except Exception as e:
        traceback.print_exc()
        print("Could not decipher properly")

print(' [*] Waiting for messages. Will shut down after 10 seconds of inactivity.')

try:
    # --- ROBUST TIMEOUT LOOP ---
    timeout_seconds = 10
    start_time = time.time()
    
    while True:
        # Check for a message. This is a non-blocking call.
        method_frame, header_frame, body = channel.basic_get(queue='data_gametracker', auto_ack=True)
        
        if method_frame:
            # If a message was received, process it and reset timer
            print(" [x] Received message")
            callback(channel, method_frame, header_frame, body)
            start_time = time.time() # Reset the timer
        else:
            # No message, check if timeout has been reached
            if time.time() - start_time > timeout_seconds:
                print(f" [.] No message received for {timeout_seconds} seconds. Shutting down.")
                break # Exit the loop
            else:
                # Wait a short period before polling again to avoid a tight loop
                time.sleep(1)

except KeyboardInterrupt:
    print("\n[!] Interrupted by user. Shutting down.")
except Exception as e:
    print(f"\n[!] An error occurred: {e}. Shutting down.")
finally:
    # This block ensures the connection is always closed properly
    print("Closing connection...")
    try:
        # Check if the connection object exists and is open before trying to close it
        if 'connection' in locals() and connection.is_open:
            connection.close()
    except Exception as e:
        print(f"Error while closing connection: {e}")
    print("Shutdown complete.")