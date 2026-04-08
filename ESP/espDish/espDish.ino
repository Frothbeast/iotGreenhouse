#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "config.h"

#define ONE_WIRE_BUS 2 

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

char rawBuffer[128];
char hexBuffer[256];

unsigned long lastMeasureTime = 0;
const unsigned long measureInterval = 10000;

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
    sensors.setResolution(9);
    sensors.setWaitForConversion(true);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
}

void loop() {
    yield();

    if (millis() - lastMeasureTime >= measureInterval) {
        lastMeasureTime = millis();

        sensors.requestTemperatures();

        float tempC = sensors.getTempCByIndex(0);

        if (tempC == -127.00 || tempC == 85.00) {
            tempC = -128.00;
            Serial.println("Local Sensor Error: -128");
        } else {
            Serial.print("Local Temp Read: "); 
            Serial.println(tempC);
        }

        if (WiFi.status() == WL_CONNECTED) {
            WiFiClient client;
            client.setTimeout(2000);

            if (client.connect(SERVER_IP, SERVER_PORT)) {
                int rssi = WiFi.RSSI();
                snprintf(rawBuffer, sizeof(rawBuffer), "id=%s&temp=%.2f&rssi=%d", DEVICE_ID, tempC, rssi);
                toHex(rawBuffer, hexBuffer);

                client.println(hexBuffer);
                
                unsigned long startWait = millis();
                while (client.connected() && !client.available() && (millis() - startWait < 1500)) {
                    yield();
                }

                String response = "";
                unsigned long startRead = millis();
                while (client.connected() && (millis() - startRead < 1000)) {
                    while (client.available()) {
                        char c = client.read();
                        if (c == '\n') break; 
                        response += c;
                    }
                }
                
                if (response.length() > 0) {
                    Serial.println("ACK: " + response);
                }
                client.stop();
            } else {
                Serial.println("Server connection failed.");
            }
        } else {
            Serial.println("WiFi down: Skipping transmission.");
        }
    }
}