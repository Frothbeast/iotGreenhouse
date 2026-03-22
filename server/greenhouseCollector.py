# greenhouseCollector.py
import os
import socket
import mysql.connector
import time
from datetime import datetime
from dotenv import load_dotenv

load_dotenv()

DB_CONFIG = {
    'host': os.getenv('DB_HOST'),
    'user': os.getenv('GREEN_DB_USER'),
    'password': os.getenv('GREEN_DB_PASS'),
    'database': os.getenv('DB_NAME'),
}

# Tracking state for multiple ESP devices
device_states = {}


def flush_device(device_id):
    global device_states
    state = device_states.get(device_id)
    if not state or not state["temps"]:
        return

    try:
        # Calculate stats matching your schema
        t_high = max(state["temps"])
        t_low = min(state["temps"])
        r_high = max(state["rssis"])
        r_low = min(state["rssis"])
        count = len(state["temps"])
        now = datetime.now()

        conn_db = mysql.connector.connect(**DB_CONFIG)
        cursor = conn_db.cursor()

        query = """
            INSERT INTO greenhouseData 
            (esp_ID, datetime, tempHigh, tempLow, rssiHigh, rssiLow, readingCount) 
            VALUES (%s, %s, %s, %s, %s, %s, %s)
        """
        cursor.execute(query, (device_id, now, t_high, t_low, r_high, r_low, count))

        conn_db.commit()
        cursor.close()
        conn_db.close()

        # Reset buffer for this device
        device_states[device_id]["temps"] = []
        device_states[device_id]["rssis"] = []

    except Exception as e:
        print(f"Flush Error: {e}", flush=True)


def start_collector():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    bind_host = os.getenv('BIND_HOST')
    port = int(os.getenv('COLLECTOR_PORT'))  # Matches Greenhouse .env

    server_socket.bind((bind_host, port))
    server_socket.listen(5)
    print(f"Greenhouse Collector listening on {port}...", flush=True)

    while True:
        try:
            conn, addr = server_socket.accept()
            with conn:
                data = conn.recv(1024)
                if data:
                    # Expecting hex-encoded string: "id=GH1&temp=22.50&rssi=-65"
                    hex_data = data.decode('ascii').strip()
                    decoded_str = bytes.fromhex(hex_data).decode('ascii')

                    params = dict(item.split("=") for item in decoded_str.split("&"))

                    dev_id = params.get("id", "default")
                    temp = float(params.get("temp", 0))
                    rssi = int(params.get("rssi", 0))

                    if dev_id not in device_states:
                        device_states[dev_id] = {"temps": [], "rssis": []}

                    device_states[dev_id]["temps"].append(temp)
                    device_states[dev_id]["rssis"].append(rssi)

                    # Flush to DB every 5 readings
                    if len(device_states[dev_id]["temps"]) >= 5:
                        flush_device(dev_id)

                    conn.sendall(b"ACK")
        except Exception as e:
            print(f"Collector Error: {e}", flush=True)
            time.sleep(2)


if __name__ == "__main__":
    start_collector()