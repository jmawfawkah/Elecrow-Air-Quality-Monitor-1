#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

#ifndef DEVICE_NAME
#define DEVICE_NAME "Elecrow Advance 7.0 S3"
#endif

// =============================================================================
//  Elecrow CrowPanel Advance 7.0" HMI
//  ESP32-S3-WROOM-1-N16R8, 7.0" IPS Display over SPI (TFT_eSPI backend),
//  GT911 capacitive touch over I2C. Concrete pin assignments live in the build
//  flags (boards/elecrow_s3_7/elecrow_s3_7.ini); this header only
//  provides the standard Arduino aliases the core/libraries expect.
// =============================================================================

static const uint8_t TX = 43;
static const uint8_t RX = 44;

// GT911 capacitive touch I2C bus
static const uint8_t SDA = 15;
static const uint8_t SCL = 16;

// Display / SD share SPI signals; chip-selects differ (see board .ini)
// Note: SD Card CS is hardwired to 3.3V, so SS is only for the LCD.
static const uint8_t SS = 7;    // TFT_CS
static const uint8_t MOSI = 6;  // TFT_MOSI / SD_MOSI
static const uint8_t MISO = 4;  // SD_MISO (TFT MISO is unused, -1)
static const uint8_t SCK = 5;   // TFT_SCLK / SD_SCK

// Additional pin aliases for 7.0" model
static const uint8_t TFT_DC_PIN = 8;
static const uint8_t TFT_BL_V10 = 1; // Backlight control for V1.0 hardware
// V1.2+ backlight is controlled by STC8H1K28 microcontroller via I2C (address 0x30)

#endif /* Pins_Arduino_h */
