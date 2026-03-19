# greenhouseCollector.py
import os
import socket
import mysql.connector
import time
from datetime import datetime
from dotenv import load_dotenv

load_dotenv()

DB_CONFIG = {
    'host': os.getenv('DB_HOST', 'database'),
    'user': os.getenv('GREEN_DB_USER'),
    'password': os.getenv('GREEN_DB_PASS'),
    'database': os.getenv('DB_NAME', 'green_db'),
}

device_states = {}


def flush_device(device_id):
    global device_states
    state = device_states.get(device_id)
    if not state or not state["temps"]:
        return

    try:
        temp_avg = round(sum(state["temps"]) / len(state["temps"]), 2)
        rssi_current = state["rssis"][-1]

        conn_db = mysql.connector.connect(**DB_CONFIG)
        cursor = conn_db.cursor()
        query = "INSERT INTO greenhouseData (temperature, rssi) VALUES (%s, %s)"
        cursor.execute(query, (temp_avg, rssi_current))
        conn_db.commit()
        cursor.close()
        conn_db.close()

        device_states[device_id]["temps"] = []
        device_states[device_id]["rssis"] = []
    except Exception as e:
        print(f"Flush Error: {e}")


def start_collector():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    bind_host = os.getenv('BIND_HOST', '0.0.0.0')
    port = int(os.getenv('COLLECTOR_PORT', 1884))

    server_socket.bind((bind_host, port))
    server_socket.listen(5)

    while True:
        try:
            conn, addr = server_socket.accept()
            with conn:
                data = conn.recv(1024)
                if data:
                    # 1. Decode Hex to ASCII string
                    # Example: "69643d474831..." -> "id=GH1&temp=22.50&rssi=-65"
                    hex_data = data.decode('ascii').strip()
                    decoded_str = bytes.fromhex(hex_data).decode('ascii')

                    # 2. Parse the key-value pairs
                    params = dict(item.split("=") for item in decoded_str.split("&"))

                    dev_id = params.get("id", "default")
                    temp = float(params.get("temp", 0))
                    rssi = int(params.get("rssi", 0))

                    if dev_id not in device_states:
                        device_states[dev_id] = {"temps": [], "rssis": []}

                    device_states[dev_id]["temps"].append(temp)
                    device_states[dev_id]["rssis"].append(rssi)

                    if len(device_states[dev_id]["temps"]) >= 5:
                        flush_device(dev_id)

                    conn.sendall(b"ACK")
        except Exception as e:
            print(f"Collector Error: {e}")
            time.sleep(2)


if __name__ == "__main__":
    start_collector()