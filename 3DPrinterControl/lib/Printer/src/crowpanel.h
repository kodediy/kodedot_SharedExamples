#ifndef CROWPANEL_H
#define CROWPANEL_H

#include <WiFi.h>
#include <lvgl.h>
#include <Arduino.h>

// WiFi scan configuration
#define SCAN_SSIDS_NUM 20

typedef struct {
    int32_t rssi[SCAN_SSIDS_NUM]; // RSSI returned RSSI values
    char ssid[SCAN_SSIDS_NUM][65]; // SSIDs returned by the wifi scan.
    wifi_auth_mode_t authmode[SCAN_SSIDS_NUM]; // encryptionType
    uint8_t connected[SCAN_SSIDS_NUM];
    uint8_t count; // Number of WiFi Scanned
} crowpanel_wifi_scan_t;

typedef struct {
    char sta_ssid[33];
    char sta_pwd[65];
    wifi_auth_mode_t sta_auth;
    char ap_ssid[33];
    char ap_pwd[65];
    char hostname[16];
    char moonraker_ip[65]; // "192.168.255.255" or "<hostname>.local"
    char moonraker_port[6]; // "65536", max_len=6
    char moonraker_tool[7]; // "tool0"
    char mode[6]; // "ap":WIFI_MODE_AP, "sta":WIFI_MODE_STA, "apsta":WIFI_MODE_APSTA
    lv_color_t theme_color;
} crowpanel_config_t;

// WiFi status enum
typedef enum {
    WIFI_STATUS_DISCONNECTED,
    WIFI_STATUS_CONNECTING,
    WIFI_STATUS_CONNECTED,
    WIFI_STATUS_ERROR
} wifi_status_t;

extern crowpanel_config_t crowpanel_config;
extern crowpanel_wifi_scan_t crowpanel_wifi_scan;

void wifi_task(void *parameter);
void wifi_connect(void);
wifi_status_t wifi_get_connect_status(void);
void crowpanel_init(void);

#endif 