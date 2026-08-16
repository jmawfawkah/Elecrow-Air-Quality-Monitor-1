#include "display_init.h"
#include <Arduino.h>
#include <Wire.h>
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include "TouchDrvGT911.hpp"

// Create our own touch object for LVGL to use
TouchDrvGT911 lvgl_touch;

// =============================================================================
//  Custom display class - based on official Elecrow LovyanGFX_Driver.h
//  Fixed: touch I2C port changed from I2C_NUM_0 to I2C_NUM_1 to match Wire1
// =============================================================================
class LGFX_Elecrow7 : public lgfx::LGFX_Device {
public:
    lgfx::Bus_RGB _bus_instance;
    lgfx::Panel_RGB _panel_instance;
    lgfx::Touch_GT911 _touch_instance;

    LGFX_Elecrow7(void) {
        {
            auto cfg = _panel_instance.config();
            cfg.memory_width = 800;
            cfg.memory_height = 480;
            cfg.panel_width = 800;
            cfg.panel_height = 480;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            _panel_instance.config(cfg);
        }

        {
            auto cfg = _panel_instance.config_detail();
            cfg.use_psram = 1;
            _panel_instance.config_detail(cfg);
        }

        {
            auto cfg = _bus_instance.config();
            cfg.panel = &_panel_instance;
            cfg.pin_d0 = GPIO_NUM_21;
            cfg.pin_d1 = GPIO_NUM_47;
            cfg.pin_d2 = GPIO_NUM_48;
            cfg.pin_d3 = GPIO_NUM_45;
            cfg.pin_d4 = GPIO_NUM_38;
            cfg.pin_d5 = GPIO_NUM_9;
            cfg.pin_d6 = GPIO_NUM_10;
            cfg.pin_d7 = GPIO_NUM_11;
            cfg.pin_d8 = GPIO_NUM_12;
            cfg.pin_d9 = GPIO_NUM_13;
            cfg.pin_d10 = GPIO_NUM_14;
            cfg.pin_d11 = GPIO_NUM_7;
            cfg.pin_d12 = GPIO_NUM_17;
            cfg.pin_d13 = GPIO_NUM_18;
            cfg.pin_d14 = GPIO_NUM_3;
            cfg.pin_d15 = GPIO_NUM_46;
            cfg.pin_henable = GPIO_NUM_42;
            cfg.pin_vsync = GPIO_NUM_41;
            cfg.pin_hsync = GPIO_NUM_40;
            cfg.pin_pclk = GPIO_NUM_39;
            cfg.freq_write = 18000000;
            cfg.hsync_polarity = 1;
            cfg.hsync_pulse_width = 4;
            cfg.hsync_back_porch = 8;
            cfg.hsync_front_porch = 8;
            cfg.vsync_polarity = 1;
            cfg.vsync_pulse_width = 4;
            cfg.vsync_back_porch = 8;
            cfg.vsync_front_porch = 8;
            cfg.pclk_idle_high = 1;
            _bus_instance.config(cfg);
        }
        _panel_instance.setBus(&_bus_instance);

        {
            auto cfg = _touch_instance.config();
            cfg.x_min = 0;
            cfg.x_max = 800;
            cfg.y_min = 0;
            cfg.y_max = 480;
            cfg.pin_int = -1;
            cfg.bus_shared = false;
            cfg.offset_rotation = 0;
            cfg.i2c_port = I2C_NUM_1;
            cfg.pin_sda = GPIO_NUM_15;
            cfg.pin_scl = GPIO_NUM_16;
            cfg.pin_rst = -1;
            cfg.freq = 400000;
            cfg.i2c_addr = 0x5D;
            _touch_instance.config(cfg);
            // CRITICAL FIX: Disable LovyanGFX touch to prevent I2C conflict
            // interface.cpp handles touch via TouchDrvGT911 instead
            // _panel_instance.setTouch(&_touch_instance);
        } // <-- THIS BRACE WAS MISSING

        setPanel(&_panel_instance);
    }
};

LGFX_Elecrow7 display;

// =============================================================================
//  Backlight - STC8H1K28 at I2C address 0x30
// =============================================================================
#define STC8H1K28_I2C_ADDR  0x30
#define STC8H1K28_BL_ON     0x00
#define STC8H1K28_BL_OFF    0xF5

static bool setBacklightSTC(uint8_t value) {
    Wire1.beginTransmission(STC8H1K28_I2C_ADDR);
    Wire1.write(0x10);
    Wire1.write(value);
    return Wire1.endTransmission() == 0;
}

static bool probeI2CAddress(uint8_t address) {
    Wire1.beginTransmission(address);
    return Wire1.endTransmission() == 0;
}

void initBacklight() {
    // REMOVED Wire1.begin() from here. main.cpp handles it now to prevent conflicts.
    Serial.printf("[Backlight] Using Wire1 on SDA=%d SCL=%d\n", SYS_I2C_SDA, SYS_I2C_SCL);

    for (int attempt = 0; attempt < 4; ++attempt) {
        if (probeI2CAddress(STC8H1K28_I2C_ADDR)) {
            Serial.println("[Backlight] STC8H1K28 controller detected");
            if (probeI2CAddress(GT911_SLAVE_ADDRESS_L)) {
                Serial.println("[Backlight] GT911 touch controller detected");
            } else {
                Serial.println("[Backlight] GT911 touch controller NOT detected yet");
            }
            break;
        }
        Serial.println("[Backlight] Panel not ready, waiting...");

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

    setBacklightSTC(STC8H1K28_BL_ON);
    Serial.println("[Backlight] Backlight ON");
}

// =============================================================================
//  LVGL 9.1 flush callback with DMA synchronization
// =============================================================================
static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    if (display.getStartCount() > 0) {
        display.endWrite();
    }

    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;

    display.pushImageDMA(area->x1, area->y1, w, h, (lgfx::rgb565_t*)px_map);
    display.waitDMA();

    lv_display_flush_ready(disp);
}

// =============================================================================
//  LVGL 9.1 touch read callback
// =============================================================================
static void lvgl_touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {
    int16_t x, y;
    uint8_t touched = lvgl_touch.getPoint(&x, &y);
    if (touched > 0) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = x;
        data->point.y = y;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

// =============================================================================
//  LVGL 9.1 tick callback
// =============================================================================
static uint32_t lvgl_tick_cb(void) {
    return millis();
}

// =============================================================================
//  Initialize LVGL 9.1 with full-screen double buffers in PSRAM
// =============================================================================
void initLVGL() {
    lv_init();
    lv_tick_set_cb(lvgl_tick_cb);

    // Use PARTIAL buffers in INTERNAL SRAM to avoid PSRAM bus contention
    uint32_t buf_lines = 20; // Reduced from 40 to save internal SRAM
    size_t buffer_size = 800 * buf_lines * sizeof(lv_color_t);

    // Allocate in internal DMA-capable SRAM
    void *buf1 = heap_caps_malloc(buffer_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    void *buf2 = heap_caps_malloc(buffer_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

    if (!buf1 || !buf2) {
        Serial.println("Failed to allocate LVGL partial buffers in internal SRAM!");
        Serial.println("Falling back to PSRAM (tearing may occur)");
        if (!buf1) buf1 = heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
        if (!buf2) buf2 = heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM);
    }

    Serial.printf("LVGL buffers allocated: buf1=%p, buf2=%p (%d bytes each)\n", buf1, buf2, buffer_size);

    lv_display_t *disp = lv_display_create(800, 480);
    // Use PARTIAL render mode
    lv_display_set_buffers(disp, buf1, buf2, buffer_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, lvgl_flush_cb);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, lvgl_touch_read_cb);

    Serial.println("LVGL 9.1 initialized with internal SRAM partial buffers");
}

// =============================================================================
//  Main display init
// =============================================================================
void initDisplay() {
    initBacklight();

    display.init();
    display.initDMA();
    display.startWrite();
    display.fillScreen(TFT_BLACK);
    display.setRotation(0);
    display.setColorDepth(16);

    Serial.println("LovyanGFX display initialized with DMA");

    // Initialize GT911 Touch directly here
    Serial.println("[Display] Initializing GT911 touch...");
    lvgl_touch.setPins(-1, -1); // No INT/RST pins wired
    if (lvgl_touch.begin(Wire1, GT911_SLAVE_ADDRESS_L, SYS_I2C_SDA, SYS_I2C_SCL)) {
        Serial.println("[Display] GT911 touch started successfully");
      // We will do the math manually in the callback

      lvgl_touch.setMaxCoordinates(TFT_HEIGHT, TFT_WIDTH);  // 480, 800
    lvgl_touch.setSwapXY(false);
    lvgl_touch.setMirrorXY(false, false);
        
    } else {
        Serial.println("[Display] GT911 touch FAILED to start!");
    }

    initLVGL();
}