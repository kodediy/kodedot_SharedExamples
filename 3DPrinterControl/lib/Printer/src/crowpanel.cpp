#include "crowpanel.h"
#include <wifi_manager_lib/WiFiManager.h>

// Define the global crowpanel_config variable
crowpanel_config_t crowpanel_config;
crowpanel_wifi_scan_t crowpanel_wifi_scan;

// Default values for Wi-Fi and configuration
void crowpanel_init(void) {
    // *** IMPORTANT: Replace with your actual WiFi name (SSID) and password ***
    strcpy(crowpanel_config.sta_ssid, "****");    // <-- CHANGE THIS to your WiFi name
    strcpy(crowpanel_config.sta_pwd, "****");    // <-- CHANGE THIS to your WiFi password
    strcpy(crowpanel_config.hostname, "crowpanel");
    
    // *** IMPORTANT: Replace with your actual printer IP address ***
    strcpy(crowpanel_config.moonraker_ip, "****");  // <-- CHANGE THIS to your printer's IP address
    strcpy(crowpanel_config.moonraker_port, "7125");         // <-- Verify this is your Moonraker port
    strcpy(crowpanel_config.moonraker_tool, "tool0");        // Default extruder name
    strcpy(crowpanel_config.mode, "sta");                    // WiFi in station mode (client)
    
    // Initialize Wi-Fi scan data
    crowpanel_wifi_scan.count = 0;
    for (int i = 0; i < SCAN_SSIDS_NUM; i++) {
        crowpanel_wifi_scan.rssi[i] = 0;
        memset(crowpanel_wifi_scan.ssid[i], 0, sizeof(crowpanel_wifi_scan.ssid[i]));
        crowpanel_wifi_scan.authmode[i] = WIFI_AUTH_OPEN;
        crowpanel_wifi_scan.connected[i] = 0;
    }
}

// Map Arduino WiFi status to our enum
wifi_status_t wifi_get_connect_status(void) {
    wl_status_t st = WiFi.status();
    switch (st) {
        case WL_CONNECTED:     return WIFI_STATUS_CONNECTED;
        case WL_IDLE_STATUS:   return WIFI_STATUS_CONNECTING;
        case WL_DISCONNECTED:  return WIFI_STATUS_DISCONNECTED;
        case WL_CONNECTION_LOST:
        case WL_CONNECT_FAILED:
        default:               return WIFI_STATUS_ERROR;
    }
}

void wifi_connect(void) {
    // Use WiFiManager to load credentials (if not already) and connect
    wifiManager.ensureSDMounted();
    wifiManager.loadCredentialsFromSD();
    wifiManager.connectToWiFi();
}

void wifi_task(void *parameter) {
    // Minimal background task; keep alive without interfering with WiFiManager
    for(;;) {
        // Optionally, reconnect if dropped
        if (WiFi.status() != WL_CONNECTED) {
            // Avoid tight loops
            // Try reconnect with a gentle cadence
            wifi_connect();
        }
        delay(5000);
    }
}