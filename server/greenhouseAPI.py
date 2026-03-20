from flask import Flask, request, jsonify, send_from_directory
import mysql.connector
import json
from datetime import datetime
import time
import os
import requests  # Added missing import
import urllib3   # Added missing import
from flask_cors import CORS
from dotenv import load_dotenv
import sys

static_dir = os.environ.get('STATIC_FOLDER', '/app/client/build')

app = Flask(__name__, static_folder=static_dir, static_url_path='/')

print(f"DEBUG: Static folder is set to: {app.static_folder}")
print(f"DEBUG: Does path exist? {os.path.exists(app.static_folder)}")

CORS(app)
load_dotenv()
CL1P_TOKEN = os.getenv('CL1P_TOKEN')
CL1P_URL = os.getenv('CL1P_URL')
LOCATION = os.getenv('LOCATION')

GREEN_USER = os.getenv('GREEN_DB_USER')
GREEN_PASS = os.getenv('GREEN_DB_PASS')
DB_HOST = os.getenv('DB_HOST')
DB_NAME = os.getenv('DB_NAME')

if not GREEN_USER or not GREEN_PASS:
    sys.stderr.write(f"ERROR: Environment Variables Missing! User: {GREEN_USER}, Pass: {'SET' if GREEN_PASS else 'MISSING'}\n")

db_config = {
    'host': DB_HOST,
    'user': GREEN_USER,
    'password': GREEN_PASS,
    'database': DB_NAME
}

def datetime_handler(x):
    if isinstance(x, datetime):
        return x.isoformat()
    raise TypeError("Unknown type")

def get_db_connection():
    retries = 5
    while retries > 0:
        try:
            conn = mysql.connector.connect(**db_config)
            return conn
        except mysql.connector.Error as err:
            sys.stderr.write(f"Connection failed, retrying in 5s... {err}\n")
            retries -= 1
            time.sleep(5)
    return None

def bootstrap_db():
    retries = 5
    while retries > 0:
        try:
            conn = get_db_connection()
            cursor = conn.cursor()
            cursor.execute("""
                CREATE TABLE IF NOT EXISTS greenhouseData (
                    id INT AUTO_INCREMENT PRIMARY KEY,
                    esp_ID VARCHAR(50),
                    datetime TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                    tempHigh DECIMAL(5,2),
                    tempLow DECIMAL(5,2),
                    rssiHigh INT,
                    rssiLow INT,
                    readingCount INT,
                    notes VARCHAR(50)
                )
            """)
            conn.commit()
            cursor.close()
            conn.close()
            print("Database bootstrapped successfully.")
            return
        except Exception as e:
            print(f"Database not ready, retrying... ({retries} left)")
            retries -= 1
            time.sleep(5)

@app.route('/api/greenhouseData')
def get_data():
    try:
        hours = request.args.get('hours', default=24, type=int)
        print(f"DEBUG: Fetching data for last {hours} hours", file=sys.stderr, flush=True)
        conn = get_db_connection()
        cursor = conn.cursor(dictionary=True)
        cursor.execute("SELECT datetime, tempHigh, tempLow, rssiHigh, rssiLow, readingCount, notes FROM greenhouseData WHERE datetime > NOW() - INTERVAL %s HOUR")
        rows = cursor.fetchall()
        cursor.close()
        conn.close()

        return app.response_class(
            response=json.dumps(rows, default=datetime_handler),
            status=200,
            mimetype='application/json'
        )


    except Exception as e:
        print(f"ERROR: {str(e)}", file=sys.stderr, flush=True)
        return jsonify([]), 200

@app.route('/api/time', methods=['GET'])
def get_time():
    return jsonify({"time": datetime.now().strftime("%I:%M %p")})

# Serve React App
@app.route('/', defaults={'path': ''})
@app.route('/<path:path>')
def serve(path):
    if path != "" and os.path.exists(os.path.join(app.static_folder, path)):
        return send_from_directory(app.static_folder, path)
    else:
        return send_from_directory(app.static_folder, 'index.html')


@app.route('/api/cl1p', methods=['POST'])
def handle_cl1p_sync():
    urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)
    headers = {"Content-Type": "text/plain", "cl1papitoken": CL1P_TOKEN}

    try:
        if location == "home":
            conn = get_db_connection()
            cursor = conn.cursor(dictionary=True)

            query = """
                SELECT datetime,
                    esp_ID,
                    tempHigh,
                    tempLow,
                    rssiHigh,
                    rssiLow,
                    readingCount,
                    notes
                FROM greenhouseData
                WHERE datetime >= NOW() - INTERVAL 7 DAY
            """
            cursor.execute(query)
            rows = cursor.fetchall()

            for row in rows:
                if isinstance(row['datetime'], datetime):
                    row['datetime'] = str(row['datetime'])
                    row['tempHigh'] = str(row['tempHigh'])
                    row['tempLow'] = str(row['tempLow'])
                    row['rssiHigh'] = str(row['rssiHigh'])
                    row['rssiLow'] = str(row['rssiLow'])
                    row['readingCount'] = str(row['readingCount'])
                    row['notes'] = str(row['notes'])
                    row['esp_ID'] = str(row['esp_ID'])


            long_string_payload = json.dumps(rows)
            response = requests.post(cl1pURL, data=long_string_payload, headers=headers, verify=False)

            cursor.close()
            conn.close()
            return jsonify({"status": "pushed to cl1p", "count": len(rows)}), 200

        elif LOCATION == "work":
            response = requests.get(cl1pURL, headers=headers, verify=False)

            if response.status_code == 200:
                cl1p_payloads = json.loads(response.text)
                if isinstance(cl1p_payloads, list):
                    conn = get_db_connection()
                    if not conn: return jsonify({"error": "DB Connection Timeout"}), 500
                    cursor = conn.cursor()

                    for item in cl1p_payloads:
                        ts = item.get('datetime')
                        cursor.execute("SELECT COUNT(*) FROM greenhouseData WHERE datetime = %s", (ts,))
                        if cursor.fetchone()[0] == 0:
                            query = """
                                    INSERT INTO greenhouseData (datetime,
                                        esp_ID,
                                        tempHigh,
                                        tempLow,
                                        rssiHigh,
                                        rssiLow,
                                        readingCounts,
                                        notes
                                    )
                                    VALUES (%s, %s, %s, %s, %s, %s, %s, %s)
                                """
                            cursor.execute(query, (ts, item.get('esp_ID'), item.get('tempHigh'),
                                                   item.get('tempLow'), item.get('rssiHigh'),
                                                   item.get('rssiLow'), item.get('readingCounts'),
                                                   item.get('notes')))
                    conn.commit()
                    cursor.close()
                    conn.close()
            return '', 204
    except Exception as e:
        return jsonify({"error": str(e)}), 500

if __name__ == '__main__':
    bootstrap_db()
    port_env = int(os.getenv('API_PORT'))
    app.run(host='0.0.0.0', port=port_env)