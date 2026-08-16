#ifndef SD_LOGGER_H
#define SD_LOGGER_H

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

class SDLogger {
public:
    bool begin();
    void log(const String& eco2, const String& tvoc, const String& vocIndex,
             const String& co, const String& no2, const String& outdoorAqi);
    bool isInitialized() const { return _initialized; }
    String getCurrentFile() const { return _currentFile; }
    
private:
    bool _initialized = false;
    String _currentFile = "";
    unsigned long _lastLog = 0;
    bool _headerWritten = false;
    
    String generateFilename();
};

extern SDLogger sdLogger;

#endif