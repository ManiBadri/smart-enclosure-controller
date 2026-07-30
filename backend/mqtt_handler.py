import paho.mqtt.client as mqtt
import myglobals
import myconfig


#called when the Pi connects to the broker 
def on_connect(client, userdata, flags, reason_code, properties):
        print("Connected!")
        #sub to a topic
        client.subscribe("enclosure/temperature")
        client.subscribe("enclosure/humidity")
#called everytime a message arrives

def on_message(client, userdata, msg):
        if msg.topic == "enclosure/temperature":
            myglobals.sensor_data["temperature"] = float(msg.payload.decode())
            print("New temperature:", myglobals.sensor_data["temperature"])
        
        elif msg.topic == "enclosure/humidity":
            myglobals.sensor_data["humidity"] = float(msg.payload.decode())
            print("New humidity:", myglobals.sensor_data["humidity"])


def start_mqtt():
        client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)

        client.on_connect = on_connect
        client.on_message = on_message

        client.connect("192.168.0.135", myconfig.MQTT_PORT)

        print("starting loop....")

        client.loop_forever()



