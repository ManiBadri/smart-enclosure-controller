from PySide6.QtWidgets import QApplication
from PySide6.QtUiTools import QUiLoader
from PySide6.QtCore import QFile
from scripts.websocket_client import WebSocketClient

app = QApplication([])

loader = QUiLoader()

ui_file = QFile("ui/mainwindow.ui")
ui_file.open(QFile.ReadOnly)

window = loader.load(ui_file)
ui_file.close()


ws = WebSocketClient()

ws.message_received.connect(
    lambda message: window.temperatureLabel.setText(message)
)

ws.start()


window.show()

app.exec()