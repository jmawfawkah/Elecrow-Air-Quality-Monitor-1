

### `AIRWATCH_PRO_PROJECT_STATE.txt`

```text
===============================================================================
 AIRWATCH PRO — COMPREHENSIVE PROJECT STATE & EXECUTION PLAN
 Date: August 16, 2026
 Target: Elecrow CrowPanel Advance 7.0" HMI (ESP32-S3) + M5Stack Stamp S3
===============================================================================

-------------------------------------------------------------------------------
1. HARDWARE INVENTORY & STATUS
-------------------------------------------------------------------------------
MAIN BOARD: Elecrow CrowPanel Advance 7.0" HMI (ESP32-S3-WROOM-1-N16R8)
   - 16MB Flash, 8MB OPI PSRAM
   - 7.0" IPS Display, 800x480, Parallel RGB (SC7277 driver IC)
   - GT911 Capacitive Touch (I2C addr 0x5D, SDA=15, SCL=16)
   - STC8H1K28 Backlight Controller (I2C addr 0x30)
   - UART1 for Stamp S3 comm (RX=19, TX=20)

WIFI MODULE: M5Stack Stamp S3
   - Plugs into WM (Wireless Module) port on back of Elecrow
   - Communicates with main board via UART1

SENSORS (currently on shared I2C bus, Wire1):
   - SGP30: eCO2 + TVOC (addr 0x58) — Working
   - AHT20: Temp + Humidity (addr 0x38) — Working (Calibrated: -1.4F, +4.8%)
   - DFRobot MICS-4514: 6-gas (addr 0x75) — Working
   - SGP40: VOC Index (addr 0x59) — Disabled (I2C bus overload conflict)

-------------------------------------------------------------------------------
2. SOFTWARE STACK
-------------------------------------------------------------------------------
BUILD SYSTEM:
   - PlatformIO Core 6.1.19+
   - pioarduino platform-espressif32 55.03.39
   - Arduino-ESP32 3.3.9, ESP-IDF 5.5.4
   - lib_ldf_mode = deep+

LIBRARIES:
   - lovyan03/LovyanGFX@1.2.26 (Display driver with bounce buffer patch)
   - lewisxhe/SensorLib@0.3.4 (TouchDrvGT911)
   - lvgl/lvgl@9.1.0 (UI framework)
   - Adafruit_AHTX0, Adafruit_SGP30, DFRobot_MICS

I2C BUS ARCHITECTURE:
   - `Wire1.begin(15, 16)` called EXACTLY ONCE in main.cpp setup().
   - Devices: 0x30 (BL), 0x38 (AHT20), 0x51 (RTC), 0x58 (SGP30), 0x5D (Touch), 0x75 (MICS)

-------------------------------------------------------------------------------
3. NEW REQUIREMENTS & EXECUTION PLAN
-------------------------------------------------------------------------------
PHASE 1: UI, Layout, and Theme Fixes
   - Settings: Remove " F " from humidity slider label.
   - Sensor Columns: Left = 0x75 (MICS). Right = SGP30, AHT20. Temp/Humidity at top right.
   - Day Mode: Wire day/night theme toggle. (Currently hardcoded to night).

PHASE 2: Chart & History Overhaul
   - AQI History: Add expandable blowup chart for AQI.
   - Data Recording: Use hourly recorded history arrays, not live data streams.
   - CO2 Chart: Fix scaling issue where data isn't showing.
   - Chart Styling: Add right/bottom white borders. Clear X/Y scale labels. 0,0 at bottom-left.

PHASE 3: Settings Expansion
   - Add Display Brightness control (STC8H1K28 I2C).
   - Add Screen Calibration function (GT911 touch).

PHASE 4: SD & WiFi Pin Multiplexing Resolution (See Pinout Plan Below)
   - Hardwire board to bypass S0/S1 switches so SD Card and WM port (Stamp S3) work simultaneously.
   - Remap SPI pins for SD card in platformio.ini and update SD init code.

PHASE 5: Wireless Sensor Architecture
   - Move primary I2C sensors to a separate ESP32 WiFi module.
   - Keep main board I2C port free for plug-and-play additions.
   - Support multi-room remote WiFi modules (Temp/Humidity).
   - Add "Modules" widget that expands to show connected rooms.
   - Add "Add Device" option in settings/expanded screen.

WORKFLOW RULE:
   - ALWAYS plan and confirm before writing code.

-------------------------------------------------------------------------------
4. HARDWARE PINOUT PLAN (PHASE 4 PREP)
-------------------------------------------------------------------------------
PROBLEM:
The Elecrow board uses S0/S1 DIP switches to multiplex IO4, IO5, IO6, IO19, 
and IO20 between the WM (Wireless Module) port and the SD Card slot. 
They cannot operate at the same time.

SOLUTION:
Bypass the switches by hardwiring the signals to dedicated pins.
1. Set DIP switches S0=1, S1=0 (WM Mode). This permanently routes IO19/IO20 
   to the WM port for UART communication with the M5Stack Stamp S3.
2. The SD Card slot will no longer receive signals from the switch. 
   We will run jumper wires from the SD Card slot traces to new, unused 
   ESP32-S3 GPIO pins.

PROPOSED SD CARD REMAP (To be soldered to SD slot traces):
   - SD_MOSI -> GPIO 0 (or other available pin)
   - SD_MISO -> GPIO 2 (or other available pin)
   - SD_SCK  -> GPIO 13 (or other available pin)
   - SD_CS   -> GPIO 14 (or other available pin)
   (Note: Exact pin choices to be confirmed based on available breakout pads 
    and avoiding strapping pins if possible, though IO0/IO2 are manageable 
    if pulled up/down appropriately during boot).

-------------------------------------------------------------------------------
5. REMAINING WORK
-------------------------------------------------------------------------------
1. Execute Phase 1 through 5 in order.
2. Confirm hardware mod pinouts before soldering.
===============================================================================
```
