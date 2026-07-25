import threading
import mqtt_handler
import websocket_server
import myscheduler


#Start MQTT
mqtt_thread = threading.Thread(
    target=mqtt_handler.start_mqtt,
    daemon=True
)

mqtt_thread.start()


#Start WebSocket
websocket_thread = threading.Thread(
    target=websocket_server.websocket_thread,
    daemon=True
)

websocket_thread.start()


#Start scheduler
myscheduler.start_scheduler()