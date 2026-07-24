import sqlite3

import paho.mqtt.client as mqtt
#called when the Pi connects to the broker

connection = sqlite3.connect('enclosure.db')

def on_connect(client, userdata, flags, reason_code, properties):
        print("Connected!")

        #sub to a topic
        client.subscribe("enclosure/temperature")
        client.subscribe("enclosure/humidity")

#called everytime a message arrives

#def on_message(client, userdata, msg):
#       print(f"Topic: {msg.topic}")
#       print(f"Message: {msg.payload.decode()}")
#       print("--------------")

def on_message(client, userdata, msg):
        if msg.topic == "enclosure/temperature":
                print(f"Temperature: {msg.payload.decode()}")
        #print(f"{msg.topic} - > {msg.payload.decode()}")

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)

client.on_connect = on_connect
client.on_message = on_message

client.connect("192.168.0.135", 1883)

print("starting loop....")
client.loop_forever()



