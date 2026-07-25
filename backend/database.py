import sqlite3
import time

import my_globals

def save_temp_data():
    
    if my_globals.latest_temperature is not None:
        connection = sqlite3.connect("enclosure.db")
        connection.execute("""
        CREATE TABLE IF NOT EXISTS temperature_data (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            temperature REAL,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
        )
        """)
    connection.execute(
        "INSERT INTO temperature_data (temperature) VALUES (?)",
        (my_globals.latest_temperature,)
    )
    connection.commit()
    connection.close()

    print("Saved:", my_globals.latest_temperature)