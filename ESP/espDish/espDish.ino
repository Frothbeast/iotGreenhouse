#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "config.h"

// --- Configuration ---
const char* DEVICE_ID = "DISH_UNIT"; 
#define ONE_WIRE_BUS 2 
// Use the port defined in your .env (GREENHOUSE_PORT)
const int SERVER_PORT = 1884; 

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Fixed-size buffers to avoid dynamic RAM allocation (Strings)
char rawBuffer[128];
char hexBuffer[256];

// --- Optimized Hex Converter (No Strings) ---
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
    sensors.begin();
    
    // Set non-blocking mode for DS18B20 to avoid long waits
    sensors.setWaitForConversion(false); 
    
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500); 
        Serial.print(".");
    }
    Serial.println("\nConnected.");
}

void loop() {
    // Feed the watchdog
    ESP.wdtFeed();

    if (WiFi.status() == WL_CONNECTED) {
        WiFiClient client;

        // Use the IP address of your Python server (defined in config.h or manually)
        // Extracting server IP/Host from SERVER_URL if needed, 
        // but for sockets, we just need the raw Hostname/IP.
        if (client.connect("192.168.1.XX", SERVER_PORT)) { 
            
            // 1. Get Temperature
            sensors.requestTemperatures(); 
            unsigned long startWait = millis();
            while (millis() - startWait < 750) {
                yield(); // Feed WiFi stack during sensor conversion
            }
            float tempC = sensors.getTempCByIndex(0);
            
            // 2. Get RSSI
            int rssi = WiFi.RSSI();

            // 3. Format and Encode
            snprintf(rawBuffer, sizeof(rawBuffer), "id=%s&temp=%.2f&rssi=%d", DEVICE_ID, tempC, rssi);
            toHex(rawBuffer, hexBuffer);

            // 4. Send over raw TCP socket
            client.print("payload=");
            client.print(hexBuffer);
            
            // 5. Wait for ACK from Python
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
            Serial.println("Connection to Collector failed.");
        }
    }
    
    delay(10000); 
}