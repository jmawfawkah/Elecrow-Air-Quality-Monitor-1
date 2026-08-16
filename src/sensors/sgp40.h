#ifndef SGP40_H
#define SGP40_H

#include <Arduino.h>
#include <SparkFun_SGP40_Arduino_Library.h>

class SGP40Sensor {
public:
    bool begin(TwoWire &wire);
    void update();
    bool isInitialized() const { return _initialized; }
    
    uint16_t getVocIndex() const { return _vocIndex; }
    
private:
    SGP40 _sgp;
    bool _initialized = false;
    uint16_t _vocIndex = 0;
    unsigned long _lastRead = 0;
};

extern SGP40Sensor sgp40;

#endif