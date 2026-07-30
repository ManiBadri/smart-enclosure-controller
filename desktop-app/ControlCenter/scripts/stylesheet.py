def apply_theme(app):
    app.setStyleSheet("""
    QWidget {
        background-color: #2b2b2b;
        color: white;
    }

    QPushButton {
        background-color: #3c3f41;
        border: 1px solid #555;
        border-radius: 5px;
        padding: 5px;
    }

    QPushButton:hover {
        background-color: #4a4d50;
    }

    QLineEdit {
        background-color: #3c3f41;
        color: white;
        border: 1px solid #555;
    }

    QLabel {
        color: white;
    }
    """)