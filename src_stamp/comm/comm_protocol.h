#ifndef COMM_PROTOCOL_H
#define COMM_PROTOCOL_H

#include <Arduino.h>

class CommProtocol {
public:
    void begin(Stream &serial);
    void update();
    
    // Send commands (Elecrow → Stamp S3)
    void sendWiFiConnect(const String &ssid, const String &password);
    void sendWiFiDisconnect();
    void sendWiFiScan();
    void sendSensorData(uint16_t eco2, uint16_t tvoc, uint16_t vocIndex, 
                        float co, float no2);
    void sendTimezone(int offset);
    void sendLocation(const String &lat, const String &lon);
    
    // Send responses (Stamp S3 → Elecrow)
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
};

extern CommProtocol comm;

#endif