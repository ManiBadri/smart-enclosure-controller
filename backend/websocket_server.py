import asyncio
import json
import websockets
import myglobals
import myconfig

async def websocket_handler(websocket):
    myglobals.connected_clients.add(websocket)

    try:
        async for message in websocket:
            print(message)

    finally:
        myglobals.connected_clients.remove(websocket)

async def broadcast_sensor_data():

    packet = {
    "type": "sensor_update",
    "data": myglobals.sensor_data
    }

    message = json.dumps(packet)

    if myglobals.connected_clients:
        await asyncio.gather(
            *(client.send(message) for client in myglobals.connected_clients)
        )
        print("*********broadcasted sensor data**********")

async def websocket_server():
    async with websockets.serve(websocket_handler, "0.0.0.0", myconfig.WEBSOCKET_PORT):
        print("WebSocket server started")
        await asyncio.Future()

def websocket_thread():
    myglobals.websocket_loop = asyncio.new_event_loop()
    asyncio.set_event_loop(myglobals.websocket_loop)

    myglobals.websocket_loop.run_until_complete(websocket_server())