#pragma once
#include <Adafruit_AHTX0.h>
#include <Wire.h>

class AHT20Sensor {
public:
    bool begin(TwoWire &wire);
    void update();
    bool isInitialized() { return _initialized; }
    
    float getTemperature() { return _temperature; }  // °F
    float getHumidity() { return _humidity; }         // %RH
    
private:
    Adafruit_AHTX0 _aht;
    sensors_event_t _hum, _temp;
    float _temperature = 0.0;
    float _humidity = 0.0;
    bool _initialized = false;
    unsigned long _lastRead = 0;
};

extern AHT20Sensor aht20;