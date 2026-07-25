import asyncio
import websockets
import my_globals

#connected_clients = set()
async def websocket_handler(websocket):
      my_globals.connected_clients.add(websocket)
      print("desktop connected")

      try:
        async for messsage in websocket:
              print("Desktop:", messsage)
      finally:
            my_globals.connected_clients.remove(websocket)
            print("desktop disconnected")  

#This sends the newest temperature to every connected desktop
async def broadcast_temperature(temp):
      if my_globals.connected_clients:
        await asyncio.gather(
            *[client.send(str(temp)) for client in my_globals.connected_clients]
        )

async def websocket_server():
    async with websockets.serve(websocket_handler, "0.0.0.0", 8765):
        print("WebSocket server started")
        await asyncio.Future()

def websocket_thread():
    asyncio.run(websocket_server())
