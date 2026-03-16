from flask import Flask, request, jsonify, send_from_directory
import mysql.connector
import json
import os
import requests
import urllib3
from datetime import datetime
from dotenv import load_dotenv
from flask_cors import CORS

# Load environment variables
load_dotenv()

template_dir = '/app/client/build'

app = Flask(__name__,
            static_folder=template_dir,
            static_url_path='/')

CORS(app)

db_config = {
    'host': os.getenv('DB_HOST', 'shared_db_container'),
    'user': os.getenv('DB_USER'),
    'password': os.getenv('DB_PASS'),
    'database': 'iotData'
}

LOCATION = os.getenv('LOCATION')
CL1P_TOKEN = os.getenv('CL1P_TOKEN')
CL1P_URL = os.getenv('CL1P_URL')


def get_db():
    return mysql.connector.connect(**db_config)

# --- START OF ADDITION: Bootstrap Logic ---
def bootstrap_db():
    """Ensures the greenhouseData table exists in iotData database on startup."""
    try:
        conn = get_db()
        cursor = conn.cursor()
        # [Correction]: Standalone blueprint for greenhouseData
        create_table_query = """
        CREATE TABLE IF NOT EXISTS greenhouseData (
            id INT AUTO_INCREMENT PRIMARY KEY,
            timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            temp_current DECIMAL(5,2),
            temp_high DECIMAL(5,2),
            temp_low DECIMAL(5,2),
            rssi_current INT,
            rssi_high INT,
            rssi_low INT,
            reading_count INT
        );
        """
        cursor.execute(create_table_query)
        conn.commit()
        cursor.close()
        conn.close()
        print("Database bootstrap: greenhouseData table is ready.")
    except Exception as e:
        print(f"Database bootstrap failed: {e}")
# --- END OF ADDITION ---

@app.route('/api/cl1p', methods=['POST'])
def handle_cl1p_sync():
    urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)
    headers = {"Content-Type": "text/plain", "cl1papitoken": CL1P_TOKEN}

    try:
        if LOCATION == "home":
            conn = get_db()
            cursor = conn.cursor(dictionary=True)

            # [Correction]: Updated table name to greenhouseData
            query = """
                SELECT temp_current, temp_high, temp_low, 
                       rssi_current, rssi_high, rssi_low, 
                       reading_count, timestamp 
                FROM greenhouseData 
                WHERE timestamp >= NOW() - INTERVAL 7 DAY
            """
            cursor.execute(query)
            rows = cursor.fetchall()

            for row in rows:
                if isinstance(row['timestamp'], datetime):
                    row['timestamp'] = row['timestamp'].isoformat()

            payload = json.dumps(rows)
            requests.post(CL1P_URL, data=payload, headers=headers, verify=False)

            cursor.close()
            conn.close()
            return jsonify({"status": "pushed to cl1p", "count": len(rows)}), 200

        elif LOCATION == "work":
            response = requests.get(CL1P_URL, headers={"cl1papitoken": CL1P_TOKEN}, verify=False)

            if response.status_code == 200:
                pulled_data = json.loads(response.text)
                conn = get_db()
                cursor = conn.cursor()
                new_count = 0

                for item in pulled_data:
                    ts = item.get('timestamp')
                    # [Correction]: Updated table name to greenhouseData
                    cursor.execute("SELECT COUNT(*) FROM greenhouseData WHERE timestamp = %s", (ts,))
                    if cursor.fetchone()[0] == 0:
                        query = """
                            INSERT INTO greenhouseData 
                            (temp_current, temp_high, temp_low, rssi_current, rssi_high, rssi_low, reading_count, timestamp) 
                            VALUES (%s, %s, %s, %s, %s, %s, %s, %s)
                        """
                        cursor.execute(query, (
                            item.get('temp_current'), item.get('temp_high'), item.get('temp_low'),
                            item.get('rssi_current'), item.get('rssi_high'), item.get('rssi_low'),
                            item.get('reading_count'), ts
                        ))
                        new_count += 1

                conn.commit()
                cursor.close()
                conn.close()
                return jsonify({"status": "imported from cl1p", "new_rows": new_count}), 200

            return jsonify({"error": "failed to fetch from cl1p"}), response.status_code

    except Exception as e:
        return jsonify({"error": str(e)}), 500


# [Correction]: Updated endpoint name to greenhouseData
@app.route('/api/greenhouseData', methods=['GET'])
def get_greenhouse_data():
    try:
        hours = request.args.get('hours', default=24, type=int)
        conn = get_db()
        cursor = conn.cursor(dictionary=True)
        # [Correction]: Updated table name to greenhouseData
        query = """
            SELECT timestamp, temp_current, temp_high, temp_low,
                   rssi_current, rssi_high, rssi_low, reading_count 
            FROM greenhouseData 
            WHERE timestamp >= NOW() - INTERVAL %s HOUR
            ORDER BY timestamp ASC
        """
        cursor.execute(query, (hours,))
        rows = cursor.fetchall()
        cursor.close()
        conn.close()
        return jsonify(rows)
    except Exception as e:
        return jsonify({"error": str(e)}), 500


@app.route('/api/data', methods=['POST'])
def post_data():
    try:
        data = request.get_json()
        conn = get_db()
        cursor = conn.cursor()
        # [Correction]: Updated table name to greenhouseData
        query = """
            INSERT INTO greenhouseData 
            (temp_current, temp_high, temp_low, rssi_current, rssi_high, rssi_low, reading_count) 
            VALUES (%s, %s, %s, %s, %s, %s, %s)
        """
        cursor.execute(query, (
            data.get('temp_current'), data.get('temp_high'), data.get('temp_low'),
            data.get('rssi_current'), data.get('rssi_high'), data.get('rssi_low'),
            data.get('reading_count')
        ))
        conn.commit()
        cursor.close()
        conn.close()
        return jsonify({"status": "success"}), 201
    except Exception as e:
        return jsonify({"status": "error", "message": str(e)}), 500


@app.route('/', defaults={'path': ''})
@app.route('/<path:path>')
def serve(path):
    if path != "" and os.path.exists(os.path.join(app.static_folder, path)):
        return send_from_directory(app.static_folder, path)
    return send_from_directory(app.static_folder, 'index.html')


if __name__ == '__main__':
    # [Correction]: Call bootstrap to ensure table exists before serving requests
    bootstrap_db()
    port = int(os.getenv('API_PORT', 5000))
    app.run(host='0.0.0.0', port=port)