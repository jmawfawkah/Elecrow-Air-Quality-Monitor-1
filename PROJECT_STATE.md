# Project Checkpoint - Bedtime Update

## Hardware Setup
- **Elecrow Advance 7.0" HMI** (ESP32-S3, 16MB Flash, 8MB PSRAM) - COM20
  - Display: 800x480 RGB parallel (SC7277 driver), LovyanGFX
  - Touch: GT911 capacitive I2C (addr 0x5D, SDA=15, SCL=16)
  - Backlight: STC8H1K28 via I2C addr 0x30
  - Role: Display, UI (LVGL 9.1), Touch, Sensor reading
- **XIAO ESP32-S3** (Seeed) - COM17
  - Role: WiFi, NTP, Weather API
  - Seated in Elecrow WM port
  - UART: Elecrow IO19(RX)/IO20(TX) ↔ XIAO GPIO43(TX)/GPIO44(RX)

## Build Configuration
- PlatformIO platform-espressif32 55.03.39
- Arduino-ESP32 3.3.9, ESP-IDF 5.5.4
- LVGL 9.1.0, LovyanGFX 1.2.26
- `LGFX_BOUNCE_LINES=30`, `TFT_WRITE_FREQ=21000000`
- `ROTATION=0` (landscape, USB left)
- Two environments: `elecrow_s3_7` and `xiao_s3`
- Bounce buffer patch applied (V3 marker) in Bus_RGB.cpp

## What Works
- ✅ Display renders correctly at 800x480 landscape
- ✅ Touch coordinates register and map correctly
- ✅ WiFi scan: Elecrow sends `WIFI_SCAN`, XIAO scans, results display on Elecrow
- ✅ Password modal with LVGL keyboard
- ✅ WiFi connect: Elecrow sends `WIFI_CONNECT:ssid,password`, XIAO connects
- ✅ Back button navigation (WiFi → Settings → Main dashboard)
- ✅ `main_screen` reference saved in ui.cpp for navigation
- ✅ Protocol mismatch fixed (`WIFI_STATUS:connected,...` format)
- ✅ RAM monitor widget added (shows E/P/X heap stats)

## Current Issues
- Tearing on WiFi connect at 30 bounce lines
- 40 lines causes horrible corruption (SRAM allocation failure)
- 20 lines had more tearing than 30
- PCLK reduction to 16MHz rejected by user
- WiFi is on XIAO, so tearing is from UI update bursts, not radio interference
- WM port power question raised (not yet resolved)

## Files Modified
- `platformio.ini` - bounce lines, PCLK, rotation, build flags
- `src/ui/ui.cpp` - added `main_screen` reference, RAM label, `ui_set_ram_stats()`
- `src/ui/ui.h` - declared `main_screen`, `ram_label`, `ui_set_ram_stats()` extern
- `src/ui/wifi_screen.cpp` - full WiFi screen with password modal, back button, auto-navigate
- `src/ui/wifi_screen.h` - function declarations
- `src/ui/settings_screen.cpp` - back button goes to `main_screen`
- `src/comm/comm_protocol.cpp` - fixed protocol parsing, added ui_set_wifi/time/aqi calls, RAM stats
- `src/comm/comm_protocol.h` - added `sendStampRam()`, `getStampRam()`, `_stampRam`
- `src/main.cpp` - added periodic RAM update in loop()
- `src_stamp/main.cpp` - XIAO firmware (WiFi, NTP, weather, RAM reporting)
- `boards/elecrow_s3_7/patch_lgfx_bounce.py` - bounce buffer patcher

## Next Steps
1. Test RAM monitor widget (verify it displays and updates)
2. Resolve tearing on WiFi connect (watch RAM stats during connect for clues)
3. Verify `ui_set_wifi(true)` updates the main screen icon
4. Verify auto-navigate back to dashboard on successful connection
5. Sensor integration on Elecrow (SGP30, SGP40, MICS)
6. Display sensor data on dashboard cards
7. Weather/time display on UI
8. Investigate WM port power question

## Key Code Patterns
- `main_screen = lv_scr_act()` in `ui_init()` saves dashboard reference
- `lv_screen_load(main_screen)` for back navigation
- `updateWifiStatus()` auto-navigates after 3s delay on "Connected" status
- `ui_set_wifi(bool)`, `ui_set_time(const char*)`, `ui_set_outdoor_aqi(uint16_t)` update dashboard
- `ui_set_ram_stats(elecrowHeap, elecrowPsram, stampHeap)` updates RAM display
- `comm.sendStampRam(ESP.getFreeHeap())` on XIAO reports its RAM to Elecrow

## Communication Protocol
- Elecrow → Stamp: `WIFI_SCAN`, `WIFI_CONNECT:ssid,pass`, `WIFI_DISCONNECT`, `SET_TIMEZONE:offset`, `SET_LOCATION:lat,lon`, `SENSOR_DATA:...`
- Stamp → Elecrow: `WIFI_STATUS:connected,ssid,ip`, `WIFI_STATUS:disconnected`, `WIFI_STATUS:failed`, `SCAN_RESULT:ssid,rssi,enc`, `SCAN_DONE:count`, `TIME:timestr`, `WEATHER:aqi`, `WEATHER:ERROR`, `STAMP_RAM:heap`

---

Save this to `PROJECT_STATE.md` in your project root. Next session, paste it as your first message and I'll have full context immediately. Good night!