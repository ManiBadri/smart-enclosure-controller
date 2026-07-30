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

def update_temperature(message):
    window.temperatureLabel.setText(message)

ws.message_received.connect(update_temperature) ##when message is received the update



ws.start()

window.show()

app.exec()