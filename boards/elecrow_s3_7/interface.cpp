#include "core/bus_HAL.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include <Arduino.h>
#include <Wire.h>
#include <interface.h>

// =============================================================================
//  CrowPanel Advance 7.0" (ESP32-S3) interface
//  - Display: 7.0" IPS over SPI (handled by TFT_eSPI)
//  - Touch:   GT911 capacitive, directly on I2C (SDA=15, SCL=16, INT=47),
//             no IO expander, no dedicated RST line.
//  - Backlight: V1.0: direct PWM on GPIO1. V1.2+: controlled by STC8H1K28 microcontroller via I2C (address 0x30).
// =============================================================================

#if defined(HAS_CAPACITIVE_TOUCH) && defined(TOUCH_GT911_I2C)
#include "TouchDrvGT911.hpp"
TouchDrvGT911 touch;
struct TouchPointPro {
    int16_t x = 0;
    int16_t y = 0;
};
#endif

// For V1.2+ backlight control via STC8H1K28
#define STC8H1K28_I2C_ADDR 0x30
#define STC8H1K28_BL_ON 0x00 // Max brightness (the Elecrow example uses 0x00)
#define STC8H1K28_BL_OFF 0xF5 // Backlight off (245)

bool setBacklightSTC(uint8_t value);

static bool probeI2CAddress(uint8_t address) {
    Wire1.beginTransmission(address);
    return Wire1.endTransmission() == 0;
}

static void wakePanelAndBacklight() {
    // The official Elecrow example powers the panel by probing the I2C controller,
    // then toggling the wake line before sending the initial backlight command.
    for (int attempt = 0; attempt < 4; ++attempt) {
        if (probeI2CAddress(STC8H1K28_I2C_ADDR) && probeI2CAddress(GT911_SLAVE_ADDRESS_L)) {
            Serial.println("Elecrow panel controller and touch controller detected");
            break;
        }

        Serial.println("Elecrow panel not yet ready; waiting for controller");
#if defined(BACKLIGHT) && BACKLIGHT != -1
        pinMode(BACKLIGHT, OUTPUT);
        digitalWrite(BACKLIGHT, LOW);
        delay(120);
        pinMode(BACKLIGHT, INPUT);
#else
        delay(120);
#endif
        delay(100);
    }

    // V1.3+ uses direct brightness values with 0x00 = max brightness.
    // Send the initial wake command directly, without the older 0x10 preamble.
    setBacklightSTC(STC8H1K28_BL_ON);
}

bool setBacklightSTC(uint8_t value) {
    Wire1.beginTransmission(STC8H1K28_I2C_ADDR);
    if (Wire1.write(0x10) != 1) {
        Wire1.endTransmission();
        return false;
    }
    if (Wire1.write(value) != 1) {
        Wire1.endTransmission();
        return false;
    }
    return Wire1.endTransmission() == 0;
}

/***************************************************************************************
** Function name: _setup_gpio()
***************************************************************************************/
void _setup_gpio() {
    bruceConfig.colorInverted = 0;

#if defined(HAS_CAPACITIVE_TOUCH) && defined(TOUCH_GT911_I2C)
    // Bring up the I2C bus the GT911 lives on.
    setSysI2CBus(&Wire1);
    // Wire1 is already initialized in main.cpp to share with sensors
    // Wire1.begin(SYS_I2C_SDA, SYS_I2C_SCL);

    // Wake the Elecrow panel/controller before enabling the LCD and touch stack.
    wakePanelAndBacklight();

    // Official v1.3+ reference uses polling-only touch: no INT/RST wiring.
    // Pass -1 for both so SensorLib never touches the LCD data pins.
    touch.setPins(-1, -1);
    if (!touch.begin(Wire1, GT911_SLAVE_ADDRESS_L, SYS_I2C_SDA, SYS_I2C_SCL)) {
        Serial.println("Failed to find GT911 touch - check wiring!");
    } else {
        Serial.println("GT911 touch started (polling mode)");
    }
#endif
}

/***************************************************************************************
** Function name: _post_setup_gpio()
***************************************************************************************/
void _post_setup_gpio() {
    // For V1.0
    #if defined(BACKLIGHT) && BACKLIGHT != -1
        pinMode(BACKLIGHT, OUTPUT);
        ledcAttach(BACKLIGHT, TFT_BRIGHT_FREQ, TFT_BRIGHT_Bits);
        ledcWrite(BACKLIGHT, 255);
        setBacklightSTC(STC8H1K28_BL_ON);
    #endif

    // For V1.2+, initialize the STC8H1K28 backlight
    // Example: Turn on backlight at max brightness
    #if defined(BACKLIGHT) && BACKLIGHT == -1
        // Initialize I2C for STC8H1K28 if not already done by touch.
        // Use the V1.3+ direct brightness command rather than the older 0x10 preamble.
        setBacklightSTC(STC8H1K28_BL_ON);
    #endif
}

/***************************************************************************************
** Function name: getBattery()
***************************************************************************************/
int getBattery() { return 100; }

/*********************************************************************
** Function: _setBrightness
**********************************************************************/
void _setBrightness(uint8_t brightval) {
    // For V1.0
    #if defined(BACKLIGHT) && BACKLIGHT != -1
        int dutyCycle;
        if (brightval == 100) dutyCycle = 255;
        else if (brightval == 75) dutyCycle = 130;
        else if (brightval == 50) dutyCycle = 70;
        else if (brightval == 25) dutyCycle = 20;
        else if (brightval == 0) dutyCycle = 0;
        else dutyCycle = ((brightval * 255) / 100);
        ledcWrite(BACKLIGHT, dutyCycle);
    #endif

    // For V1.2+, map 0-100 to 244-0 (0=max, 244=min, 245=off)
    #if defined(BACKLIGHT) && BACKLIGHT == -1
        if (brightval == 0) {
            setBacklightSTC(0xF5); // 245
        } else {
            setBacklightSTC(0x10);
            delay(10);
            // Map 1-100 to 244-0
            uint8_t stc_value = 244 - ((brightval - 1) * 244) / 99;
            setBacklightSTC(stc_value);
        }
    #endif
}

/*********************************************************************
** Function: InputHandler (GT911 capacitive)
**********************************************************************/
void InputHandler(void) {
#if defined(HAS_CAPACITIVE_TOUCH) && defined(TOUCH_GT911_I2C)
    static long d_tmp = 0;
    if (millis() - d_tmp > 200 || LongPress) {
        static unsigned long tm = millis();
        TouchPointPro t;
        uint8_t touched = 0;
        static uint8_t rot = 5;

        if (rot != bruceConfigPins.rotation) {
            if (bruceConfigPins.rotation == 1) {
                touch.setMaxCoordinates(TFT_HEIGHT, TFT_WIDTH);
                touch.setSwapXY(true);
                touch.setMirrorXY(false, true);
            }
            if (bruceConfigPins.rotation == 3) {
                touch.setMaxCoordinates(TFT_HEIGHT, TFT_WIDTH);
                touch.setSwapXY(true);
                touch.setMirrorXY(true, false);
            }
            if (bruceConfigPins.rotation == 0) {
                touch.setMaxCoordinates(TFT_WIDTH, TFT_HEIGHT);
                touch.setSwapXY(false);
                touch.setMirrorXY(false, false);
            }
            if (bruceConfigPins.rotation == 2) {
                touch.setMaxCoordinates(TFT_WIDTH, TFT_HEIGHT);
                touch.setSwapXY(false);
                touch.setMirrorXY(true, true);
            }
            rot = bruceConfigPins.rotation;
        }

        static bool lastTouchState = false;
        static unsigned long lastTouchTime = 0;

        touched = touch.getPoint(&t.x, &t.y);
        bool currentTouchState = touched > 0;

        if (currentTouchState && !lastTouchState && (millis() - lastTouchTime) > 100) {
            lastTouchTime = millis();
        } else if (!currentTouchState || lastTouchState) {
            touched = 0;
        }
        lastTouchState = currentTouchState;

        if (((millis() - tm) > 190 || LongPress) && touched) {
            tm = millis();
            if (!wakeUpScreen()) AnyKeyPress = true;
            else goto END;

#if defined(DISPLAY_VIEWPORT_W) && defined(DISPLAY_VIEWPORT_H)
            t.x = constrain(t.x - ((tft.width() - DISPLAY_VIEWPORT_W) / 2), 0, DISPLAY_VIEWPORT_W - 1);
            t.y = constrain(t.y - ((tft.height() - DISPLAY_VIEWPORT_H) / 2), 0, DISPLAY_VIEWPORT_H - 1);
#endif

            touchPoint.x = t.x;
            touchPoint.y = t.y;
            touchPoint.pressed = true;
            touchHeatMap(touchPoint);
        END:
            d_tmp = millis();
        }
    }
#else
    checkPowerSaveTime();
    PrevPress = false;
    NextPress = false;
    SelPress = false;
    AnyKeyPress = false;
    EscPress = false;
#endif
}

/*********************************************************************
** Function: powerOff
**********************************************************************/
void powerOff() {}

/*********************************************************************
** Function: checkReboot
**********************************************************************/
void checkReboot() {}