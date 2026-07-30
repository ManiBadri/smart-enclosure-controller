from PySide6.QtWidgets import QApplication
from PySide6.QtUiTools import QUiLoader
from PySide6.QtCore import QFile
from scripts.websocket_client import WebSocketClient
from scripts.stylesheet import apply_theme

app = QApplication([])

apply_theme(app)

loader = QUiLoader()

ui_file = QFile("ui/mainwindow.ui")
ui_file.open(QFile.ReadOnly)

window = loader.load(ui_file)
ui_file.close()

ws = WebSocketClient()

def update_dashboard(packet):

    if packet["type"] == "sensor_update":

        data = packet["data"]

        window.temperatureLabel.setText(
            f'{data["temperature"]:.1f} °C'
        )

ws.message_received.connect(update_dashboard) ##when message is received the update

ws.start()

window.show()

app.exec()