#include "comm_protocol.h"

CommProtocol comm;

void CommProtocol::begin(Stream &serial) {
    _serial = &serial;
}

void CommProtocol::update() {
    if (!_serial) return;
    
    while (_serial->available()) {
        char c = _serial->read();
        if (c == '\n') {
            processMessage(_rxBuffer);
            _rxBuffer = "";
        } else if (c != '\r' && _rxBuffer.length() < MAX_BUFFER) {
            _rxBuffer += c;
        }
    }
}

void CommProtocol::processMessage(const String &msg) {
    if (msg.length() == 0) return;
    
    Serial.printf("[Comm] RX: %s\n", msg.c_str());
    
    // Elecrow → Stamp S3 commands
    if (msg.startsWith("WIFI_CONNECT:")) {
        String params = msg.substring(13);
        int comma = params.indexOf(',');
        if (comma > 0 && onWiFiConnect) {
            String ssid = params.substring(0, comma);
            String password = params.substring(comma + 1);
            onWiFiConnect(ssid, password);
        }
    }
    else if (msg == "WIFI_DISCONNECT" && onWiFiDisconnect) {
        onWiFiDisconnect();
    }
    else if (msg == "WIFI_SCAN" && onWiFiScan) {
        onWiFiScan();
    }
    else if (msg.startsWith("SENSOR_DATA:")) {
        String params = msg.substring(12);
        if (onSensorData) {
            int idx = 0;
            uint16_t eco2 = 0, tvoc = 0, voc = 0;
            float co = 0, no2 = 0;
            
            int start = 0;
            int end = params.indexOf(',', start);
            if (end > 0) { eco2 = params.substring(start, end).toInt(); start = end + 1; }
            
            end = params.indexOf(',', start);
            if (end > 0) { tvoc = params.substring(start, end).toInt(); start = end + 1; }
            
            end = params.indexOf(',', start);
            if (end > 0) { voc = params.substring(start, end).toInt(); start = end + 1; }
            
            end = params.indexOf(',', start);
            if (end > 0) { co = params.substring(start, end).toFloat(); start = end + 1; }
            
            no2 = params.substring(start).toFloat();
            
            onSensorData(eco2, tvoc, voc, co, no2);
        }
    }
    else if (msg.startsWith("SET_TIMEZONE:")) {
        String val = msg.substring(13);
        if (onTimezone) onTimezone(val.toInt());
    }
    else if (msg.startsWith("SET_LOCATION:")) {
        String params = msg.substring(13);
        int comma = params.indexOf(',');
        if (comma > 0 && onLocation) {
            String lat = params.substring(0, comma);
            String lon = params.substring(comma + 1);
            onLocation(lat, lon);
        }
    }
    
    // Stamp S3 → Elecrow responses
    else if (msg.startsWith("WIFI_STATUS:")) {
        String status = msg.substring(12);
        if (status.startsWith("connected")) {
            int firstComma = status.indexOf(',');
            int secondComma = status.indexOf(',', firstComma + 1);
            if (firstComma > 0 && secondComma > 0 && onWiFiStatusConnected) {
                String ssid = status.substring(firstComma + 1, secondComma);
                String ip = status.substring(secondComma + 1);
                onWiFiStatusConnected(ssid, ip);
            }
        }
        else if (status == "disconnected" && onWiFiStatusDisconnected) {
            onWiFiStatusDisconnected();
        }
        else if (status == "failed" && onWiFiStatusFailed) {
            onWiFiStatusFailed();
        }
    }
    else if (msg.startsWith("SCAN_RESULT:")) {
        String params = msg.substring(12);
        int c1 = params.indexOf(',');
        int c2 = params.indexOf(',', c1 + 1);
        if (c1 > 0 && c2 > 0 && onScanResult) {
            String ssid = params.substring(0, c1);
            int32_t rssi = params.substring(c1 + 1, c2).toInt();
            bool encrypted = params.substring(c2 + 1).toInt() == 1;
            onScanResult(ssid, rssi, encrypted);
        }
    }
    else if (msg.startsWith("SCAN_DONE:")) {
        String val = msg.substring(10);
        if (onScanDone) onScanDone(val.toInt());
    }
    else if (msg.startsWith("TIME:")) {
        String timeStr = msg.substring(5);
        if (onTime) onTime(timeStr);
    }
    else if (msg.startsWith("WEATHER:")) {
        String val = msg.substring(8);
        if (val == "ERROR") {
            if (onWeatherError) onWeatherError();
        } else {
            if (onWeather) onWeather(val.toInt());
        }
    }
}

// === Send methods (Elecrow → Stamp S3) ===

void CommProtocol::sendWiFiConnect(const String &ssid, const String &password) {
    if (_serial) {
        _serial->print("WIFI_CONNECT:");
        _serial->print(ssid);
        _serial->print(",");
        _serial->println(password);
    }
}

void CommProtocol::sendWiFiDisconnect() {
    if (_serial) _serial->println("WIFI_DISCONNECT");
}

void CommProtocol::sendWiFiScan() {
    if (_serial) _serial->println("WIFI_SCAN");
}

void CommProtocol::sendSensorData(uint16_t eco2, uint16_t tvoc, uint16_t vocIndex, 
                                   float co, float no2) {
    if (_serial) {
        _serial->print("SENSOR_DATA:");
        _serial->print(eco2);
        _serial->print(",");
        _serial->print(tvoc);
        _serial->print(",");
        _serial->print(vocIndex);
        _serial->print(",");
        _serial->print(co, 2);
        _serial->print(",");
        _serial->println(no2, 4);
    }
}

void CommProtocol::sendTimezone(int offset) {
    if (_serial) {
        _serial->print("SET_TIMEZONE:");
        _serial->println(offset);
    }
}

void CommProtocol::sendLocation(const String &lat, const String &lon) {
    if (_serial) {
        _serial->print("SET_LOCATION:");
        _serial->print(lat);
        _serial->print(",");
        _serial->println(lon);
    }
}

// === Send methods (Stamp S3 → Elecrow) ===

void CommProtocol::sendWiFiStatusConnected(const String &ssid, const String &ip) {
    if (_serial) {
        _serial->print("WIFI_STATUS:connected,");
        _serial->print(ssid);
        _serial->print(",");
        _serial->println(ip);
    }
}

void CommProtocol::sendWiFiStatusDisconnected() {
    if (_serial) _serial->println("WIFI_STATUS:disconnected");
}

void CommProtocol::sendWiFiStatusFailed() {
    if (_serial) _serial->println("WIFI_STATUS:failed");
}

void CommProtocol::sendScanResult(const String &ssid, int32_t rssi, bool encrypted) {
    if (_serial) {
        _serial->print("SCAN_RESULT:");
        _serial->print(ssid);
        _serial->print(",");
        _serial->print(rssi);
        _serial->print(",");
        _serial->println(encrypted ? 1 : 0);
    }
}

void CommProtocol::sendScanDone(int count) {
    if (_serial) {
        _serial->print("SCAN_DONE:");
        _serial->println(count);
    }
}

void CommProtocol::sendTime(const String &timeStr) {
    if (_serial) {
        _serial->print("TIME:");
        _serial->println(timeStr);
    }
}

void CommProtocol::sendWeather(uint16_t aqi) {
    if (_serial) {
        _serial->print("WEATHER:");
        _serial->println(aqi);
    }
}

void CommProtocol::sendWeatherError() {
    if (_serial) _serial->println("WEATHER:ERROR");
}