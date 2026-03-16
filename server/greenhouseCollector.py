# greenhouseCollector.py
import os
import socket
import json
import binascii
import mysql.connector
from urllib.parse import parse_qs
from dotenv import load_dotenv
from datetime import datetime

load_dotenv(os.path.join(os.path.dirname(__file__), '..', '.env'))

BIND_HOST = os.getenv('BIND_HOST', '0.0.0.0')
PORT = 1884
DB_CONFIG = {
    'host': os.getenv('DB_HOST', 'shared_db_container'),
    'user': os.getenv('DB_USER'),
    'password': os.getenv('DB_PASS'),
    'database': 'iotData',
}

device_states = {}


def flush_device(device_id):
    global device_states
    state = device_states[device_id]

    if not state["temps"]:
        return

    try:
        count = len(state["temps"])
        temp_high = max(state["temps"])
        temp_low = min(state["temps"])
        temp_avg = round(sum(state["temps"]) / count, 2)
        rssi_high = max(state["rssis"])
        rssi_low = min(state["rssis"])
        rssi_current = state["rssis"][-1]

        conn_db = mysql.connector.connect(**DB_CONFIG)
        cursor = conn_db.cursor()

        query = """
            INSERT INTO greenhouseData 
            (temp_current, temp_high, temp_low, rssi_current, rssi_high, rssi_low, reading_count) 
            VALUES (%s, %s, %s, %s, %s, %s, %s)
        """
        cursor.execute(query, (temp_avg, temp_high, temp_low, rssi_current, rssi_high, rssi_low, count))

        conn_db.commit()
        cursor.close()
        conn_db.close()

        # Reset state
        device_states[device_id]["temps"] = []
        device_states[device_id]["rssis"] = []
    except Exception as e:
        print(f"Flush Error: {e}")