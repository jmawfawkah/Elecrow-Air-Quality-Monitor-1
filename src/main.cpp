#include <Arduino.h>
#include <Wire.h>
#include "display/display_init.h"
#include "ui/ui.h"
#include "sensors/sgp30.h"
#include "sensors/sgp40.h"
#include "sensors/mics.h"
#include "sdcard/sd_logger.h"
#include "comm/comm_protocol.h"
#include "lvgl.h"
#include "sensors/aht20.h"

static lv_obj_t * touch_test_label;
static lv_obj_t * touch_test_dot;

static void touch_test_event_cb(lv_event_t * e) {
    lv_indev_t * indev = lv_indev_get_act();
    lv_point_t vect;
    lv_indev_get_point(indev, &vect);

    // Move the dot to the touched coordinate
    lv_obj_set_pos(touch_test_dot, vect.x - 5, vect.y - 5);

    // Update the label with the coordinate
    char buf[32];
    snprintf(buf, sizeof(buf), "X: %d\nY: %d", vect.x, vect.y);
    lv_label_set_text(touch_test_label, buf);
    lv_obj_align(touch_test_label, LV_ALIGN_TOP_LEFT, 10, 10);
}

void show_touch_test_screen() {
    lv_obj_t * scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // Label to show coordinates
    touch_test_label = lv_label_create(scr);
    lv_label_set_text(touch_test_label, "Touch the screen");
    lv_obj_set_style_text_color(touch_test_label, lv_color_hex(0x22d3ee), 0);
    lv_obj_align(touch_test_label, LV_ALIGN_TOP_LEFT, 10, 10);

    // Dot to show touch location
    touch_test_dot = lv_obj_create(scr);
    lv_obj_set_size(touch_test_dot, 10, 10);
    lv_obj_set_style_bg_color(touch_test_dot, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_border_width(touch_test_dot, 0, 0);
    lv_obj_set_style_radius(touch_test_dot, 5, 0);
    lv_obj_clear_flag(touch_test_dot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(touch_test_dot, LV_ALIGN_CENTER, 0, 0);

    // Add event to the screen
    lv_obj_add_event_cb(scr, touch_test_event_cb, LV_EVENT_PRESSING, NULL);
}

// UART to Stamp S3
static HardwareSerial StampSerial(1);

// Sensor update timing
static unsigned long lastSensorSend = 0;
static unsigned long lastStatsUpdate = 0;

// === Comm callbacks (responses from Stamp S3) ===

void onWiFiStatusConnected(const String &ssid, const String &ip) {
    Serial.printf("[Elecrow] WiFi connected: %s (%s)\n", ssid.c_str(), ip.c_str());
    ui_set_wifi(true);
    String status = "WiFi: " + ssid;
    ui_set_status(status.c_str());
}

void onWiFiStatusDisconnected() {
    Serial.println("[Elecrow] WiFi disconnected");
    ui_set_wifi(false);
    ui_set_status("WiFi disconnected");
}

void onWiFiStatusFailed() {
    Serial.println("[Elecrow] WiFi connection failed");
    ui_set_wifi(false);
    ui_set_status("WiFi connection failed");
}

void onScanResult(const String &ssid, int32_t rssi, bool encrypted) {
    Serial.printf("[Elecrow] Network: %s (%d dBm) %s\n", 
                  ssid.c_str(), rssi, encrypted ? "encrypted" : "open");
    extern void addNetworkToList(const String &ssid, int32_t rssi, bool encrypted);
    addNetworkToList(ssid, rssi, encrypted);
}

void onScanDone(int count) {
    Serial.printf("[Elecrow] Scan complete: %d networks\n", count);
    extern void scanComplete(int count);
    scanComplete(count);
}

void onTime(const String &timeStr) {
    ui_set_time(timeStr.c_str());
}

void onWeather(uint16_t aqi) {
    ui_set_outdoor_aqi(aqi);
}

void onWeatherError() {
    ui_set_outdoor_aqi(0);
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n=== Elecrow Air Quality Monitor ===");
    Serial.println("=== Dual-MCU Mode (Display MCU) ===");

    // 1. Initialize I2C bus FIRST
    Serial.println("[Init] Starting I2C bus on SDA=15 SCL=16");
    Wire1.begin(15, 16);
    Wire1.setClock(400000);
    delay(100);

    // 2. Initialize Display (LovyanGFX will NOT touch I2C now)
    initDisplay();
    ui_init();
    //show_touch_test_screen();

    // 3. Initialize UART to Stamp S3
    StampSerial.begin(115200, SERIAL_8N1, STAMP_UART_RX, STAMP_UART_TX);
    comm.begin(StampSerial);
    // ... register callbacks ...

    // 4. Initialize sensors
    Serial.println("\n--- Initializing Sensors ---");
    aht20.begin(Wire1);
    sgp30.begin(Wire1);
    sgp40.begin(Wire1);
    mics.begin(Wire1);

    Serial.println("\n--- Initializing SD Card ---");
    sdLogger.begin();

    ui_set_wifi(false);
    ui_set_status("Tap gear icon to configure WiFi");

    Serial.println("\n=== System Initialized ===");
}

void loop() {
    // Process incoming from Stamp S3
    comm.update();

    // Update sensors
    sgp30.update();
    sgp40.update();
    aht20.update();
    mics.update();

    // Send sensor data to Stamp S3 every 5 seconds
    if (millis() - lastSensorSend > 5000) {
        lastSensorSend = millis();
        if (sgp30.isInitialized() || sgp40.isInitialized()) {
            comm.sendSensorData(
                sgp30.getECO2(),
                sgp30.getTVOC(),
                sgp40.getVocIndex(),
                mics.getCO(),
                mics.getNO2()
            );
        }
    }

    // Log to SD card
    if (sgp30.isInitialized() || sgp40.isInitialized()) {
        sdLogger.log(
            String(sgp30.getECO2()),
            String(sgp30.getTVOC()),
            String(sgp40.getVocIndex()),
            String(mics.getCO(), 2),
            String(mics.getNO2(), 4),
            "0"
        );
    }

    // Update CPU/RAM stats every 10 seconds
    if (millis() - lastStatsUpdate > 10000) {
        lastStatsUpdate = millis();
        update_display_cpu_stats();
    }

    lv_timer_handler();
    delay(5);
}