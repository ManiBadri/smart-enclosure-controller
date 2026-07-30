latest_temperature = None
latest_humidity = None

connected_clients = set()

sensor_data = {
    "temperature": None,
    "humidity": None,
    "pressure": None,
    "light": None,
    "fan_speed": None
}

connected_clients = set()
websocket_loop = None
