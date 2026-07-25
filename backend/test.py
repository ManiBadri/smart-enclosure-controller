import asyncio
import websockets


async def main():

    uri = "ws://localhost:8765"

    async with websockets.connect(uri) as websocket:

        print("Connected to server!")

        while True:
            message = await websocket.recv()
            print("Received:", message)


asyncio.run(main())