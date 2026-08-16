#include <Arduino.h>
#include <Wire.h>

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n=== I2C SCANNER ===");
    
    Wire1.begin(15, 16);
    Wire1.setClock(100000); // Use slow speed for scanning
    
    delay(500);
    
    byte count = 0;
    Serial.println("Scanning addresses 1-127...");
    for (byte i = 1; i < 128; i++) {
        Wire1.beginTransmission(i);
        byte status = Wire1.endTransmission();
        if (status == 0) {
            Serial.printf("Found device at 0x%02X\n", i);
            count++;
        }
    }
    
    if (count == 0) {
        Serial.println("No I2C devices found! Check wiring.");
    } else {
        Serial.printf("Scan complete. Found %d device(s).\n", count);
    }
}

void loop() {
    delay(10000);
}