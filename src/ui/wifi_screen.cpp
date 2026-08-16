#include "wifi_screen.h"
#include "settings_screen.h"
#include <lvgl.h>
#include <Arduino.h>
#include "comm/comm_protocol.h"

extern lv_obj_t *main_screen;
extern CommProtocol comm;

// UI objects
static lv_obj_t * wifi_screen = nullptr;
static lv_obj_t * network_list = nullptr;
static lv_obj_t * status_label = nullptr;

// Password modal objects
static lv_obj_t * pass_modal = nullptr;
static lv_obj_t * pass_ta = nullptr;
static lv_obj_t * pass_kb = nullptr;
static String selected_ssid = "";

// Forward declarations
static const char* getSignalBars(int32_t rssi);
static void network_btn_cb(lv_event_t * e);
static void rescan_btn_cb(lv_event_t * e);
static void disconnect_btn_cb(lv_event_t * e);
static void back_btn_cb(lv_event_t * e);
static void connect_btn_cb(lv_event_t * e);
static void cancel_btn_cb(lv_event_t * e);

// Convert RSSI to signal strength bars
static const char* getSignalBars(int32_t rssi) {
    if (rssi >= -55) return "####";
    if (rssi >= -65) return "### ";
    if (rssi >= -75) return "##  ";
    if (rssi >= -85) return "#   ";
    return "    ";
}

// Callback when Connect button is pressed in the password modal
static void connect_btn_cb(lv_event_t * e) {
    (void)e;
    const char * pass = lv_textarea_get_text(pass_ta);
    Serial.printf("[UI] Connecting to %s\n", selected_ssid.c_str());
    comm.sendWiFiConnect(selected_ssid, String(pass));
    
    if (status_label) {
        String status = "Connecting to " + selected_ssid + "...";
        lv_label_set_text(status_label, status.c_str());
    }

    if (pass_modal) {
        lv_obj_delete(pass_modal);
        pass_modal = nullptr;
        pass_ta = nullptr;
        pass_kb = nullptr;
    }
}

// Callback when Cancel button is pressed in the password modal
static void cancel_btn_cb(lv_event_t * e) {
    (void)e;
    if (pass_modal) {
        lv_obj_delete(pass_modal);
        pass_modal = nullptr;
        pass_ta = nullptr;
        pass_kb = nullptr;
    }
}

// Callback when a network button is tapped
static void network_btn_cb(lv_event_t * e) {
    lv_obj_t * btn = (lv_obj_t *)lv_event_get_target(e);
    lv_obj_t * label = lv_obj_get_child(btn, 0);
    const char * text = lv_label_get_text(label);
    
    if (text == nullptr) return;
    
    String fullText = String(text);
    int spaceIdx = fullText.indexOf("  ");
    selected_ssid = (spaceIdx > 0) ? fullText.substring(0, spaceIdx) : fullText;
    
    Serial.printf("[UI] Network selected: %s\n", selected_ssid.c_str());
    
    // Create password modal overlay
    if (pass_modal) lv_obj_delete(pass_modal);
    
    pass_modal = lv_obj_create(wifi_screen);
    lv_obj_set_size(pass_modal, 780, 460);
    lv_obj_align(pass_modal, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(pass_modal, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(pass_modal, 2, 0);
    lv_obj_set_style_border_color(pass_modal, lv_color_hex(0x555555), 0);
    lv_obj_clear_flag(pass_modal, LV_OBJ_FLAG_SCROLLABLE);

    // Title (SSID)
    lv_obj_t * title = lv_label_create(pass_modal);
    lv_label_set_text(title, selected_ssid.c_str());
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 10, 10);

    // Text Area for password
    pass_ta = lv_textarea_create(pass_modal);
    lv_obj_set_width(pass_ta, 400);
    lv_obj_align(pass_ta, LV_ALIGN_TOP_LEFT, 10, 40);
    lv_textarea_set_placeholder_text(pass_ta, "Password");
    lv_textarea_set_password_mode(pass_ta, true);
    lv_textarea_set_one_line(pass_ta, true);

    // Connect Button
    lv_obj_t * connect_btn = lv_btn_create(pass_modal);
    lv_obj_set_size(connect_btn, 100, 40);
    lv_obj_align_to(connect_btn, pass_ta, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_set_style_bg_color(connect_btn, lv_color_hex(0x4CAF50), 0);
    lv_obj_t * connect_label = lv_label_create(connect_btn);
    lv_label_set_text(connect_label, "Connect");
    lv_obj_center(connect_label);
    lv_obj_add_event_cb(connect_btn, connect_btn_cb, LV_EVENT_CLICKED, nullptr);

    // Cancel Button
    lv_obj_t * cancel_btn = lv_btn_create(pass_modal);
    lv_obj_set_size(cancel_btn, 100, 40);
    lv_obj_align_to(cancel_btn, connect_btn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_set_style_bg_color(cancel_btn, lv_color_hex(0xF44336), 0);
    lv_obj_t * cancel_label = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_label, "Cancel");
    lv_obj_center(cancel_label);
    lv_obj_add_event_cb(cancel_btn, cancel_btn_cb, LV_EVENT_CLICKED, nullptr);

    // Keyboard
    pass_kb = lv_keyboard_create(pass_modal);
    lv_obj_set_height(pass_kb, 200);
    lv_obj_align(pass_kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(pass_kb, pass_ta);
}

// Callback when rescan button is tapped
static void rescan_btn_cb(lv_event_t * e) {
    (void)e;
    Serial.println("[UI] Rescan requested");
    comm.sendWiFiScan();
    if (network_list) {
        lv_obj_clean(network_list);
    }
    if (status_label) {
        lv_label_set_text(status_label, "Scanning...");
    }
}

// Callback when disconnect button is tapped
static void disconnect_btn_cb(lv_event_t * e) {
    (void)e;
    Serial.println("[UI] Disconnect requested");
    comm.sendWiFiDisconnect();
    if (status_label) {
        lv_label_set_text(status_label, "Disconnected");
    }
}

// Callback when back button is tapped
 static void back_btn_cb(lv_event_t * e) {
    (void)e;
    Serial.println("[UI] WiFi back button pressed");
    lv_screen_load(main_screen);
}

// Add a network to the scrollable list
void addNetworkToList(const String &ssid, int32_t rssi, bool encrypted) {
    if (!network_list) return;

    lv_obj_t * btn = lv_btn_create(network_list);
    lv_obj_set_width(btn, lv_pct(100));
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
    
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x2E2E2E), 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x4A4A4A), (lv_style_selector_t)(LV_PART_MAIN | LV_STATE_CHECKED));
    
    lv_obj_t * label = lv_label_create(btn);
    String text = ssid + "  " + getSignalBars(rssi);
    if (encrypted) text += "  *";
    lv_label_set_text(label, text.c_str());
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    
    lv_obj_add_event_cb(btn, network_btn_cb, LV_EVENT_CLICKED, nullptr);
}

// Show the Wi-Fi screen
void show_wifi_screen() {
    if (!wifi_screen) {
        createWifiScreen();
    }
    lv_screen_load(wifi_screen);
}

// Create the Wi-Fi settings screen
lv_obj_t * createWifiScreen() {
    wifi_screen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(wifi_screen, lv_color_hex(0x1A1A1A), 0);
    lv_obj_clear_flag(wifi_screen, LV_OBJ_FLAG_SCROLLABLE);
    
    // Back button
    lv_obj_t * back_btn = lv_btn_create(wifi_screen);
    lv_obj_set_size(back_btn, 60, 40);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x555555), 0);
    lv_obj_t * back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "< Back");
    lv_obj_center(back_label);
    lv_obj_add_event_cb(back_btn, back_btn_cb, LV_EVENT_CLICKED, nullptr);
    
    // Title
    lv_obj_t * title = lv_label_create(wifi_screen);
    lv_label_set_text(title, "Wi-Fi Networks");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // Status label
    status_label = lv_label_create(wifi_screen);
    lv_label_set_text(status_label, "Idle");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 35);
    
    // Network list
    network_list = lv_obj_create(wifi_screen);
    lv_obj_set_size(network_list, 760, 340);
    lv_obj_align(network_list, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_color(network_list, lv_color_hex(0x222222), 0);
    lv_obj_set_style_border_width(network_list, 0, 0);
    lv_obj_set_style_pad_all(network_list, 5, 0);
    lv_obj_set_flex_flow(network_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(network_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    
    // Rescan button
    lv_obj_t * rescan_btn = lv_btn_create(wifi_screen);
    lv_obj_set_size(rescan_btn, 120, 40);
    lv_obj_align(rescan_btn, LV_ALIGN_BOTTOM_LEFT, 20, -10);
    lv_obj_set_style_bg_color(rescan_btn, lv_color_hex(0x2196F3), 0);
    lv_obj_t * rescan_label = lv_label_create(rescan_btn);
    lv_label_set_text(rescan_label, "Rescan");
    lv_obj_center(rescan_label);
    lv_obj_add_event_cb(rescan_btn, rescan_btn_cb, LV_EVENT_CLICKED, nullptr);
    
    // Disconnect button
    lv_obj_t * disconnect_btn = lv_btn_create(wifi_screen);
    lv_obj_set_size(disconnect_btn, 120, 40);
    lv_obj_align(disconnect_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -10);
    lv_obj_set_style_bg_color(disconnect_btn, lv_color_hex(0xF44336), 0);
    lv_obj_t * disconnect_label = lv_label_create(disconnect_btn);
    lv_label_set_text(disconnect_label, "Disconnect");
    lv_obj_center(disconnect_label);
    lv_obj_add_event_cb(disconnect_btn, disconnect_btn_cb, LV_EVENT_CLICKED, nullptr);
    
    return wifi_screen;
}

// Update the status label
void updateWifiStatus(const char * status) {
    // Close password modal if open
    if (pass_modal) {
        lv_obj_delete(pass_modal);
        pass_modal = nullptr;
        pass_ta = nullptr;
        pass_kb = nullptr;
    }
    
    if (status_label) {
        lv_label_set_text(status_label, status);
    }
    
    // Check if status starts with "Connected"
    if (status && strncmp(status, "Connected", 9) == 0) {
        Serial.println("[UI] WiFi connected - auto-navigating to main screen");
        // Longer delay (3 seconds) to let UI settle before navigating
        lv_timer_t * timer = lv_timer_create([](lv_timer_t * t) {
            lv_screen_load(main_screen);
            lv_timer_del(t);
        }, 3000, nullptr);
    }
}

// Clear the network list
void clearNetworkList() {
    if (network_list) {
        lv_obj_clean(network_list);
    }
}

// Called when the Wi-Fi scan is complete
void scanComplete(int count) {
    if (status_label) {
        String text = "Scan complete: " + String(count) + " networks found";
        lv_label_set_text(status_label, text.c_str());
    }
}