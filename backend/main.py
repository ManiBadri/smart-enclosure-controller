import threading
import mqtt_handler
import myscheduler

mqtt_thread = threading.Thread(
    target=mqtt_handler.start_mqtt,
    daemon=True
)
mqtt_thread.start()

myscheduler.start_scheduler()