#ifndef WIFI_SCREEN_H
#define WIFI_SCREEN_H

#include <lvgl.h>
#include <Arduino.h>   // Add this line!

lv_obj_t * createWifiScreen();
void show_wifi_screen();
void addNetworkToList(const String &ssid, int32_t rssi, bool encrypted);
void updateWifiStatus(const char * status);
void clearNetworkList();
void scanComplete(int count);

#endif