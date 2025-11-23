#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "moonraker.h"
#include "crowpanel.h"

// Connection parameters
#define HTTP_TIMEOUT 10000  // 10 seconds timeout for normal requests
#define HTTP_LONG_TIMEOUT 60000  // 60 seconds timeout for longer operations like G28
#define MAX_RETRY_ATTEMPTS 3     // Maximum number of retry attempts for failed requests
#define RETRY_DELAY 500          // Delay between retries in milliseconds

void lv_popup_warning(const char * warning, bool clickable) {
    // Silently handle warnings without serial output
}

String MOONRAKER::send_request(const char * type, String path) {
    String ip = moonraker_ip;
    String port = moonraker_port;
    String url = "http://" + ip + ":" + port + path;
    String response = "";
    HTTPClient client;
    int retry_count = 0;
    bool requestSuccessful = false;
    
    // replace all " " space to "%20" for http
    url.replace(" ", "%20");
    
    // Set longer timeout for POST requests (likely to be G-code that takes time)
    int timeout = (strcmp(type, "POST") == 0) ? HTTP_LONG_TIMEOUT : HTTP_TIMEOUT;
    
    // Retry logic for more reliable connections
    while (retry_count < MAX_RETRY_ATTEMPTS && !requestSuccessful) {
        if (retry_count > 0) {
            delay(RETRY_DELAY * retry_count); // Progressive delay
        }
        
        client.begin(url);
        client.setTimeout(timeout);
        
        int code = client.sendRequest(type, "");
        
        // http request success
        if (code > 0) {
            response = client.getString();
            
            if (code >= 200 && code < 300) {
                // Success (2xx response)
                unconnected = false;
                requestSuccessful = true;
            } else if (code == 400) {
                // Handle 400 Bad Request - often contains useful error info
                requestSuccessful = true; // Consider this a "successful" request in terms of communication
                unconnected = false;
                
                if (!response.isEmpty()) {
                    DynamicJsonDocument json_parse(4096);
                    DeserializationError error = deserializeJson(json_parse, response);
                    if (!error) {
                        // Check if there's an error message
                        if (json_parse.containsKey("error")) {
                            JsonVariant errorObj = json_parse["error"];
                            if (errorObj.containsKey("message")) {
                                String msg = errorObj["message"].as<String>();
                                
                                msg.remove(0, 41); //  remove header {'error': 'WebRequestError', 'message':
                                msg.remove(msg.length() - 2, 2); // remove tail }
                                msg.replace("\\n", "\n");
                                lv_popup_warning(msg.c_str(), true);
                            }
                        }
                    }
                }
            } else {
                // Other non-2xx responses
                // For some errors, we should retry
                if (code >= 500 || code == 408) { // Server errors or timeout
                    retry_count++;
                } else {
                    // For client errors besides 400, don't retry
                    requestSuccessful = true;
                }
            }
        } else {
            // HTTP request failed
            // Connection-level errors should probably get retried
            retry_count++;
            
            // Only set unconnected flag for GET requests to avoid false negatives during long operations
            if (strcmp(type, "GET") == 0) {
                unconnected = true;
            }
        }
        
        client.end(); // Free resources
    }

    return response;
}

void MOONRAKER::http_post_loop(void) {
    if (post_queue.count == 0) return;
    
    String request = post_queue.queue[post_queue.index_r];
    send_request("POST", request);
    
    post_queue.count--;
    post_queue.index_r = (post_queue.index_r + 1) % QUEUE_LEN;
}

bool MOONRAKER::post_to_queue(String path) {
    if (post_queue.count >= QUEUE_LEN) {
        return false;
    }
    post_queue.queue[post_queue.index_w] = path;
    post_queue.index_w = (post_queue.index_w + 1) % QUEUE_LEN;
    post_queue.count++;
    
    return true;
}

bool MOONRAKER::post_gcode_to_queue(String gcode) {
    String path = "/printer/gcode/script?script=" + gcode;
    return post_to_queue(path);
}

void MOONRAKER::get_printer_ready(void) {
    String webhooks = send_request("GET", "/printer/objects/query?webhooks");
    if (!webhooks.isEmpty()) {
        DynamicJsonDocument json_parse(2048);
        DeserializationError error = deserializeJson(json_parse, webhooks);
        
        if (error) {
            unready = true;
            return;
        }
        
        // Check the JSON structure using containsKey
        if (json_parse.containsKey("result")) {
            JsonVariant result = json_parse["result"];
            if (result.containsKey("status")) {
                JsonVariant status = result["status"];
                if (status.containsKey("webhooks")) {
                    JsonVariant webhooks = status["webhooks"];
                    if (webhooks.containsKey("state")) {
                        String state = webhooks["state"].as<String>();
                        bool wasUnready = unready;
                        unready = (state == "ready") ? false : true;
                        
                        if (wasUnready != unready) {
                            if (!unready) {
                                // Printer just became ready, refresh all data
                                get_printer_info();
                                get_crowpanel_status();
                            }
                        }
                        return;
                    }
                }
            }
        }
        
        unready = true;
    } else {
        unready = true;
    }
}

void MOONRAKER::get_printer_info(void) {
    String printer_info = send_request("GET", "/api/printer");
    if (!printer_info.isEmpty()) {
        DynamicJsonDocument json_parse(4096);
        DeserializationError error = deserializeJson(json_parse, printer_info);
        
        if (error) {
            return;
        }
        
        // Check if the JSON structure is what we expect
        if (!json_parse.containsKey("state") || !json_parse.containsKey("temperature")) {
            return;
        }
        
        // Parse flags
        data.pause = false;
        data.printing = false;
        
        // Check flags
        if (json_parse["state"].containsKey("flags")) {
            JsonVariant flags = json_parse["state"]["flags"];
            bool pausing = flags.containsKey("pausing") ? flags["pausing"].as<bool>() : false;
            bool paused = flags.containsKey("paused") ? flags["paused"].as<bool>() : false;
            bool printing = flags.containsKey("printing") ? flags["printing"].as<bool>() : false;
            bool cancelling = flags.containsKey("cancelling") ? flags["cancelling"].as<bool>() : false;
            
            data.pause = pausing || paused;
            data.printing = printing || cancelling || data.pause;
        }
        
        // Parse temperatures
        if (json_parse["temperature"].containsKey("bed")) {
            JsonVariant bed = json_parse["temperature"]["bed"];
            if (bed.containsKey("actual") && bed.containsKey("target")) {
                data.bed_actual = bed["actual"].as<double>() + 0.5f;
                data.bed_target = bed["target"].as<double>() + 0.5f;
            }
        }
        
        // Check if the tool exists and get temperatures
        bool toolFound = false;
        if (json_parse["temperature"].containsKey(moonraker_tool)) {
            JsonVariant tool = json_parse["temperature"][moonraker_tool];
            if (tool.containsKey("actual") && tool.containsKey("target")) {
                toolFound = true;
                data.nozzle_actual = tool["actual"].as<double>() + 0.5f;
                data.nozzle_target = tool["target"].as<double>() + 0.5f;
            }
        }
        
        // If specified tool wasn't found, try tool0 as fallback
        if (!toolFound) {
            if (json_parse["temperature"].containsKey("tool0")) {
                JsonVariant tool0 = json_parse["temperature"]["tool0"];
                if (tool0.containsKey("actual") && tool0.containsKey("target")) {
                    data.nozzle_actual = tool0["actual"].as<double>() + 0.5f;
                    data.nozzle_target = tool0["target"].as<double>() + 0.5f;
                }
            }
        }
    }
}

// only return gcode file name except path
// for example:"SD:/test/123.gcode"
// only return "123.gcode"
const char * path_only_gcode(const char * path)
{
  char * name = strrchr(path, '/');

  if (name != NULL)
    return (name + 1);
  else
    return path;
}

void MOONRAKER::get_progress(void) {
    String display_status = send_request("GET", "/printer/objects/query?virtual_sdcard");
    if (!display_status.isEmpty()) {
        DynamicJsonDocument json_parse(2048);
        DeserializationError error = deserializeJson(json_parse, display_status);
        
        if (error) {
            return;
        }
        
        // Check the JSON structure
        if (!json_parse.containsKey("result")) {
            return;
        }
        
        JsonVariant result = json_parse["result"];
        if (!result.containsKey("status")) {
            return;
        }
        
        JsonVariant status = result["status"];
        if (!status.containsKey("virtual_sdcard")) {
            return;
        }
        
        JsonVariant virtual_sdcard = status["virtual_sdcard"];
        
        // Get progress percentage
        if (virtual_sdcard.containsKey("progress")) {
            double progress = virtual_sdcard["progress"].as<double>();
            data.progress = (uint8_t)(progress * 100 + 0.5f);
        }
        
        // Get file path
        if (virtual_sdcard.containsKey("file_path")) {
            const char* file_path = virtual_sdcard["file_path"].as<const char*>();
            strlcpy(data.file_path, path_only_gcode(file_path), sizeof(data.file_path) - 1);
            data.file_path[sizeof(data.file_path) - 1] = 0;
        }
    }
}

void MOONRAKER::get_crowpanel_status(void) {
    String crowpanel_status = send_request("GET", "/printer/objects/query?gcode_macro%20_CROWPANEL_STATUS");
    if (!crowpanel_status.isEmpty()) {
        DynamicJsonDocument json_parse(2048);
        DeserializationError error = deserializeJson(json_parse, crowpanel_status);
        
        if (error) {
            return;
        }
        
        // Check if the CROWPANEL_STATUS macro exists
        if (!json_parse.containsKey("result")) {
            return;
        }
        
        JsonVariant result = json_parse["result"];
        if (!result.containsKey("status")) {
            return;
        }
        
        JsonVariant status = result["status"];
        if (!status.containsKey("gcode_macro _CROWPANEL_STATUS")) {
            // This is normal if the macro isn't defined in Klipper config
            return;
        }
        
        JsonVariant crowpanel = status["gcode_macro _CROWPANEL_STATUS"];
        
        // Parse the status values - check each field individually
        data.homing = crowpanel.containsKey("homing") ? crowpanel["homing"].as<bool>() : false;
        data.probing = crowpanel.containsKey("probing") ? crowpanel["probing"].as<bool>() : false;
        data.qgling = crowpanel.containsKey("qgling") ? crowpanel["qgling"].as<bool>() : false;
        data.heating_nozzle = crowpanel.containsKey("heating_nozzle") ? crowpanel["heating_nozzle"].as<bool>() : false;
        data.heating_bed = crowpanel.containsKey("heating_bed") ? crowpanel["heating_bed"].as<bool>() : false;
    }
}

void MOONRAKER::http_get_loop(void) {
    static unsigned long lastFullRefresh = 0;
    unsigned long currentMillis = millis();
    
    // Set a lock to prevent data being read while we're updating it
    data_unlock = false;
    
    // Always check if the printer is ready first
    get_printer_ready();
    
    if (!unready) {
        // Full refresh every 5 seconds or when printer just became ready
        bool doFullRefresh = (currentMillis - lastFullRefresh) > 5000;
        
        if (doFullRefresh) {
            lastFullRefresh = currentMillis;
            
            // get_crowpanel_status() must be before get_printer_info()
            // to ensure correct status flags
            get_crowpanel_status();
            get_printer_info();
        }
        
        // Always check progress if printing
        if (data.printing) {
            get_progress();
        }
    }
    
    // Release the lock
    data_unlock = true;
}

MOONRAKER moonraker;

void moonraker_post_task(void * parameter) {
    for(;;) {
        if (wifi_get_connect_status() == WIFI_STATUS_CONNECTED) {
            moonraker.http_post_loop();
        }
        delay(500);
    }
}

void moonraker_task(void * parameter) {
    // Create the POST processing task
    xTaskCreate(moonraker_post_task, "moonraker post",
        4096,  // Stack size (bytes)
        NULL,  // Parameter to pass
        8,     // Task priority
        NULL   // Task handle
    );

    // Allow time for WiFi to connect
    delay(2000);
    
    for(;;) {
        if (wifi_get_connect_status() == WIFI_STATUS_CONNECTED) {
            moonraker.http_get_loop();
        }
        delay(200);
    }
}

void moonraker_setup() {
    // Copy the configuration from crowpanel_config
    strcpy(moonraker.moonraker_ip, crowpanel_config.moonraker_ip);
    strcpy(moonraker.moonraker_port, crowpanel_config.moonraker_port);
    strcpy(moonraker.moonraker_tool, crowpanel_config.moonraker_tool);
    
    // Initialize various flags
    moonraker.unready = true;
    moonraker.unconnected = true;
    moonraker.data_unlock = true;
    
    // Initialize the queue
    moonraker.post_queue.count = 0;
    moonraker.post_queue.index_r = 0;
    moonraker.post_queue.index_w = 0;
    
    // Initialize data
    memset(&moonraker.data, 0, sizeof(moonraker.data));
} 