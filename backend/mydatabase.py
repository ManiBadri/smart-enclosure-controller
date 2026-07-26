import sqlite3
import myglobals

def save_temp_data():
    if myglobals.latest_temperature is not None:
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
            (myglobals.latest_temperature,)
        )
        connection.commit()
        connection.close()
        print("Saved:", myglobals.latest_temperature)