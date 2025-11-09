import traceback
import pika
import dataFormat as df
import json
import re
from datetime import datetime
import os
from dbManage import Database as db

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
        db.save_ddataframe(db.convert_dataframe_to_ddataframe(dataframe_instance))
        
        #for data in dataSnaps:
            #df.save_to_csv(data, fName)
    except Exception as e:
        traceback.print_exc()
        print("Could not decipher properly")

# Set up the consumer to listen to the queue
channel.basic_consume(queue='data_gametracker',
                      on_message_callback=callback,
                      auto_ack=True)

print(' [*] Waiting for messages. To exit press CTRL+C')
channel.start_consuming()
