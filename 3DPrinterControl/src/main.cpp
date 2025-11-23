/*
 * Base Project for Kode Dot (ESP32-S3)
 * ------------------------------------------------------------
 * Purpose
 *  - Provide a clean, production-ready starting point that wires up
 *    all the on-board peripherals with clear, English documentation.
 *  - Keep implementations in this file for approachability; advanced
 *    board abstractions live under `lib/kodedot_bsp/`.
 *
 * Features covered
 *  - Display + LVGL v9 UI
 *  - Touch + IO Expander buttons
 *  - Addressable LED (WS2812B) with robust RMT init
 *  - SD card over SD_MMC (1-bit)
 *  - IMU (LSM6DSOX) + Magnetometer (LIS2MDL)
 *  - RTC (MAX31329)
 *  - PMIC / Charger (BQ25896)
 *  - Fuel Gauge (BQ27220)
 *
 * Guidance
 *  - All sections are documented with why/what they do and how to change
 *    common parameters.
 *  - Prefer short, frequent updates (1–60s) and avoid long blocking calls.
 *  - Serial prints are informative but minimal; adjust verbosity as needed.
 */
#include <Arduino.h>
#include <lvgl.h>
#include <kodedot/display_manager.h>
#include <TCA9555.h>
#include <kodedot/pin_config.h>
#include <led_manager/LEDManager.h>
// SD card (SD_MMC, 1-bit)
#include <FS.h>
#include <SD_MMC.h>
// IMU + Magnetometer
#include <Wire.h>
#include <Adafruit_LIS2MDL.h>
#include <Adafruit_LSM6DSOX.h>
#include <esp_pm.h>
// RTC MAX31329
#include <kode_MAX31329.h>
// PMIC BQ25896
#include <PMIC_BQ25896.h>
// Fuel gauge BQ27220
#include <BQ27220.h>
// WiFi manager + Printer integration
#include <wifi_manager_lib/WiFiManager.h>
#include <moonraker.h>
#include <crowpanel.h>
// Brand logo image (generated C array)
extern const lv_image_dsc_t logotipo;
// Klipper banner image asset (generated C array in src/images/Klipper.c)
extern const lv_image_dsc_t Klipper;

// ---- Brand fonts (Inter) ----
// These symbols are provided by the generated font C files under src/fonts/
extern const lv_font_t Inter_20;
extern const lv_font_t Inter_30;
extern const lv_font_t Inter_40;
extern const lv_font_t Inter_50;

// Display manager instance
DisplayManager display;

// UI labels
static lv_obj_t *touch_label;
static lv_obj_t *button_label;
static lv_obj_t *sd_label;
static lv_obj_t *imu_label;
static lv_obj_t *mag_label;
static lv_obj_t *rtc_label;
static lv_obj_t *pmic_label;
static lv_obj_t *gauge_label;

// Printer UI widgets
static lv_obj_t *printer_status_label;
static lv_obj_t *nozzle_temp_label;
static lv_obj_t *bed_temp_label;
static lv_obj_t *progress_bar;
static lv_obj_t *progress_label;
static lv_obj_t *file_label;
static lv_obj_t *file_info_container; // container for file + progress

// IO Expander
static TCA9555 ioexp(IOEXP_I2C_ADDR);

// NeoPixel
static LEDManager led;

// -----------------------------------------------------------------------------
// Update cadences (ms) – tweak to balance responsiveness vs. CPU usage
// -----------------------------------------------------------------------------
static const uint32_t GUI_LOOP_DELAY_MS   = 5;      // loop() delay – target ~200 Hz
static const uint32_t IMU_UPDATE_MS       = 1000;   // IMU + MAG
static const uint32_t RTC_UPDATE_MS       = 1000;   // RTC (1 s)
static const uint32_t PMIC_UPDATE_MS      = 1000;   // Charger/PMIC status
static const uint32_t GAUGE_UPDATE_MS     = 1000;   // Fuel gauge

// -----------------------------------------------------------------------------
// Brand palette (see design guide)
// -----------------------------------------------------------------------------
static const lv_color_t KODE_BG_DARK        = lv_color_hex(0x000000); // background (brand: black)
static const lv_color_t KODE_TEXT_LIGHT     = lv_color_hex(0xFFFAF5); // normal light text
static const lv_color_t KODE_TEXT_MUTED     = lv_color_hex(0x9A948F); // titles / not highlighted
static const lv_color_t KODE_ACCENT         = lv_color_hex(0xFF7F1F); // accent
static const lv_color_t KODE_ACCENT_SECOND  = lv_color_hex(0x7B00FF); // secondary accent

// Global styles
static lv_style_t style_screen_bg;
static lv_style_t style_title;
static lv_style_t style_text;
static lv_style_t style_hint;
static lv_style_t style_accent;

static void setupBrandStyles() {
    lv_style_init(&style_screen_bg);
    lv_style_set_bg_color(&style_screen_bg, KODE_BG_DARK);

    lv_style_init(&style_title);
    lv_style_set_text_color(&style_title, KODE_TEXT_LIGHT);
    lv_style_set_text_font(&style_title, &Inter_50);

    lv_style_init(&style_text);
    lv_style_set_text_color(&style_text, KODE_TEXT_LIGHT);
    lv_style_set_text_font(&style_text, &Inter_20);

    lv_style_init(&style_hint);
    lv_style_set_text_color(&style_hint, KODE_TEXT_LIGHT);
    lv_style_set_text_font(&style_hint, &Inter_20);

    lv_style_init(&style_accent);
    lv_style_set_text_color(&style_accent, KODE_ACCENT);
    lv_style_set_text_font(&style_accent, &Inter_20);
}

// -----------------------------------------------------------------------------
// Forward declarations
// -----------------------------------------------------------------------------
void createUserInterface();
static void createPrinterOnlyUI();
void createFontExamples(lv_obj_t *parent);
void updateTouchDisplay();
void updateButtonDisplay();
void initSdCardDemo();
static const char* sdTypeToText(uint8_t t);
void initImuAndMag();
void updateImuAndMag();
void initRtc();
void updateRtc();
void initPmic();
void updatePmic();
void initGauge();
void updateGauge();

// ---- Moonraker / Printer ----
static MOONRAKER printer; // Moonraker client instance
// Set your printer Moonraker endpoint here (or load from SD if desired)
static const char PRINTER_IP[]   = "192.168.1.136"; // Moonraker host/IP
static const char PRINTER_PORT[] = "7125";          // default Moonraker port
static const char PRINTER_TOOL[] = "tool0";         // default tool head

// Forward decl. of printer UI pieces
static void updatePrinterUI();
static void home_btn_event_cb(lv_event_t *e);
static void qgl_btn_event_cb(lv_event_t *e);
static void pla_btn_event_cb(lv_event_t *e);
static void abs_btn_event_cb(lv_event_t *e);

void setup() {
    // ---- Boot diagnostics ----
    Serial.begin(115200);
    Serial.println("Starting Base Project with LVGL...");

    // Initialize display subsystem
    if (!display.init()) {
        Serial.println("Error: Failed to initialize display");
        while(1) {
            delay(1000);
        }
    }

    // Create the minimal printer-only UI
    setupBrandStyles();
    createPrinterOnlyUI();

    Serial.println("System ready!");

    // ---- WiFi via WiFiManager (loads /wifi.txt from SD) ----
    // File format example:
    // NAME=Home WiFi
    // SSID=my-ssid
    // PASSWORD=my-pass
    // ---
    wifiManager.ensureSDMounted();
    if (wifiManager.loadCredentialsFromSD()) {
        wifiManager.printLoadedNetworks();
        wifiManager.connectToWiFi();
    }

    // ---- Printer / Moonraker configuration ----
    // If you want to use crowpanel defaults, call crowpanel_init() and copy values
    crowpanel_init();
    // Prefer values from crowpanel_config if filled; otherwise fallback to constants
    if (strlen(crowpanel_config.moonraker_ip) > 0 && strcmp(crowpanel_config.moonraker_ip, "****") != 0) {
        strncpy(printer.moonraker_ip, crowpanel_config.moonraker_ip, sizeof(printer.moonraker_ip)-1);
    } else {
        strncpy(printer.moonraker_ip, PRINTER_IP, sizeof(printer.moonraker_ip)-1);
    }
    if (strlen(crowpanel_config.moonraker_port) > 0) {
        strncpy(printer.moonraker_port, crowpanel_config.moonraker_port, sizeof(printer.moonraker_port)-1);
    } else {
        strncpy(printer.moonraker_port, PRINTER_PORT, sizeof(printer.moonraker_port)-1);
    }
    if (strlen(crowpanel_config.moonraker_tool) > 0) {
        strncpy(printer.moonraker_tool, crowpanel_config.moonraker_tool, sizeof(printer.moonraker_tool)-1);
    } else {
        strncpy(printer.moonraker_tool, PRINTER_TOOL, sizeof(printer.moonraker_tool)-1);
    }
}

void loop() {
    // Pump display subsystem (LVGL timers + rendering)
    display.update();
    
    // Update printer panel and process Moonraker queue periodically
    updatePrinterUI();
    
    delay(GUI_LOOP_DELAY_MS);
}

// Create a screen with only printer status and control buttons
static void createPrinterOnlyUI() {
    lv_obj_t * scr = lv_scr_act();
    lv_obj_add_style(scr, &style_screen_bg, 0);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(scr, 8, 0);
    lv_obj_set_style_pad_row(scr, 4, 0);
    lv_obj_set_style_pad_top(scr, 30, 0);  // reducido: era 50, ahora 30 para subir 20px
    lv_obj_set_style_pad_bottom(scr, 40, 0);  // aumentado: era 20, ahora 40

    // === HEADER ROW: Status (left, narrow) + Image (center) ===
    lv_obj_t * headerRow = lv_obj_create(scr);
    lv_obj_remove_style_all(headerRow);
    lv_obj_set_width(headerRow, LV_PCT(100));
    lv_obj_set_height(headerRow, 120);  // aumentado a 120 para que la imagen 80x80 quepa con margen
    lv_obj_set_flex_flow(headerRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(headerRow, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(headerRow, 30, 0);  // aumentado de 20 a 30 para más separación

    // Left side: Status container (narrow, fixed width)
    lv_obj_t * statusContainer = lv_obj_create(headerRow);
    lv_obj_remove_style_all(statusContainer);
    lv_obj_set_width(statusContainer, 160);  // aumentado más para dar más hueco
    lv_obj_set_flex_flow(statusContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(statusContainer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Status label - smaller font
    printer_status_label = lv_label_create(statusContainer);
    lv_obj_set_style_text_color(printer_status_label, KODE_TEXT_LIGHT, 0);
    lv_obj_set_style_text_font(printer_status_label, &Inter_40, 0);
    lv_label_set_text(printer_status_label, "IDLE");

    // Center: Klipper image (80x80 - escala original)
    lv_obj_t * banner = lv_image_create(headerRow);
    lv_image_set_src(banner, &Klipper);
    lv_obj_set_size(banner, 80, 80);

    // === FILE INFO SECTION ===
    file_info_container = lv_obj_create(scr);
    lv_obj_set_width(file_info_container, LV_PCT(96));
    lv_obj_set_style_bg_opa(file_info_container, LV_OPA_20, 0);
    lv_obj_set_style_bg_color(file_info_container, lv_color_hex(0x444444), 0);
    lv_obj_set_style_border_width(file_info_container, 0, 0);
    lv_obj_set_style_pad_all(file_info_container, 6, 0);
    lv_obj_set_style_pad_row(file_info_container, 2, 0);
    lv_obj_set_flex_flow(file_info_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(file_info_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(file_info_container, LV_OBJ_FLAG_HIDDEN); // start hidden

    // File name
    file_label = lv_label_create(file_info_container);
    lv_obj_set_style_text_color(file_label, KODE_TEXT_MUTED, 0);
    lv_obj_set_style_text_font(file_label, &lv_font_montserrat_14, 0);
    lv_label_set_text(file_label, "");

    // Progress bar + percentage
    progress_bar = lv_bar_create(file_info_container);
    lv_obj_set_width(progress_bar, LV_PCT(90));
    lv_obj_set_height(progress_bar, 16);
    lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_color(progress_bar, KODE_ACCENT, LV_PART_INDICATOR);
    lv_bar_set_value(progress_bar, 0, LV_ANIM_OFF);
    lv_obj_add_flag(progress_bar, LV_OBJ_FLAG_HIDDEN);

    progress_label = lv_label_create(file_info_container);
    lv_obj_set_style_text_color(progress_label, KODE_ACCENT, 0);
    lv_obj_set_style_text_font(progress_label, &Inter_20, 0);
    lv_label_set_text(progress_label, "0%");
    lv_obj_add_flag(progress_label, LV_OBJ_FLAG_HIDDEN);

    // === TEMPERATURE SECTION ===
    lv_obj_t * tempContainer = lv_obj_create(scr);
    lv_obj_set_width(tempContainer, LV_PCT(100));
    // Hacer el contenedor más pequeño y sin fondo para que no ocupe más que el texto
    lv_obj_set_style_bg_opa(tempContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(tempContainer, 0, 0);
    lv_obj_set_style_pad_all(tempContainer, 0, 0);
    lv_obj_set_flex_flow(tempContainer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tempContainer, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Nozzle column
    lv_obj_t * nozCol = lv_obj_create(tempContainer);
    lv_obj_remove_style_all(nozCol);
    lv_obj_set_flex_flow(nozCol, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(nozCol, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(nozCol, 0, 0);

    // Título "Nozzle"
    lv_obj_t * nozTitle = lv_label_create(nozCol);
    lv_obj_set_style_text_color(nozTitle, KODE_TEXT_MUTED, 0);
    lv_obj_set_style_text_font(nozTitle, &Inter_30, 0);
    lv_label_set_text(nozTitle, "Nozzle");

    nozzle_temp_label = lv_label_create(nozCol);
    lv_obj_set_style_text_color(nozzle_temp_label, KODE_ACCENT, 0);
    lv_obj_set_style_text_font(nozzle_temp_label, &Inter_50, 0); // Larger font
    lv_label_set_text(nozzle_temp_label, "0");

    // Bed column
    lv_obj_t * bedCol = lv_obj_create(tempContainer);
    lv_obj_remove_style_all(bedCol);
    lv_obj_set_flex_flow(bedCol, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(bedCol, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(bedCol, 0, 0);

    // Título "Bed"
    lv_obj_t * bedTitle = lv_label_create(bedCol);
    lv_obj_set_style_text_color(bedTitle, KODE_TEXT_MUTED, 0);
    lv_obj_set_style_text_font(bedTitle, &Inter_30, 0);
    lv_label_set_text(bedTitle, "Bed");

    bed_temp_label = lv_label_create(bedCol);
    lv_obj_set_style_text_color(bed_temp_label, KODE_ACCENT, 0);
    lv_obj_set_style_text_font(bed_temp_label, &Inter_50, 0); // Larger font
    lv_label_set_text(bed_temp_label, "0");

    // === CONTROL BUTTONS GRID (2x2) ===
    lv_obj_t * controls = lv_obj_create(scr);
    lv_obj_remove_style_all(controls);
    lv_obj_set_width(controls, LV_PCT(100));
    // Use row wrap to create a 2x2 grid con spacing
    lv_obj_set_flex_flow(controls, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(controls, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(controls, 8, 0);  // padding interno
    lv_obj_set_style_pad_column(controls, 8, 0);  // espaciado entre botones horizontalmente
    lv_obj_set_style_pad_row(controls, 8, 0);  // espaciado entre botones verticalmente
    lv_obj_set_flex_grow(controls, 1); // que ocupe todo el espacio sobrante hasta abajo

    auto makeGridBtn = [&](const char* txt, lv_event_cb_t cb, lv_color_t color){
        lv_obj_t * b = lv_btn_create(controls);
        lv_obj_set_width(b, LV_PCT(45));
        lv_obj_set_height(b, LV_PCT(45));
        lv_obj_set_style_radius(b, 30, 0);
        lv_obj_set_style_bg_color(b, color, 0);
        lv_obj_set_style_bg_color(b, lv_color_darken(color, 40), LV_STATE_PRESSED);
        lv_obj_set_style_border_width(b, 0, 0);
        // Transición de color y escala al presionar
        static lv_style_transition_dsc_t trans;
        static const lv_style_prop_t props[] = {LV_STYLE_BG_COLOR, LV_STYLE_TRANSFORM_WIDTH, LV_STYLE_TRANSFORM_HEIGHT, 0};
        lv_style_transition_dsc_init(&trans, props, lv_anim_path_ease_in_out, 150, 0, NULL);
        lv_obj_set_style_transition(b, &trans, 0);
        lv_obj_set_style_transform_width(b, 0, 0);
        lv_obj_set_style_transform_height(b, 0, 0);
        lv_obj_set_style_transform_width(b, 10, LV_STATE_PRESSED); // crece 10px al presionar
        lv_obj_set_style_transform_height(b, 10, LV_STATE_PRESSED);
        lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
        lv_obj_t * l = lv_label_create(b);
        lv_obj_set_style_text_font(l, &Inter_30, 0);
        lv_label_set_text(l, txt);
        lv_obj_center(l);
        return b;
    };

    makeGridBtn("PLA",  pla_btn_event_cb, lv_color_hex(0x00AA00));
    makeGridBtn("ABS",  abs_btn_event_cb, lv_color_hex(0xAA0000));
    makeGridBtn("HOME", home_btn_event_cb, lv_color_hex(0x0066CC));
    makeGridBtn("QGL",  qgl_btn_event_cb, lv_color_hex(0xCC6600));
}
static void updatePrinterUI() {
    static uint32_t last = 0;
    const uint32_t now = millis();
    // Poll every ~1s to keep UI fresh without blocking
    if (now - last < 1000) return;
    last = now;

    // Process any queued POSTs (non-blocking slices)
    printer.http_post_loop();

    // Only fetch info if WiFi is up
    if (WiFi.status() == WL_CONNECTED) {
        printer.get_printer_info();
    }

    // Update labels + log to Serial
    if (printer_status_label) {
        const char *status_text = "IDLE";
        if (printer.data.printing)        status_text = "PRINTING";
        else if (printer.data.homing)     status_text = "HOMING";
        else if (printer.data.probing)    status_text = "PROBING";
        else if (printer.data.qgling)     status_text = "QGL";
        else if (printer.data.heating_nozzle) status_text = "HEATING NOZZLE";
        else if (printer.data.heating_bed)   status_text = "HEATING BED";

        if (WiFi.status() != WL_CONNECTED) {
            lv_label_set_text(printer_status_label, "CONNECTING...");
            Serial.println("[WiFi] Connecting...");
            // Hide progress elements
            if (file_info_container) lv_obj_add_flag(file_info_container, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(progress_bar, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(progress_label, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(file_label, "");
        } else if (printer.unconnected || printer.unready) {
            lv_label_set_text(printer_status_label, "DISCONNECTED");
            Serial.println("[Moonraker] Disconnected or not ready");
            if (file_info_container) lv_obj_add_flag(file_info_container, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(progress_bar, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(progress_label, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(file_label, "");
        } else {
            lv_label_set_text(printer_status_label, status_text);
            // Optional: fetch progress when printing
            if (printer.data.printing) {
                printer.get_progress();
                // Show progress
                if (file_info_container) lv_obj_clear_flag(file_info_container, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(progress_bar, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(progress_label, LV_OBJ_FLAG_HIDDEN);
                lv_bar_set_value(progress_bar, printer.data.progress, LV_ANIM_ON);
                lv_label_set_text_fmt(progress_label, "%u%%", (unsigned)printer.data.progress);
                // Show file name
                if (printer.data.file_path[0]) {
                    lv_label_set_text(file_label, printer.data.file_path);
                }
            } else {
                // Hide progress when not printing
                if (file_info_container) lv_obj_add_flag(file_info_container, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(progress_bar, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(progress_label, LV_OBJ_FLAG_HIDDEN);
                lv_label_set_text(file_label, "");
            }
            // Log current state
            char fileBuf[40];
            if (printer.data.file_path[0]) snprintf(fileBuf, sizeof(fileBuf), "%s", printer.data.file_path);
            else snprintf(fileBuf, sizeof(fileBuf), "-");
            Serial.printf("[Moonraker] %s | Nozzle %d/%d C | Bed %d/%d C | Progress %u%% | File %s\n",
                          status_text,
                          (int)printer.data.nozzle_actual, (int)printer.data.nozzle_target,
                          (int)printer.data.bed_actual,    (int)printer.data.bed_target,
                          (unsigned)printer.data.progress,
                          fileBuf);
        }
    }

    // Update temperature displays with actual and target
    if (nozzle_temp_label) {
        lv_label_set_text_fmt(nozzle_temp_label, "%d", (int)printer.data.nozzle_actual);
    }
    if (bed_temp_label) {
        lv_label_set_text_fmt(bed_temp_label, "%d", (int)printer.data.bed_actual);
    }
}

static void home_btn_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (WiFi.status() == WL_CONNECTED && !printer.unready) {
        Serial.println("[BTN] HOME - Sending G28");
        printer.post_gcode_to_queue("G28");
        if (printer_status_label) lv_label_set_text(printer_status_label, "HOMING...");
    }
}

static void qgl_btn_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (WiFi.status() == WL_CONNECTED && !printer.unready) {
        Serial.println("[BTN] QGL - Sending QUAD_GANTRY_LEVEL");
        printer.post_gcode_to_queue("QUAD_GANTRY_LEVEL");
        if (printer_status_label) lv_label_set_text(printer_status_label, "QGL...");
    }
}

static void pla_btn_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (WiFi.status() == WL_CONNECTED && !printer.unready) {
        Serial.println("[BTN] PLA - Heating Nozzle:220C Bed:60C");
        printer.post_gcode_to_queue("M104 S220 T0");
        printer.post_gcode_to_queue("M140 S60");
        if (printer_status_label) lv_label_set_text(printer_status_label, "PLA HEATING...");
    }
}

static void abs_btn_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (WiFi.status() == WL_CONNECTED && !printer.unready) {
        Serial.println("[BTN] ABS - Heating Nozzle:250C Bed:100C");
        printer.post_gcode_to_queue("M104 S250 T0");
        printer.post_gcode_to_queue("M140 S100");
        if (printer_status_label) lv_label_set_text(printer_status_label, "ABS HEATING...");
    }
}

/**
 * @brief Create the main user interface
 *
 * Minimal UI showing live status for SD, IMU/MAG, RTC, PMIC/Charger.
 * Extend this to add your own screens, themes, and widgets. For complex
 * apps, consider splitting screens into dedicated modules.
 */
void createUserInterface() {
    lv_obj_t * scr = lv_scr_act();
    // Apply brand background style and center content using flex layout
    lv_obj_add_style(scr, &style_screen_bg, 0);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    // Compact padding/row spacing to ensure everything fits vertically
    lv_obj_set_style_pad_all(scr, 6, 0);
    lv_obj_set_style_pad_row(scr, 4, 0);
    // Top padding = 0; we'll space the logo from top using its own margin (20px)
    lv_obj_set_style_pad_top(scr, 0, 0);

    // Logo at the very top, tinted with brand light color (no textual title)
    lv_obj_t * img_logo = lv_image_create(scr);
    lv_image_set_src(img_logo, &logotipo);
    lv_obj_set_style_img_recolor_opa(img_logo, LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_opa(img_logo, LV_OPA_TRANSP, 0);
    lv_obj_set_style_margin_top(img_logo, 40, 0);

    // Content container: fills remaining space and centers its children
    lv_obj_t * content = lv_obj_create(scr);
    lv_obj_set_size(content, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_style_pad_all(content, 6, 0);
    lv_obj_set_style_pad_row(content, 6, 0);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Label for touch coordinates
    touch_label = lv_label_create(content);
    lv_obj_add_style(touch_label, &style_text, 0);
    lv_label_set_text(touch_label, "Touch: (-, -)");
    lv_obj_set_style_text_font(touch_label, &Inter_20, 0);

    // Label for buttons state (expander)
    button_label = lv_label_create(content);
    lv_obj_add_style(button_label, &style_text, 0);
    lv_label_set_text(button_label, "Button: none");
    lv_obj_set_style_text_font(button_label, &Inter_20, 0);

    // Ejemplos de diferentes fuentes
    // Omit font examples in base UI to save space

    // Label for SD status
    sd_label = lv_label_create(content);
    lv_obj_add_style(sd_label, &style_text, 0);
    lv_label_set_text(sd_label, "SD: --");
    lv_obj_set_style_text_font(sd_label, &Inter_20, 0);

    // Label for IMU + MAG
    imu_label = lv_label_create(content);
    lv_obj_add_style(imu_label, &style_text, 0);
    lv_label_set_text(imu_label, "IMU: --");
    lv_obj_set_style_text_font(imu_label, &Inter_20, 0);

    mag_label = lv_label_create(content);
    lv_obj_add_style(mag_label, &style_text, 0);
    lv_label_set_text(mag_label, "MAG: --");
    lv_obj_set_style_text_font(mag_label, &Inter_20, 0);

    // Label for RTC time
    rtc_label = lv_label_create(content);
    lv_obj_add_style(rtc_label, &style_text, 0);
    lv_label_set_text(rtc_label, "RTC: --");
    lv_obj_set_style_text_font(rtc_label, &Inter_20, 0);

    // Label for PMIC status
    pmic_label = lv_label_create(content);
    lv_obj_add_style(pmic_label, &style_text, 0);
    lv_label_set_text(pmic_label, "PMIC: --");
    lv_obj_set_style_text_font(pmic_label, &Inter_20, 0);

    // Optional hint
    lv_obj_t * hint = lv_label_create(content);
    lv_obj_add_style(hint, &style_hint, 0);
    lv_label_set_text(hint, "Hold TOP button to test LED colors");

    // Label for Fuel Gauge
    gauge_label = lv_label_create(content);
    lv_obj_set_style_text_font(gauge_label, &lv_font_montserrat_18, 0);
    lv_label_set_text(gauge_label, "BAT: --");
    lv_obj_set_style_text_color(gauge_label, KODE_TEXT_LIGHT, 0);

    // ---- Printer mini panel ----
    lv_obj_t * printerWrap = lv_obj_create(content);
    lv_obj_set_width(printerWrap, LV_PCT(100));
    lv_obj_set_style_bg_opa(printerWrap, LV_OPA_10, 0);
    lv_obj_set_style_bg_color(printerWrap, lv_color_hex(0x222222), 0);
    lv_obj_set_style_border_width(printerWrap, 0, 0);
    lv_obj_set_style_pad_all(printerWrap, 6, 0);
    lv_obj_set_flex_flow(printerWrap, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(printerWrap, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Status label
    printer_status_label = lv_label_create(printerWrap);
    lv_obj_add_style(printer_status_label, &style_title, 0);
    lv_label_set_text(printer_status_label, "Printer: --");

    // Temperature row
    lv_obj_t * tempsRow = lv_obj_create(printerWrap);
    lv_obj_remove_style_all(tempsRow);
    lv_obj_set_width(tempsRow, LV_PCT(100));
    lv_obj_set_flex_flow(tempsRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(tempsRow, 20, 0);
    lv_obj_set_style_pad_row(tempsRow, 0, 0);

    lv_obj_t * nozCol = lv_obj_create(tempsRow);
    lv_obj_remove_style_all(nozCol);
    lv_obj_set_flex_flow(nozCol, LV_FLEX_FLOW_COLUMN);
    lv_obj_t * nozTitle = lv_label_create(nozCol);
    lv_label_set_text(nozTitle, "Nozzle");
    lv_obj_add_style(nozTitle, &style_hint, 0);
    nozzle_temp_label = lv_label_create(nozCol);
    lv_obj_add_style(nozzle_temp_label, &style_text, 0);
    lv_label_set_text(nozzle_temp_label, "-- °C");

    lv_obj_t * bedCol = lv_obj_create(tempsRow);
    lv_obj_remove_style_all(bedCol);
    lv_obj_set_flex_flow(bedCol, LV_FLEX_FLOW_COLUMN);
    lv_obj_t * bedTitle = lv_label_create(bedCol);
    lv_label_set_text(bedTitle, "Bed");
    lv_obj_add_style(bedTitle, &style_hint, 0);
    bed_temp_label = lv_label_create(bedCol);
    lv_obj_add_style(bed_temp_label, &style_text, 0);
    lv_label_set_text(bed_temp_label, "-- °C");

    // Buttons row
    lv_obj_t * btnRow1 = lv_obj_create(printerWrap);
    lv_obj_remove_style_all(btnRow1);
    lv_obj_set_width(btnRow1, LV_PCT(100));
    lv_obj_set_flex_flow(btnRow1, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(btnRow1, 10, 0);

    auto makeBtn = [&](const char* txt, lv_event_cb_t cb){
        lv_obj_t * b = lv_btn_create(btnRow1);
        lv_obj_set_size(b, 90, 36);
        lv_obj_set_style_radius(b, 16, 0);
        lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
        lv_obj_t * l = lv_label_create(b);
        lv_label_set_text(l, txt);
        lv_obj_center(l);
        return b;
    };

    makeBtn("HOME", home_btn_event_cb);
    makeBtn("QGL",  qgl_btn_event_cb);
    makeBtn("PLA",  pla_btn_event_cb);
    makeBtn("ABS",  abs_btn_event_cb);
}

/**
 * @brief Create sample labels using different font sizes
 */
void createFontExamples(lv_obj_t *parent) {
    // Small font
    lv_obj_t * font_small = lv_label_create(parent);
    lv_obj_set_style_text_font(font_small, &lv_font_montserrat_14, 0);
    lv_label_set_text(font_small, "Montserrat 14px");
    lv_obj_set_style_text_color(font_small, lv_color_hex(0xCCCCCC), 0);
    lv_obj_align(font_small, LV_ALIGN_BOTTOM_LEFT, 20, -80);

    // Medium font
    lv_obj_t * font_medium = lv_label_create(parent);
    lv_obj_set_style_text_font(font_medium, &lv_font_montserrat_18, 0);
    lv_label_set_text(font_medium, "Montserrat 18px");
    lv_obj_set_style_text_color(font_medium, lv_color_hex(0xCCCCCC), 0);
    lv_obj_align(font_medium, LV_ALIGN_BOTTOM_LEFT, 20, -50);
    
    // Large font
    lv_obj_t * font_large = lv_label_create(parent);
    lv_obj_set_style_text_font(font_large, &lv_font_montserrat_42, 0);
    lv_label_set_text(font_large, "42");
    lv_obj_set_style_text_color(font_large, lv_color_hex(0x999999), 0);
    lv_obj_align(font_large, LV_ALIGN_BOTTOM_RIGHT, -30, -30);
}

/**
 * @brief Update the touch coordinates label
 */
void updateTouchDisplay() {
    if (!touch_label) return;
    
    int16_t x, y;
    if (display.getTouchCoordinates(x, y)) {
        lv_label_set_text_fmt(touch_label, "Touch: (%d, %d)", x, y);
    }
}

static inline bool isPressed(uint8_t pinIndex) {
    // Entradas con pull-up externa: activo en LOW
    int v = ioexp.read1(pinIndex);
    return (v != TCA9555_INVALID_READ) && (v == LOW);
}

static inline bool isGpioPressed(int gpio) {
  return digitalRead(gpio) == LOW; // activo en LOW por pull-up externa
}

void updateButtonDisplay() {
    if (!button_label) return;

    const char* status = "none";

  if (isGpioPressed(BUTTON_TOP)) {
      status = "BUTTON_TOP";
  } else if (isPressed(EXPANDER_BUTTON_BOTTOM)) {
        status = "BUTTON_BOTTOM";
    } else if (isPressed(EXPANDER_PAD_TOP)) {
        status = "PAD_TOP";
    } else if (isPressed(EXPANDER_PAD_BOTTOM)) {
        status = "PAD_BOTTOM";
    } else if (isPressed(EXPANDER_PAD_LEFT)) {
        status = "PAD_LEFT";
    } else if (isPressed(EXPANDER_PAD_RIGHT)) {
        status = "PAD_RIGHT";
    }

    // Always update label and LED color based on current state
    lv_label_set_text_fmt(button_label, "Button: %s", status);

    // Hold-to-test: if TOP is held > 700ms, cycle RGB quickly to verify LED
    static bool topWasPressed = false;
    static uint32_t topPressStart = 0;
    static uint8_t testPhase = 0;

    bool topNow = (strcmp(status, "BUTTON_TOP") == 0);
    uint32_t now = millis();
    if (topNow && !topWasPressed) {
        topWasPressed = true;
        topPressStart = now;
        testPhase = 0;
    } else if (!topNow && topWasPressed) {
        topWasPressed = false;
    }

    if (topWasPressed && (now - topPressStart > 700)) {
        // non-blocking cycle every 150ms
        static uint32_t lastStep = 0;
        if (now - lastStep > 150) {
            lastStep = now;
            testPhase = (testPhase + 1) % 3;
            if (testPhase == 0)      led.setPixelColor(0, 255, 0, 0);
            else if (testPhase == 1) led.setPixelColor(0, 0, 255, 0);
            else                     led.setPixelColor(0, 0, 0, 255);
            led.show();
        }
        return;
    }

    // Normal mapping: only TOP lights the LED; all others keep it off
    uint8_t r=0,g=0,b=0; // idle off
    if (strcmp(status, "BUTTON_TOP") == 0) {
        r=255; g=0; b=0;      // red
    }
    static uint8_t lastR=255, lastG=255, lastB=255;
    if (r!=lastR || g!=lastG || b!=lastB) {
        lastR=r; lastG=g; lastB=b;
        led.setPixelColor(0, r, g, b);
        led.show();
    }
}

// Boot LED test removed per request

static const char* sdTypeToText(uint8_t t) {
    switch (t) {
        case CARD_MMC: return "MMC";
        case CARD_SD: return "SDSC";
        case CARD_SDHC: return "SDHC";
        default: return "UNKNOWN";
    }
}

void initSdCardDemo() {
    Serial.println("Mounting SD_MMC (1-bit)...");

    // Assign custom pins from pin_config.h
    if (!SD_MMC.setPins(SD_PIN_CLK, SD_PIN_CMD, SD_PIN_D0)) {
        Serial.println("[SD] setPins failed");
        if (sd_label) {
            lv_label_set_text(sd_label, "SD: setPins failed");
            lv_obj_set_style_text_color(sd_label, KODE_TEXT_LIGHT, 0);
        }
        return;
    }

    // busWidth = 1 for 1-bit mode
    if (!SD_MMC.begin(SD_MOUNT_POINT, 1)) {
        Serial.println("[SD] Card mount failed");
        if (sd_label) {
            lv_label_set_text(sd_label, "SD: mount failed");
            lv_obj_set_style_text_color(sd_label, KODE_TEXT_LIGHT, 0);
        }
        return;
    }

    uint8_t type = SD_MMC.cardType();
    if (type == CARD_NONE) {
        Serial.println("[SD] No card attached");
        if (sd_label) {
            lv_label_set_text(sd_label, "SD: no card");
            lv_obj_set_style_text_color(sd_label, KODE_TEXT_LIGHT, 0);
        }
        return;
    }

    uint64_t sizeMB = SD_MMC.cardSize() / (1024ULL * 1024ULL);
    Serial.printf("[SD] Type=%s Size=%lluMB\n", sdTypeToText(type), sizeMB);

    if (sd_label) {
        char buf[64];
        snprintf(buf, sizeof(buf), "SD: OK (%s %lluMB)", sdTypeToText(type), sizeMB);
        lv_label_set_text(sd_label, buf);
        lv_obj_set_style_text_color(sd_label, KODE_TEXT_LIGHT, 0);
    }

    // Minimal demo: list root directory
    Serial.println("[SD] Listing root /");
    File root = SD_MMC.open("/");
    if (root && root.isDirectory()) {
        File f = root.openNextFile();
        while (f) {
            Serial.printf("  %s %s (%u)\n", f.isDirectory() ? "DIR " : "FILE", f.name(), (unsigned)f.size());
            f = root.openNextFile();
        }
    }
}

// ---- IMU + Magnetometer ----
// Reads basic motion + magnetic field for demos and quick health checks.
// Pins are configured in pin_config.h; wire is initialized once and reused.
static Adafruit_LSM6DSOX imu;
static Adafruit_LIS2MDL mag(12345);
static MAX31329 rtc;
static PMIC_BQ25896 bq;
static BQ27220 gauge;

void initImuAndMag() {
    // Inicializa bus I2C con pines de la placa
    Wire.begin(TOUCH_I2C_SDA, TOUCH_I2C_SCL);

    Serial.println("Init IMU (LSM6DSOX) + MAG (LIS2MDL)...");

    bool imu_ok = imu.begin_I2C();
    if (!imu_ok) {
        Serial.println("[IMU] LSM6DSOX not found");
        if (imu_label) {
            lv_label_set_text(imu_label, "IMU: not found");
            lv_obj_set_style_text_color(imu_label, KODE_TEXT_LIGHT, 0);
        }
    } else {
        Serial.println("[IMU] LSM6DSOX OK");
        if (imu_label) {
            lv_label_set_text(imu_label, "IMU: OK");
            lv_obj_set_style_text_color(imu_label, KODE_TEXT_LIGHT, 0);
        }
    }

    bool mag_ok = mag.begin();
    if (!mag_ok) {
        Serial.println("[MAG] LIS2MDL not found");
        if (mag_label) {
            lv_label_set_text(mag_label, "MAG: not found");
            lv_obj_set_style_text_color(mag_label, KODE_TEXT_LIGHT, 0);
        }
    } else {
        Serial.println("[MAG] LIS2MDL OK");
        if (mag_label) {
            lv_label_set_text(mag_label, "MAG: OK");
            lv_obj_set_style_text_color(mag_label, KODE_TEXT_LIGHT, 0);
        }
    }
}

// ---- RTC MAX31329 ----
static void rtcPrintToSerialAndUi() {
    if (!rtc.readTime()) {
        Serial.println("[RTC] readTime failed");
        if (rtc_label) {
            lv_label_set_text(rtc_label, "RTC: read fail");
            lv_obj_set_style_text_color(rtc_label, KODE_TEXT_LIGHT, 0);
        }
        return;
    }
    char buf[48];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
             rtc.t.year, rtc.t.month, rtc.t.day,
             rtc.t.hour, rtc.t.minute, rtc.t.second);
    Serial.printf("[RTC] %s\n", buf);
    if (rtc_label) {
        lv_label_set_text_fmt(rtc_label, "RTC: %s", buf);
        lv_obj_set_style_text_color(rtc_label, KODE_TEXT_LIGHT, 0);
    }
}

// ---- PMIC BQ25896 ----
// Shows power source (USB/Battery) and charge state (PRE/CARGA/COMPLETA).
// Note: To source 5V on the upper connector, enable OTG (boost) mode using
// setOTG_CONFIG(true). Keep disabled by default.
static void pmicUpdateUi(bool haveUsb, uint8_t chrg_stat) {
    if (!pmic_label) return;
    const char* fuente = haveUsb ? "USB" : "BATERIA";
    const char* estado = "IDLE";
    switch (chrg_stat) {
        case 1: estado = "PRE"; break;           // Pre-charge
        case 2: estado = "CARGA"; break;         // Fast charge (CC/CV)
        case 3: estado = "COMPLETA"; break;      // Done
        default: estado = "IDLE"; break;         // Not charging
    }
    lv_label_set_text_fmt(pmic_label, "Fuente: %s  Estado: %s",
                          fuente, estado);
    lv_obj_set_style_text_color(pmic_label, KODE_TEXT_LIGHT, 0);
}

void initPmic() {
    // Reutiliza Wire(48/47)
    bq.begin();
    delay(200);
    // Habilita conversión continua de ADC (1 Hz) para refrescar medidas
    bq.setCONV_RATE(true);

    // Primera lectura para UI
    auto vstat = bq.get_VBUS_STAT_reg();
    bool haveUsb = vstat.pg_stat;                    // Power Good on VBUS
    pmicUpdateUi(haveUsb, vstat.chrg_stat);

    // To source 5V on the upper connector, enable boost mode:
    // bq.setOTG_CONFIG(true);        // enable OTG/boost (5V out)
    // bq.setPFM_OTG_DIS(false);      // optional: allow/disallow PFM in OTG
}

void updatePmic() {
    static uint32_t last = 0;
    uint32_t now = millis();
    if (now - last < PMIC_UPDATE_MS) return;
    last = now;
    // En modo continuo no es necesario; si se desactiva, usar setCONV_START(true)
    auto vstat = bq.get_VBUS_STAT_reg();
    bool haveUsb = vstat.pg_stat;
    pmicUpdateUi(haveUsb, vstat.chrg_stat);
}

// ---- Fuel Gauge BQ27220 ----
void initGauge() {
    if (!gauge.begin(Wire, 0x55, -1, -1, 400000)) {
        Serial.println("[GAUGE] BQ27220 not found");
        if (gauge_label) {
            lv_label_set_text(gauge_label, "BAT: gauge not found");
            lv_obj_set_style_text_color(gauge_label, KODE_TEXT_LIGHT, 0);
        }
        return;
    }
    Serial.println("[GAUGE] BQ27220 OK");
}

void updateGauge() {
    static uint32_t last = 0;
    uint32_t now = millis();
    if (now - last < 1000) return; // 1 s
    last = now;
    int soc = gauge.readStateOfChargePercent();
    int ma  = gauge.readCurrentMilliamps(); // positive = charging
    if (gauge_label) {
        lv_label_set_text_fmt(gauge_label, "BAT: %d%%  I=%dmA", soc, ma);
        lv_obj_set_style_text_color(gauge_label, KODE_TEXT_LIGHT, 0);
    }
}

void initRtc() {
    // Reutiliza Wire ya en pines 48/47
    rtc.begin();
    // Si necesitas fijar hora inicial, descomenta y ajusta:
    // rtc.t.year=2025; rtc.t.month=1; rtc.t.day=1; rtc.t.hour=0; rtc.t.minute=0; rtc.t.second=0; rtc.t.dayOfWeek=3;
    // rtc.writeTime();
    rtcPrintToSerialAndUi();
}

void updateRtc() {
    static uint32_t last = 0;
    uint32_t now = millis();
    if (now - last < RTC_UPDATE_MS) return;
    last = now;
    rtcPrintToSerialAndUi();
}

void updateImuAndMag() {
    static uint32_t last = 0;
    uint32_t now = millis();
    if (now - last < IMU_UPDATE_MS) return;
    last = now;

    // Read IMU
    sensors_event_t accel, gyro, temp;
    bool haveImu = imu.getEvent(&accel, &gyro, &temp);
    static float yawDeg = 0.0f; // integrated yaw from gyro (deg)
    static uint32_t lastYawMs = 0;

    float rollRad = 0.0f, pitchRad = 0.0f;
    if (haveImu) {
        const float ax = accel.acceleration.x;
        const float ay = accel.acceleration.y;
        const float az = accel.acceleration.z;

        // Roll and Pitch from accelerometer (in radians)
        rollRad  = atan2f(ay, az);
        pitchRad = atan2f(-ax, sqrtf(ay * ay + az * az));

        const float rollDeg  = rollRad * RAD_TO_DEG;
        const float pitchDeg = pitchRad * RAD_TO_DEG;

        // Gyro in deg/s for serial debug
        const float gx_dps = gyro.gyro.x * RAD_TO_DEG;
        const float gy_dps = gyro.gyro.y * RAD_TO_DEG;
        const float gz_dps = gyro.gyro.z * RAD_TO_DEG;

        // Integrate yaw from gyro Z (deg)
        if (lastYawMs == 0) lastYawMs = now;
        float dtSec = (now - lastYawMs) / 1000.0f;
        lastYawMs = now;
        yawDeg += gz_dps * dtSec;
        // Normalize to [0,360)
        while (yawDeg < 0.0f) yawDeg += 360.0f;
        while (yawDeg >= 360.0f) yawDeg -= 360.0f;

        Serial.printf("IMU  : roll=%.1f pitch=%.1f yaw=%.1f  | gyro(dps)=(%.1f, %.1f, %.1f)  temp=%.1f C\n",
                      rollDeg, pitchDeg, yawDeg, gx_dps, gy_dps, gz_dps, temp.temperature);
        if (imu_label) {
            char buf[96];
            snprintf(buf, sizeof(buf), "IMU roll=%.1f pitch=%.1f yaw=%.1f", rollDeg, pitchDeg, yawDeg);
            lv_label_set_text(imu_label, buf);
        }
    }

    // Read Magnetometer and show raw values
    sensors_event_t mev;
    if (mag.getEvent(&mev)) {
        const float mx = mev.magnetic.x;
        const float my = mev.magnetic.y;
        const float mz = mev.magnetic.z;
        Serial.printf("MAG  : raw(uT)=(%.1f, %.1f, %.1f)\n", mx, my, mz);
        if (mag_label) {
            char buf[96];
            snprintf(buf, sizeof(buf), "MAG x=%.1f y=%.1f z=%.1f uT", mx, my, mz);
            lv_label_set_text(mag_label, buf);
        }
    }
}