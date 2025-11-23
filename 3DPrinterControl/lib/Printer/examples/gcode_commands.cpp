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
    strcpy(crowpanel_config.sta_ssid, "YourWiFiName");
    strcpy(crowpanel_config.sta_pwd, "YourWiFiPassword");
    strcpy(crowpanel_config.moonraker_ip, "192.168.1.100");
    strcpy(crowpanel_config.moonraker_port, "7125");
    
    // Configure printer connection
    strcpy(printer.moonraker_ip, crowpanel_config.moonraker_ip);
    strcpy(printer.moonraker_port, crowpanel_config.moonraker_port);
    strcpy(printer.moonraker_tool, crowpanel_config.moonraker_tool);
    
    // Connect to WiFi
    wifi_connect();
    
    Serial.println("G-code Command Example");
    Serial.println("Commands available:");
    Serial.println("  h - Home all axes");
    Serial.println("  t - Set nozzle temperature to 200°C");
    Serial.println("  b - Set bed temperature to 60°C");
    Serial.println("  p - Pause print");
    Serial.println("  r - Resume print");
    Serial.println("  s - Get printer status");
}

void loop() {
    // Check WiFi status
    wifi_status_t wifi_status = wifi_get_connect_status();
    
    if (wifi_status == WIFI_STATUS_CONNECTED) {
        // Process HTTP queue
        printer.http_post_loop();
        
        // Check for serial commands
        if (Serial.available()) {
            char command = Serial.read();
            
            switch (command) {
                case 'h':
                case 'H':
                    Serial.println("Sending: Home all axes");
                    printer.post_gcode_to_queue("G28");
                    break;
                    
                case 't':
                case 'T':
                    Serial.println("Sending: Set nozzle temperature to 200°C");
                    printer.post_gcode_to_queue("M104 S200");
                    break;
                    
                case 'b':
                case 'B':
                    Serial.println("Sending: Set bed temperature to 60°C");
                    printer.post_gcode_to_queue("M140 S60");
                    break;
                    
                case 'p':
                case 'P':
                    Serial.println("Sending: Pause print");
                    printer.post_to_queue("/printer/print/pause");
                    break;
                    
                case 'r':
                case 'R':
                    Serial.println("Sending: Resume print");
                    printer.post_to_queue("/printer/print/resume");
                    break;
                    
                case 's':
                case 'S':
                    Serial.println("Getting printer status...");
                    printer.get_printer_info();
                    
                    Serial.println("=== Current Status ===");
                    Serial.printf("Printing: %s\n", printer.data.printing ? "Yes" : "No");
                    Serial.printf("Paused: %s\n", printer.data.pause ? "Yes" : "No");
                    Serial.printf("Bed: %d°C / %d°C\n", 
                                  printer.data.bed_actual, 
                                  printer.data.bed_target);
                    Serial.printf("Nozzle: %d°C / %d°C\n", 
                                  printer.data.nozzle_actual, 
                                  printer.data.nozzle_target);
                    Serial.println("=====================");
                    break;
                    
                default:
                    Serial.println("Unknown command. Available: h, t, b, p, r, s");
                    break;
            }
        }
        
    } else if (wifi_status == WIFI_STATUS_CONNECTING) {
        Serial.println("Connecting to WiFi...");
        delay(1000);
    } else if (wifi_status == WIFI_STATUS_ERROR) {
        Serial.println("WiFi connection error!");
        delay(5000);
        wifi_connect();
    }
    
    delay(100);
}