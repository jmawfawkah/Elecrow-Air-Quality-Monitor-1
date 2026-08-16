#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <ArduinoJson.h>
#include "comm/comm_protocol.h"

// Forward declarations
void fetchWeather();

// WiFi credentials (will be set by Elecrow via UART)
String wifi_ssid = "";
String wifi_password = "";

// NTP settings
int timezone_offset = 0;  // hours
String lat = "0";
String lon = "0";

// Weather API settings
const char* weather_host = "api.open-meteo.com";

// Timing
unsigned long lastWeatherUpdate = 0;
const unsigned long WEATHER_INTERVAL = 600000;  // 10 minutes
unsigned long lastTimeSync = 0;
const unsigned long TIME_SYNC_INTERVAL = 3600000;  // 1 hour

// UART for communication with Elecrow
#define ELE_UART Serial1

void setup() {
    Serial.begin(115200);
    ELE_UART.begin(115200, SERIAL_8N1, ELECROW_UART_RX, ELECROW_UART_TX);
    
    comm.begin(ELE_UART);
    
    // Register callbacks
    comm.onWiFiConnect = [](const String &ssid, const String &password) {
        wifi_ssid = ssid;
        wifi_password = password;
        WiFi.disconnect();
        WiFi.begin(ssid.c_str(), password.c_str());
        Serial.printf("[Stamp] Connecting to WiFi: %s\n", ssid.c_str());
    };
    
    comm.onWiFiDisconnect = []() {
        WiFi.disconnect();
        comm.sendWiFiStatusDisconnected();
        Serial.println("[Stamp] WiFi disconnected");
    };
    
    comm.onWiFiScan = []() {
        Serial.println("[Stamp] Starting WiFi scan");
        int n = WiFi.scanNetworks();
        for (int i = 0; i < n; i++) {
            comm.sendScanResult(WiFi.SSID(i), WiFi.RSSI(i), 
                               WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
            delay(10);
        }
        comm.sendScanDone(n);
        WiFi.scanDelete();
    };
    
    comm.onTimezone = [](int offset) {
        timezone_offset = offset;
        Serial.printf("[Stamp] Timezone set to %d\n", offset);
    };
    
    comm.onLocation = [](const String &la, const String &lo) {
        lat = la;
        lon = lo;
        Serial.printf("[Stamp] Location set: %s, %s\n", lat.c_str(), lon.c_str());
    };
    
    Serial.println("[Stamp] XIAO ESP32-S3 Network MCU started");
}

void loop() {
    comm.update();
    
    // Check WiFi status
    static wl_status_t lastStatus = WL_IDLE_STATUS;
    wl_status_t currentStatus = WiFi.status();
    if (currentStatus != lastStatus) {
        lastStatus = currentStatus;
        if (currentStatus == WL_CONNECTED) {
            comm.sendWiFiStatusConnected(WiFi.SSID(), WiFi.localIP().toString());
            Serial.printf("[Stamp] WiFi connected: %s, IP: %s\n", 
                         WiFi.SSID().c_str(), 
                         WiFi.localIP().toString().c_str());
        } else if (currentStatus == WL_CONNECT_FAILED || currentStatus == WL_NO_SSID_AVAIL) {
            comm.sendWiFiStatusFailed();
            Serial.println("[Stamp] WiFi connection failed");
        }
    }
    
    // Sync time periodically
    if (WiFi.status() == WL_CONNECTED && 
        (millis() - lastTimeSync > TIME_SYNC_INTERVAL || lastTimeSync == 0)) {
        configTime(timezone_offset * 3600, 0, "pool.ntp.org", "time.nist.gov");
        struct tm timeinfo;
        if (getLocalTime(&timeinfo, 10000)) {
            char timeStr[64];
            strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
            comm.sendTime(String(timeStr));
            lastTimeSync = millis();
            Serial.printf("[Stamp] Time synced: %s\n", timeStr);
        }
    }
    
    // Fetch weather periodically
    if (WiFi.status() == WL_CONNECTED && lat != "0" && lon != "0" &&
        (millis() - lastWeatherUpdate > WEATHER_INTERVAL || lastWeatherUpdate == 0)) {
        fetchWeather();
        lastWeatherUpdate = millis();
    }
    
    delay(10);
}

void fetchWeather() {
    WiFiClient client;
    String url = "/v1/air-quality?latitude=" + lat + "&longitude=" + lon + 
                 "&current=european_aqi";
    
    if (client.connect(weather_host, 80)) {
        client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                     "Host: " + weather_host + "\r\n" +
                     "Connection: close\r\n\r\n");
        
        // Skip headers
        while (client.connected()) {
            String line = client.readStringUntil('\n');
            if (line == "\r") break;
        }
        
        // Parse JSON response
        String response = client.readString();
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, response);
        
        if (!error) {
            uint16_t aqi = doc["current"]["european_aqi"];
            comm.sendWeather(aqi);
            Serial.printf("[Stamp] Weather AQI: %d\n", aqi);
        } else {
            comm.sendWeatherError();
            Serial.println("[Stamp] Weather parse error");
        }
    } else {
        comm.sendWeatherError();
        Serial.println("[Stamp] Weather connection failed");
    }
    client.stop();
}