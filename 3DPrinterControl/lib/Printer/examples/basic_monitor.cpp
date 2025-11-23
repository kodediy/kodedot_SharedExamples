#include <Arduino.h>
#include "crowpanel.h"
#include "moonraker.h"

// Create moonraker instance
MOONRAKER printer;

void setup() {
    Serial.begin(115200);
    
    // Initialize crowpanel configuration
    crowpanel_init();
    
    // *** IMPORTANT: Update these with your actual values ***
    strcpy(crowpanel_config.sta_ssid, "YourWiFiName");        // Your WiFi network name
    strcpy(crowpanel_config.sta_pwd, "YourWiFiPassword");     // Your WiFi password
    strcpy(crowpanel_config.moonraker_ip, "192.168.1.100");   // Your printer's IP address
    strcpy(crowpanel_config.moonraker_port, "7125");          // Moonraker port (usually 7125)
    
    // Configure printer connection
    strcpy(printer.moonraker_ip, crowpanel_config.moonraker_ip);
    strcpy(printer.moonraker_port, crowpanel_config.moonraker_port);
    strcpy(printer.moonraker_tool, crowpanel_config.moonraker_tool);
    
    // Connect to WiFi
    wifi_connect();
    
    Serial.println("3D Printer Monitor Example");
    Serial.println("Waiting for WiFi connection...");
}

void loop() {
    // Check WiFi status
    wifi_status_t wifi_status = wifi_get_connect_status();
    
    if (wifi_status == WIFI_STATUS_CONNECTED) {
        // Get printer information
        printer.get_printer_info();
        
        // Display printer status
        Serial.println("=== Printer Status ===");
        Serial.printf("Connected: %s\n", printer.unconnected ? "No" : "Yes");
        Serial.printf("Ready: %s\n", printer.unready ? "No" : "Yes");
        Serial.printf("Printing: %s\n", printer.data.printing ? "Yes" : "No");
        Serial.printf("Progress: %d%%\n", printer.data.progress);
        
        // Temperature information
        Serial.printf("Bed: %d°C / %d°C\n", 
                      printer.data.bed_actual, 
                      printer.data.bed_target);
        
        Serial.printf("Nozzle: %d°C / %d°C\n", 
                      printer.data.nozzle_actual, 
                      printer.data.nozzle_target);
        
        // File information
        if (strlen(printer.data.file_path) > 0) {
            Serial.printf("Current file: %s\n", printer.data.file_path);
        }
        
        Serial.println("====================");
        
        // Process HTTP queue
        printer.http_post_loop();
        
    } else if (wifi_status == WIFI_STATUS_CONNECTING) {
        Serial.println("Connecting to WiFi...");
    } else if (wifi_status == WIFI_STATUS_ERROR) {
        Serial.println("WiFi connection error!");
        delay(5000);
        wifi_connect(); // Retry connection
    }
    
    delay(2000); // Update every 2 seconds
}