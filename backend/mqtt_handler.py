import paho.mqtt.client as mqtt
import threading
import my_globals

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
                my_globals.latest_temperature = float(msg.payload.decode())
        print("New temperature:", my_globals.latest_temperature)
        if msg.topic == "enclosure/humidity":
                my_globals.latest_humidity = float(msg.payload.decode())
        print("New humidity:", my_globals.latest_humidity)
        #print(f"{msg.topic} - > {msg.payload.decode()}")


client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)

client.on_connect = on_connect
client.on_message = on_message

client.connect("192.168.0.135", 1883)

print("starting loop....")

client.loop_forever()



