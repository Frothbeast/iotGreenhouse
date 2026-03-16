USE iotData;

-- [Correction]: Using greenhouseData as the table name
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