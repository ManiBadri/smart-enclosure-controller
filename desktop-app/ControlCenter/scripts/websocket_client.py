from PySide6.QtCore import QObject, Signal
import asyncio
import websockets
import threading
import json


class WebSocketClient(QObject):
    message_received = Signal(dict)

    def start(self):
        threading.Thread(target=self._run, daemon=True).start()

    def _run(self):
        asyncio.run(self._listen())

    async def _listen(self):
        uri = "ws://localhost:8765"

        async with websockets.connect(uri) as websocket:
            print("Connected!")

            while True:
                message = await websocket.recv()

                packet = json.loads(message)

                self.message_received.emit(packet)