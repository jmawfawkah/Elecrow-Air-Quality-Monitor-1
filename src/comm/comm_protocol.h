#ifndef COMM_PROTOCOL_H
#define COMM_PROTOCOL_H

#include <Arduino.h>

// =============================================================================
//  Communication protocol between Elecrow (Display) and Stamp S3 (Network)
//  
//  Message format: "CMD:VALUE\n" (simple text, newline-delimited)
//  
//  Elecrow → Stamp S3:
//    "WIFI_CONNECT:ssid,password\n"     - Connect to WiFi
//    "WIFI_DISCONNECT\n"                - Disconnect from WiFi
//    "WIFI_SCAN\n"                      - Scan for networks
//    "SENSOR_DATA:eco2,tvoc,voc,co,no2\n" - Sensor readings
//    "SET_TIMEZONE:offset\n"            - Set timezone offset
//    "SET_LOCATION:lat,lon\n"           - Set weather location
//  
//  Stamp S3 → Elecrow:
//    "WIFI_STATUS:connected,ssid,ip\n"  - WiFi connected
//    "WIFI_STATUS:disconnected\n"       - WiFi disconnected
//    "WIFI_STATUS:failed\n"             - Connection failed
//    "SCAN_RESULT:ssid,rssi,encrypted\n" - One per network found
//    "SCAN_DONE:count\n"                - Scan complete
//    "TIME:HH:MM AM\n"                  - Current time
//    "WEATHER:aqi_value\n"              - Outdoor AQI
//    "WEATHER:ERROR\n"                  - Weather fetch failed
// =============================================================================

class CommProtocol {
public:
    void begin(Stream &serial);
    void update();
    
    // Send commands (Elecrow side)
    void sendWiFiConnect(const String &ssid, const String &password);
    void sendWiFiDisconnect();
    void sendWiFiScan();
    void sendSensorData(uint16_t eco2, uint16_t tvoc, uint16_t vocIndex, 
                        float co, float no2);
    void sendTimezone(int offset);
    void sendLocation(const String &lat, const String &lon);
    
    // Send responses (Stamp S3 side)
    void sendWiFiStatusConnected(const String &ssid, const String &ip);
    void sendWiFiStatusDisconnected();
    void sendWiFiStatusFailed();
    void sendScanResult(const String &ssid, int32_t rssi, bool encrypted);
    void sendScanDone(int count);
    void sendTime(const String &timeStr);
    void sendWeather(uint16_t aqi);
    void sendWeatherError();
    
    // Callbacks - set by the application
    void (*onWiFiConnect)(const String &ssid, const String &password) = nullptr;
    void (*onWiFiDisconnect)() = nullptr;
    void (*onWiFiScan)() = nullptr;
    void (*onSensorData)(uint16_t eco2, uint16_t tvoc, uint16_t voc, 
                         float co, float no2) = nullptr;
    void (*onTimezone)(int offset) = nullptr;
    void (*onLocation)(const String &lat, const String &lon) = nullptr;
    
    void (*onWiFiStatusConnected)(const String &ssid, const String &ip) = nullptr;
    void (*onWiFiStatusDisconnected)() = nullptr;
    void (*onWiFiStatusFailed)() = nullptr;
    void (*onScanResult)(const String &ssid, int32_t rssi, bool encrypted) = nullptr;
    void (*onScanDone)(int count) = nullptr;
    void (*onTime)(const String &timeStr) = nullptr;
    void (*onWeather)(uint16_t aqi) = nullptr;
    void (*onWeatherError)() = nullptr;
    
private:
    Stream *_serial = nullptr;
    String _rxBuffer = "";
    static const int MAX_BUFFER = 512;
    
    void processMessage(const String &msg);
    String extractValue(const String &msg, const String &cmd);
};

extern CommProtocol comm;

#endif