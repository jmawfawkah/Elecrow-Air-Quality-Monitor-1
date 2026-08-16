#include "sd_logger.h"

SDLogger sdLogger;

// SD Card SPI pins (shared with display SPI bus)
// SCK=5, MISO=4, MOSI=6, CS=hardwired to 3.3V
// Since CS is hardwired, we use SPI.begin with CS=-1

String SDLogger::generateFilename() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return "/aqlog_" + String(millis()) + ".csv";
    }
    
    char filename[32];
    snprintf(filename, sizeof(filename), "/aqlog_%04d%02d%02d.csv",
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
    return String(filename);
}

bool SDLogger::begin() {
    Serial.println("[SD] Initializing SD card...");
    
    // Initialize SPI with SD card pins
    // CS is hardwired to 3.3V, so we pass -1
    SPIClass spi = SPIClass(HSPI);
    spi.begin(SDCARD_SCK, SDCARD_MISO, SDCARD_MOSI, -1);
    
    if (!SD.begin(-1, spi, 8000000)) {
        Serial.println("[SD] Card mount failed!");
        _initialized = false;
        return false;
    }
    
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("[SD] No SD card attached!");
        _initialized = false;
        return false;
    }
    
    Serial.printf("[SD] Card type: %s\n", 
                  cardType == CARD_MMC ? "MMC" :
                  cardType == CARD_SD ? "SD" :
                  cardType == CARD_SDHC ? "SDHC" : "Unknown");
    Serial.printf("[SD] Card size: %lluMB\n", SD.cardSize() / (1024 * 1024));
    
    _currentFile = generateFilename();
    _initialized = true;
    _headerWritten = false;
    
    Serial.printf("[SD] Log file: %s\n", _currentFile.c_str());
    return true;
}

void SDLogger::log(const String& eco2, const String& tvoc, const String& vocIndex,
                   const String& co, const String& no2, const String& outdoorAqi) {
    if (!_initialized) return;
    
    // Log every 30 seconds
    if (millis() - _lastLog < 30000) return;
    _lastLog = millis();
    
    File file = SD.open(_currentFile, FILE_APPEND);
    if (!file) {
        Serial.println("[SD] Failed to open file for appending!");
        return;
    }
    
    // Write header on first log
    if (!_headerWritten) {
        file.println("Timestamp,eCO2(ppm),TVOC(ppb),VOC_Index,CO(ppm),NO2(ppm),Outdoor_AQI");
        _headerWritten = true;
    }
    
    // Get timestamp
    String timestamp;
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        char ts[24];
        strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &timeinfo);
        timestamp = String(ts);
    } else {
        timestamp = String(millis());
    }
    
    // Write data row
    String row = timestamp + "," + eco2 + "," + tvoc + "," + vocIndex + "," +
                 co + "," + no2 + "," + outdoorAqi;
    file.println(row);
    file.close();
    
    Serial.printf("[SD] Logged: %s\n", row.c_str());
}