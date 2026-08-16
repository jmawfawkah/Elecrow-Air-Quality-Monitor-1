#ifndef SGP30_H
#define SGP30_H

#include <Arduino.h>
#include <Adafruit_SGP30.h>

// SGP30 sensor manager
class SGP30Sensor {
public:
    bool begin(TwoWire &wire);
    void update();  // Call this every second
    bool isInitialized() const { return _initialized; }
    
    uint16_t getECO2() const { return _eco2; }
    uint16_t getTVOC() const { return _tvoc; }
    
private:
    Adafruit_SGP30 _sgp;
    bool _initialized = false;
    uint16_t _eco2 = 0;
    uint16_t _tvoc = 0;
    unsigned long _lastRead = 0;
    unsigned long _startTime = 0;
};

extern SGP30Sensor sgp30;

#endif