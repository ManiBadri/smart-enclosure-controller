import sqlite3
import time
import paho.mqtt.client as mqtt
import threading

latest_temperature = None
#called when the Pi connects to the broker


def on_connect(client, userdata, flags, reason_code, properties):
        print("Connected!")

        #sub to a topic
        client.subscribe("enclosure/temperature")
        client.subscribe("enclosure/humidity")

#called everytime a message arrives

def on_message(client, userdata, msg):
        global latest_temperature
        if msg.topic == "enclosure/temperature":
                latest_temperature = float(msg.payload.decode())
        print("New temperature:", latest_temperature)
        #print(f"{msg.topic} - > {msg.payload.decode()}")

def database_logger():

    while True:
        time.sleep(60)
        if latest_temperature is not None:
            connection = sqlite3.connect("enclosure.db")
            connection.execute("""
            CREATE TABLE IF NOT EXISTS temperature_data (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                temperature REAL,
                timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
            )
            """)
            connection.execute(
                "INSERT INTO temperature_data (temperature) VALUES (?)",
                (latest_temperature,)
            )
            connection.commit()
            connection.close()

            print("Saved:", latest_temperature)
        
thread_temp = threading.Thread(target=database_logger)
thread_temp.daemon = True
thread_temp.start()

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)

client.on_connect = on_connect
client.on_message = on_message

client.connect("192.168.0.135", 1883)

print("starting loop....")



client.loop_forever()



