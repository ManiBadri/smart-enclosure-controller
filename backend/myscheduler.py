import time
import mydatabase
import asyncio
import myglobals
import websocket_server

# Initialize the starting tracker
start_time = time.monotonic()

class IntervalTimer:
    def __init__(self, interval_seconds):
        self.interval = interval_seconds
        self.start_time = time.monotonic()

    def check(self):
        current_time = time.monotonic()
        if current_time - self.start_time >= self.interval:
            self.start_time = current_time
            return True
        return False

def start_scheduler():
    print("*********************starting schedule*********************")
    timer_5 = IntervalTimer(5)
    timer_60 = IntervalTimer(60)

    while True:
        if timer_5.check():
            asyncio.run_coroutine_threadsafe(
                websocket_server.broadcast_sensor_data(),
                myglobals.websocket_loop
            )

        if timer_60.check():
            mydatabase.save_temp_data() 

        time.sleep(0.1)  #CPU usage
    