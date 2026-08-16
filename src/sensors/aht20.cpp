#include "aht20.h"
#include "../ui/ui.h"
#include "ui/settings_screen.h"

AHT20Sensor aht20;

bool AHT20Sensor::begin(TwoWire &wire) {
    if (!_aht.begin(&wire)) {
        Serial.println("[AHT20] Sensor not found!");
        _initialized = false;
        return false;
    }
    
    Serial.println("[AHT20] Sensor initialized successfully");
    _initialized = true;
    return true;
}

void AHT20Sensor::update() {
    if (!_initialized) return;
    
    // Read every 2 seconds (AHT20 doesn't need 1s like SGP30)
    if (millis() - _lastRead < 2000) return;
    _lastRead = millis();
    
    _aht.getEvent(&_hum, &_temp);
    
    // Convert to Fahrenheit and apply calibration offsets
    // Based on reference meters: Temp reads ~3.8°F high, Humidity reads ~8.8% low
    _temperature = ((_temp.temperature * 9.0 / 5.0) + 32.0) + aht20_temp_offset;
    _humidity = _hum.relative_humidity + aht20_hum_offset;
    
    // Prevent humidity from exceeding 100% or going negative
    if (_humidity > 100.0) _humidity = 100.0;
    if (_humidity < 0.0) _humidity = 0.0;
    
    // Update UI
    ui_set_temp(_temperature);
    ui_set_humidity(_humidity);
    
    // Print every 5 seconds
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 5000) {
        Serial.printf("[AHT20] Temp: %.1f°F, Humidity: %.1f%%RH\n", 
                      _temperature, _humidity);
        lastPrint = millis();
    }
}