from PySide6.QtWidgets import QApplication
from PySide6.QtUiTools import QUiLoader
from PySide6.QtCore import QFile

app = QApplication([])

loader = QUiLoader()

ui_file = QFile("mainwindow.ui")
ui_file.open(QFile.ReadOnly)

window = loader.load(ui_file)
ui_file.close()

window.show()

app.exec()