# Printer Library

A comprehensive 3D printer management library for ESP32-based CrowPanel devices, providing seamless integration with Moonraker API for printer control and monitoring.

## Features

- **WiFi Configuration**: Easy setup for both Station (STA) and Access Point (AP) modes
- **Moonraker Integration**: Full API communication with Klipper/Moonraker systems
- **Real-time Monitoring**: Temperature, progress, and status tracking
- **G-code Execution**: Send commands and monitor execution queue
- **LVGL Interface**: Native integration with LVGL for GUI applications

## Installation

This library is designed to work with PlatformIO and Arduino IDE. It requires the following dependencies:

- ArduinoJson
- WiFi (ESP32)
- HTTPClient (ESP32)
- lvgl

## Configuration

### WiFi Setup

```cpp
#include "crowpanel.h"

void setup() {
    crowpanel_init();
    
    // Configure your WiFi credentials
    strcpy(crowpanel_config.sta_ssid, "YourWiFiName");
    strcpy(crowpanel_config.sta_pwd, "YourWiFiPassword");
    strcpy(crowpanel_config.moonraker_ip, "192.168.1.100");
    strcpy(crowpanel_config.moonraker_port, "7125");
}
```

### Moonraker Integration

```cpp
#include "moonraker.h"

MOONRAKER printer;

void setup() {
    // Initialize printer connection
    strcpy(printer.moonraker_ip, "192.168.1.100");
    strcpy(printer.moonraker_port, "7125");
    strcpy(printer.moonraker_tool, "tool0");
}

void loop() {
    printer.get_printer_info();
    
    // Access printer data
    Serial.printf("Bed Temp: %d°C/%d°C\n", 
                  printer.data.bed_actual, 
                  printer.data.bed_target);
    
    Serial.printf("Nozzle Temp: %d°C/%d°C\n", 
                  printer.data.nozzle_actual, 
                  printer.data.nozzle_target);
    
    Serial.printf("Print Progress: %d%%\n", printer.data.progress);
}
```

## API Reference

### CrowPanel Functions

- `crowpanel_init()`: Initialize default configuration
- `wifi_connect()`: Connect to configured WiFi network
- `wifi_get_connect_status()`: Get current WiFi connection status
- `wifi_task(void *parameter)`: WiFi management task for FreeRTOS

### Moonraker Class Methods

- `get_printer_info()`: Retrieve current printer status
- `get_printer_ready()`: Check if printer is ready
- `post_gcode_to_queue(String gcode)`: Send G-code command
- `send_request(const char* type, String path)`: Send HTTP request to Moonraker

## License

This library is part of the Kode ecosystem and follows the same licensing terms.

## Contributing

Please submit issues and feature requests through the project repository.