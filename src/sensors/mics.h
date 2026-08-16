#pragma once
#include <Wire.h>
#include "DFRobot_MICS.h"

class MICS4514Sensor {
public:
    bool begin(TwoWire &wire);
    void update();
    bool isInitialized() { return _initialized; }
    
    float getCO() { return _co; }
    float getNO2() { return _no2; }
    float getEth() { return _eth; }
    float getH2() { return _h2; }
    float getNH3() { return _nh3; }
    float getCH4() { return _ch4; }
    
private:
    // Constructor takes POINTER to TwoWire, and address 0x75 = MICS_ADDRESS_0
    DFRobot_MICS_I2C _mics = DFRobot_MICS_I2C(&Wire1, MICS_ADDRESS_0);
    float _co = 0, _no2 = 0, _eth = 0, _h2 = 0, _nh3 = 0, _ch4 = 0;
    bool _initialized = false;
    unsigned long _lastRead = 0;
    unsigned long _startTime = 0;
};

extern MICS4514Sensor mics;