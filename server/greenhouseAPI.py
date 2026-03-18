from flask import Flask, request, jsonify, send_from_directory
import mysql.connector
import json
from datetime import datetime
import os
from flask_cors import CORS

app = Flask(__name__, static_folder='client/build', static_url_path='/')
CORS(app)

def get_db_connection():
    return mysql.connector.connect(
        host=os.getenv('DB_HOST', 'database'),
        user=os.getenv('GREEN_DB_USER'),
        password=os.getenv('GREEN_DB_PASS'),
        database=os.getenv('DB_NAME', 'green_db')
    )

def bootstrap():
    try:
        conn = get_db_connection()
        cursor = conn.cursor()
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS greenhouseData (
                id INT AUTO_INCREMENT PRIMARY KEY,
                timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                temperature DECIMAL(5,2),
                rssi INT
            )
        """)
        conn.commit()
        cursor.close()
        conn.close()
        print("Database bootstrapped successfully.")
    except Exception as e:
        print(f"Bootstrap failed: {e}")

@app.route('/api/greenhouseData')
def get_data():
    try:
        conn = get_db_connection()
        cursor = conn.cursor(dictionary=True)
        cursor.execute("SELECT timestamp, temperature, rssi FROM greenhouseData ORDER BY timestamp DESC LIMIT 100")
        rows = cursor.fetchall()
        cursor.close()
        conn.close()
        # Using jsonify is cleaner for Flask than json.dumps
        return jsonify(rows)
    except Exception as e:
        return jsonify({"error": str(e)}), 500

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
        if LOCATION == "home":
            conn = get_db_connection()
            cursor = conn.cursor(dictionary=True)

            # Simplified query to match your current schema
            query = """
                SELECT temperature, rssi, timestamp 
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
                conn = get_db_connection()
                cursor = conn.cursor()
                new_count = 0

                for item in pulled_data:
                    ts = item.get('timestamp')

                    # Check for duplicates based on timestamp
                    cursor.execute("SELECT COUNT(*) FROM greenhouseData WHERE timestamp = %s", (ts,))
                    if cursor.fetchone()[0] == 0:
                        # Updated INSERT to match your specific columns: temperature and rssi
                        query = """
                            INSERT INTO greenhouseData 
                            (temperature, rssi, timestamp) 
                            VALUES (%s, %s, %s)
                        """
                        cursor.execute(query, (
                            item.get('temperature'),
                            item.get('rssi'),
                            ts
                        ))
                        new_count += 1

                conn.commit()
                cursor.close()
                conn.close()
                return jsonify({"status": "imported from cl1p", "new_rows": new_count}), 200

            return jsonify({"error": "failed to fetch from cl1p"}), response.status_code

    except Exception as e:
        return jsonify({"error": str(e)}), 500

if __name__ == '__main__':
    bootstrap()
    # 0.0.0.0 is essential for Docker access
    app.run(host='0.0.0.0', port=5000)