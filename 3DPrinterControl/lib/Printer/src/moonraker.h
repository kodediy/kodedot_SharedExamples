#ifndef MOONRAKER_H
#define MOONRAKER_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "crowpanel.h"

#define QUEUE_LEN 20

class MOONRAKER {
public:
    struct {
        bool pause;
        bool printing;
        bool homing;
        bool probing;
        bool qgling;
        bool heating_nozzle;
        bool heating_bed;
        int16_t bed_actual;
        int16_t bed_target;
        int16_t nozzle_actual;
        int16_t nozzle_target;
        uint8_t progress;
        char file_path[32];
    } data;

    struct {
        String queue[QUEUE_LEN];
        uint8_t count;
        uint8_t index_r;
        uint8_t index_w;
    } post_queue;

    // Moonraker configuration
    char moonraker_ip[64];
    char moonraker_port[8];
    char moonraker_tool[8];

    bool unconnected;
    bool unready;
    bool data_unlock;

    String send_request(const char * type, String path);
    void http_post_loop(void);
    bool post_to_queue(String path);
    bool post_gcode_to_queue(String gcode);
    void get_printer_ready(void);
    void get_printer_info(void);
    void get_progress(void);
    void get_crowpanel_status(void);
    void http_get_loop(void);
};

extern MOONRAKER moonraker;

void moonraker_setup(void);
void moonraker_task(void *parameter);

#endif 