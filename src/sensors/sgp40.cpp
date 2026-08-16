#include "sgp40.h"
#include "../ui/ui.h"

SGP40Sensor sgp40;

bool SGP40Sensor::begin(TwoWire &wire) {
    if (!_sgp.begin(wire)) {
        Serial.println("[SGP40] Sensor not found!");
        _initialized = false;
        return false;
    }
    
    Serial.println("[SGP40] Sensor initialized successfully");
    _initialized = true;
    return true;
}

void SGP40Sensor::update() {
    if (!_initialized) return;
    
    // SGP40 VOC index can be read every 1 second
    if (millis() - _lastRead < 1000) return;
    _lastRead = millis();
    
    _vocIndex = _sgp.getVOCindex();
    ui_set_voc_index(_vocIndex);
    
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 5000) {
        Serial.printf("[SGP40] VOC index: %d\n", _vocIndex);
        lastPrint = millis();
    }
}