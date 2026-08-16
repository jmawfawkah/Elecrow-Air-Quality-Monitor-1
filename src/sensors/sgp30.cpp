#include "sgp30.h"
#include "../ui/ui.h"

SGP30Sensor sgp30;

bool SGP30Sensor::begin(TwoWire &wire) {
    _startTime = millis();
    
    if (!_sgp.begin(&wire)) {
        Serial.println("[SGP30] Sensor not found!");
        _initialized = false;
        return false;
    }
    
    Serial.println("[SGP30] Sensor initialized successfully");
    Serial.printf("[SGP30] Serial number: %08X%08X\n", 
                  _sgp.serialnumber[0], _sgp.serialnumber[1]);
    
    _initialized = true;
    return true;
}

void SGP30Sensor::update() {
    if (!_initialized) return;
    
    // SGP30 requires 1-second intervals for proper baseline calibration
    if (millis() - _lastRead < 1000) return;
    _lastRead = millis();
    
    if (_sgp.IAQmeasure()) {
        _eco2 = _sgp.eCO2;
        _tvoc = _sgp.TVOC;
        
        // Update UI
        ui_set_eco2(_eco2);
        ui_set_tvoc(_tvoc);
        
        // Log warm-up status (first 15 seconds)
        unsigned long elapsed = (millis() - _startTime) / 1000;
        if (elapsed < 15) {
            ui_set_status("SGP30 warming up...");
        } else if (elapsed == 15) {
            ui_set_status("SGP30 ready");
            Serial.println("[SGP30] Warm-up complete");
        }
        
        // Print to serial every 5 seconds
        static unsigned long lastPrint = 0;
        if (millis() - lastPrint > 5000) {
            Serial.printf("[SGP30] eCO2: %d ppm, TVOC: %d ppb\n", _eco2, _tvoc);
            lastPrint = millis();
        }
    } else {
        Serial.println("[SGP30] Measurement failed!");
        ui_set_status("SGP30 read error");
    }
}