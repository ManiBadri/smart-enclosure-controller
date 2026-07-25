import time
import database


# Initialize the starting tracker
start_time = time.monotonic()

def every_30_second():
    global start_time
    current_time = time.monotonic()
    if current_time - start_time >= 30:
        start_time = current_time  # Reset the tracker
        return True
    return False


def every_60_second():
    global start_time
    current_time = time.monotonic()
    if current_time - start_time >= 60:
        start_time = current_time  # Reset the tracker
        return True
    return False

while True:
    if every_30_second():
        print("30 seconds passed!")
        
    if every_60_second():
        database.save_temp_data()
        print("60 seconds passed! data saved")
        
    time.sleep(0.1)  #CPU usage
    