# greenhouseCollector.py
import os
import mysql.connector
from dotenv import load_dotenv

# Load the greenhouse-specific env
load_dotenv()

DB_CONFIG = {
    'host': os.getenv('DB_HOST', 'database'),
    'user': os.getenv('GREEN_DB_USER'), # Fixed from DB_USER
    'password': os.getenv('GREEN_DB_PASS'), # Fixed from DB_PASS
    'database': os.getenv('DB_NAME', 'green_db'), # Fixed from 'iotData'
}

device_states = {}

def flush_device(device_id):
    global device_states
    state = device_states[device_id]

    if not state["temps"]:
        return

    try:
        # Calculate the "Conservative" data
        temp_avg = round(sum(state["temps"]) / len(state["temps"]), 2)
        rssi_current = state["rssis"][-1]

        conn_db = mysql.connector.connect(**DB_CONFIG)
        cursor = conn_db.cursor()

        # Updated query to match your simple 2-column + timestamp schema
        query = """
            INSERT INTO greenhouseData 
            (temperature, rssi) 
            VALUES (%s, %s)
        """
        cursor.execute(query, (temp_avg, rssi_current))

        conn_db.commit()
        cursor.close()
        conn_db.close()

        # Reset state for next cycle
        device_states[device_id]["temps"] = []
        device_states[device_id]["rssis"] = []
    except Exception as e:
        print(f"Flush Error: {e}")