#!/bin/bash

mysql -u root -p"${MYSQL_ROOT_PASSWORD}" <<-EOSQL
    CREATE DATABASE IF NOT EXISTS \`${DB_NAME}\`;
    USE \`${DB_NAME}\`;

    CREATE USER IF NOT EXISTS '${DB_USER}'@'%' IDENTIFIED BY '${DB_PASS}';
    GRANT ALL PRIVILEGES ON \`${DB_NAME}\`.* TO '${DB_USER}'@'%';
    FLUSH PRIVILEGES;


    CREATE TABLE IF NOT EXISTS \`greenhouseData\` (
        \`id\` INT NOT NULL AUTO_INCREMENT,
        \`esp_ID\` VARCHAR(50) NOT NULL,
        \`datetime\` DATETIME NOT NULL,
        \`tempHigh\` DECIMAL(5, 2) DEFAULT NULL,
        \`tempLow\` DECIMAL(5, 2) DEFAULT NULL,
        \`rssiHigh\` INT DEFAULT NULL,
        \`rssiLow\` INT DEFAULT NULL,
        \`readingCount\` INT DEFAULT 0,
        \`notes\` TEXT,
        PRIMARY KEY (\`id\`),
        INDEX \`idx_esp_time\` (\`esp_ID\`, \`datetime\`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
EOSQL