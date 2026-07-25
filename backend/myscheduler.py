import time
import mydatabase


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
    timer_30 = IntervalTimer(30)
    timer_60 = IntervalTimer(60)

    while True:
        if timer_30.check():
            print("30 seconds passed!")

        if timer_60.check():
            mydatabase.save_temp_data()
            print("60 seconds passed! data saved")

        time.sleep(0.1)  #CPU usage
    