#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "config.h"

// --- Configuration --- 
#define ONE_WIRE_BUS 2 
 

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

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
    sensors.begin();
    
    // Standard blocking mode is safer for timing-sensitive reads
    sensors.setWaitForConversion(true); 
    
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
        // 1. Request Temperature (750ms internal delay for 12-bit)
        sensors.requestTemperatures(); 
        
        // 2. Read with Interrupt Protection and Retry Logic
        float tempC = -127.00;
        for (int i = 0; i < 3; i++) {
            // Disable interrupts to prevent WiFi stack from stealing CPU cycles during bit-banging
            noInterrupts(); 
            tempC = sensors.getTempCByIndex(0);
            interrupts(); 
            
            // If reading is valid, break out of retry loop
            if (tempC != -127.00 && tempC != 85.00) {
                break;
            }
            delay(100); // Short breather before next attempt
        }

        // 3. Only transmit if we have a valid reading
        if (tempC != -127.00 && tempC != 85.00) {
            WiFiClient client;

            if (client.connect(SERVER_IP, SERVER_PORT)) { 
                int rssi = WiFi.RSSI();

                // 4. Format and Encode
                snprintf(rawBuffer, sizeof(rawBuffer), "id=%s&temp=%.2f&rssi=%d", DEVICE_ID, tempC, rssi);
                toHex(rawBuffer, hexBuffer);

                // 5. Send Only Hex ASCII
                client.print(hexBuffer); 
                
                // 6. Wait for ACK from Python
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
        } else {
            Serial.println("Sensor Read Error: Skipping this cycle.");
        }
    }
    
    // 10-second polling interval
    delay(10000); 
}