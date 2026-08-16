#include "mics.h"
#include "../ui/ui.h"

MICS4514Sensor mics;

bool MICS4514Sensor::begin(TwoWire &wire) {
    _startTime = millis();
    
    // The library's begin() calls _pWire->begin() internally.
    // On ESP32, calling Wire1.begin() again when already initialized is safe.
    if (!_mics.begin()) {
        Serial.println("[MICS] DFRobot MICS4514 not found at 0x75!");
        _initialized = false;
        return false;
    }
    
    // Wake up the sensor (it may be in sleep mode)
    _mics.wakeUpMode();
    
    Serial.println("[MICS] DFRobot MICS4514 initialized successfully");
    Serial.println("[MICS] Waiting 3 minutes for warmup...");
    _initialized = true;
    return true;
}

void MICS4514Sensor::update() {
    if (!_initialized) return;
    
    // DFRobot recommends 3 minutes warmup — use warmUpTime() to check
    // warmUpTime returns false until the warmup period has elapsed
    if (!_mics.warmUpTime(3)) {
        // Still warming up, don't read yet
        return;
    }
    
    // Read every 3 seconds after warmup
    if (millis() - _lastRead < 3000) return;
    _lastRead = millis();
    
    // Read all 6 gas channels using getGasData()
    float co_val = _mics.getGasData(CO);
    float no2_val = _mics.getGasData(NO2);
    float eth_val = _mics.getGasData(C2H5OH);
    float h2_val = _mics.getGasData(H2);
    float nh3_val = _mics.getGasData(NH3);
    float ch4_val = _mics.getGasData(CH4);
    
    // Sanity check (library returns -1 on error)
    if (co_val >= 0) _co = co_val;
    if (no2_val >= 0) _no2 = no2_val;
    if (eth_val >= 0) _eth = eth_val;
    if (h2_val >= 0) _h2 = h2_val;
    if (nh3_val >= 0) _nh3 = nh3_val;
    if (ch4_val >= 0) _ch4 = ch4_val;
    
    // Update UI
    ui_set_co(_co);
    ui_set_no2(_no2);
    ui_set_eth(_eth);
    ui_set_h2(_h2);
    ui_set_nh3(_nh3);
    ui_set_ch4(_ch4);
    
    // Print every 10 seconds
    static unsigned long lastPrint = 0;
    static bool warmupDonePrinted = false;
    
    if (!warmupDonePrinted) {
        Serial.println("[MICS] Warmup complete! Reading gas data...");
        warmupDonePrinted = true;
    }
    
    if (millis() - lastPrint > 10000) {
        Serial.printf("[MICS] CO:%.2f NO2:%.2f ETH:%.2f H2:%.2f NH3:%.2f CH4:%.2f\n",
                      _co, _no2, _eth, _h2, _nh3, _ch4);
        lastPrint = millis();
    }
}