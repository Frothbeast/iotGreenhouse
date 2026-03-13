#include <ESP8266WiFi.h>
#include "config.h"

// --- Configuration ---
const char* DEVICE_ID = "RSSI_MONITOR_01"; // Unique ID for this unit
const int SERVER_PORT = 1884; 

// Fixed-size buffers
char rawBuffer[128];
char hexBuffer[256];

// --- Optimized Hex Converter ---
void toHex(const char* input, char* output) {
    const char* hexChars = "0123456789abcdef";
    while (*input) {
        *output++ = hexChars[(*input >> 4) & 0x0F];
        *output++ = hexChars[*input & 0x0F];
        input++;
    }
    *output = '\0';
}

void setup() {
    Serial.begin(115200);
    
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500); 
        Serial.print(".");
    }
    Serial.println("\nConnected.");
}

void loop() {
    ESP.wdtFeed();

    if (WiFi.status() == WL_CONNECTED) {
        WiFiClient client;

        if (client.connect(SERVER_IP, SERVER_PORT)) { 
            int rssi = WiFi.RSSI();

            // 1. Format String (Notice: temp=0.00 or omitted)
            // We keep the temp key so the Python aggregator doesn't crash 
            // when looking for the 'temp' value.
            snprintf(rawBuffer, sizeof(rawBuffer), "id=%s&temp=0.00&rssi=%d", DEVICE_ID, rssi);
            
            // 2. Encode to Hex
            toHex(rawBuffer, hexBuffer);

            // 3. Send Only Hex
            client.print(hexBuffer); 
            
            // 4. Wait for ACK
            unsigned long timeout = millis();
            while (client.available() == 0 && millis() - timeout < 2000) {
                yield();
            }
            
            if (client.available()) {
                String response = client.readStringUntil('\n');
                Serial.println("Server ACK: " + response);
            }

            client.stop();
        } else {
            Serial.println("Connection failed.");
        }
    }
    
    // Poll every 10 seconds to match the Greenhouse Collector's aggregation window
    delay(10000); 
}