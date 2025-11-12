#include <kodedot/display_manager.h>
#include <Preferences.h>

// Forward declarations for internal helpers
extern "C" void __wrap_esp_ota_mark_app_valid_cancel_rollback(void);
void init_nvs();
void lvgl_port_rounder_callback(lv_display_t *disp, lv_area_t *area);
static void display_rounder_event_cb(lv_event_t *e);

// Static instance used by LVGL callbacks
DisplayManager* DisplayManager::instance = nullptr;

// --- Internal system utilities ---
extern "C" void __wrap_esp_ota_mark_app_valid_cancel_rollback(void) {
    // no-op: always disables OTA validation
}

static Preferences prefs;

void init_nvs() {
    // Initialize NVS namespace for application storage
    prefs.begin("kode_storage", false);
}

// --- LVGL helper ---
void lvgl_port_rounder_callback(lv_display_t *disp, lv_area_t *area) {
    // Efficiently align area to even/odd boundaries as required by the panel
    area->x1 &= ~1;  // round down to even
    area->y1 &= ~1;
    area->x2 = (area->x2 & ~1) + 1;  // round up to odd
    area->y2 = (area->y2 & ~1) + 1;
}

static void display_rounder_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_INVALIDATE_AREA) {
        lv_area_t *area = lv_event_get_invalidated_area(e);
        if (area) {
            area->x1 &= ~1;
            area->y1 &= ~1;
            area->x2 = (area->x2 & ~1) + 1;
            area->y2 = (area->y2 & ~1) + 1;
        }
    }
}

DisplayManager::DisplayManager() : bus(nullptr), gfx(nullptr), display(nullptr), buf(nullptr), buf2(nullptr), last_tick_ms(0) {
    instance = this;
}

DisplayManager::~DisplayManager() {
    if (buf) free(buf);
    if (buf2) free(buf2);
    if (gfx) {
        delete gfx;
    }
    if (bus) {
        delete bus;
    }
    instance = nullptr;
}

bool DisplayManager::init() {
    Serial.println("Bringing up display subsystem...");
    
    // Initialize NVS (preferences)
    init_nvs();
    
    // Power on the panel if an enable pin is provided
    if (LCD_EN >= 0) {
        pinMode(LCD_EN, OUTPUT);
        digitalWrite(LCD_EN, HIGH);
    }

    bus = new Arduino_ESP32QSPI(
        LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3
    );
    
    gfx = new Arduino_CO5300(
        bus, LCD_RST, 0, false, LCD_WIDTH, LCD_HEIGHT,
        22, 0, 0, 0
    );

    if (!gfx->begin()) {
        Serial.println("Error: failed to initialize panel");
        return false;
    }
    
    gfx->setRotation(0);
    
    // Load brightness percentage (0-100) from NVS and apply to panel (0-255)
    // This ensures the display always starts with the user's preferred brightness level
    // Uses the same NVS key as EEPROMManager: "brightness"
    {
        uint8_t saved_pct = prefs.getUChar("brightness", 50); // Default to 50% if no value stored (same as EEPROMManager)
        
        // Validate brightness percentage is within valid range (10-100) - minimum 10% to prevent dark screen
        if (saved_pct < 10) saved_pct = 10;
        if (saved_pct > 100) saved_pct = 100;
        
        // Convert percentage (10-100) to hardware brightness value (0-255) with proper rounding
        uint8_t hardware_brightness = (uint8_t)(((uint16_t)saved_pct * 255 + 50) / 100);
        
        // Apply brightness to the display hardware
        gfx->setBrightness(hardware_brightness);
        
        Serial.printf("[Display] Brightness automatically configured: %u%% (%u/255) from NVS storage\n", 
                     (unsigned)saved_pct, (unsigned)hardware_brightness);
    }
    
    gfx->fillScreen(BLACK);
    Serial.println("Panel initialized");

    // Initialize LVGL core
    lv_init();
    last_tick_ms = millis();

    // Allocate draw buffers (prefer PSRAM; fallback to internal SRAM)
    size_t draw_buf_pixels = (size_t)LCD_WIDTH * (size_t)LCD_DRAW_BUFF_HEIGHT;
    size_t draw_buf_bytes = draw_buf_pixels * sizeof(lv_color_t);
    Serial.printf("Requesting LVGL draw buffer: %u bytes\n", (unsigned)draw_buf_bytes);

    buf = (lv_color_t*)heap_caps_malloc(draw_buf_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buf) {
        // Fallback to internal SRAM with reduced height window
        size_t fallback_height = 100; // small window to avoid exhausting SRAM
        draw_buf_pixels = (size_t)LCD_WIDTH * fallback_height;
        draw_buf_bytes = draw_buf_pixels * sizeof(lv_color_t);
        Serial.printf("PSRAM not available, using SRAM fallback: %u bytes\n", (unsigned)draw_buf_bytes);
        buf = (lv_color_t*)heap_caps_malloc(draw_buf_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!buf) {
            Serial.println("Error: unable to allocate draw buffer in SRAM");
            return false;
        }
    }

    // Try double buffering in PSRAM if there is room
    buf2 = (lv_color_t*)heap_caps_malloc(draw_buf_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buf2) {
        // If not enough memory, keep single buffer
        buf2 = NULL;
    }

    // Create LVGL display and configure rendering
    display = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(display, disp_flush_callback);
    // v9: rounder se implementa como event callback sobre INVALIDATE_AREA
    lv_display_add_event_cb(display, display_rounder_event_cb, LV_EVENT_INVALIDATE_AREA, nullptr);
    // Provide draw buffers (bytes)
    lv_display_set_buffers(
        display,
        buf,
        buf2,
        (uint32_t)draw_buf_bytes,
        LV_DISPLAY_RENDER_MODE_PARTIAL
    );
    Serial.println("LVGL initialized");

    // Initialize capacitive touch
    if(bbct.init(TOUCH_I2C_SDA, TOUCH_I2C_SCL, TOUCH_RST, TOUCH_INT) != CT_SUCCESS) {
        Serial.println("Error: failed to initialize touch");
        return false;
    } else {
        Serial.println("Touch initialized");
    }

    // Create LVGL input device (touch)
    {
        lv_indev_t *indev = lv_indev_create();
        lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
        lv_indev_set_read_cb(indev, touchpad_read_callback);
        (void)indev;
    }

    Serial.println("Display subsystem ready");
    return true;
}

void DisplayManager::update() {
    // Advance LVGL tick with real delta
    uint32_t now = millis();
    uint32_t delta = now - last_tick_ms;
    last_tick_ms = now;
    lv_tick_inc(delta);
    lv_timer_handler();
}

void DisplayManager::setBrightness(uint8_t brightness) {
    if (gfx) {
        gfx->setBrightness(brightness);
    }
    // Convert hardware brightness (0-255) to percentage (10-100) and persist in NVS using the same key as EEPROMManager
    uint8_t pct = (uint8_t)(((uint16_t)brightness * 100 + 127) / 255); // round to nearest percentage
    if (pct < 10) pct = 10; // Minimum 10% to prevent dark screen
    if (pct > 100) pct = 100;
    prefs.putUChar("brightness", pct);
    
    Serial.printf("[Display] Brightness updated: %u%% (%u/255) saved to NVS\n", 
                 (unsigned)pct, (unsigned)brightness);
}

uint8_t DisplayManager::getBrightnessPercentage() {
    uint8_t saved_pct = prefs.getUChar("brightness", 50); // Default to 50% if no value stored (same as EEPROMManager)
    if (saved_pct < 10) saved_pct = 10; // Minimum 10%
    if (saved_pct > 100) saved_pct = 100; // Validate range
    return saved_pct;
}

bool DisplayManager::getTouchCoordinates(int16_t &x, int16_t &y) {
    TOUCHINFO ti;
    if(bbct.getSamples(&ti) && ti.count > 0) {
        x = ti.x[0];
        y = ti.y[0];
        return true;
    }
    return false;
}

// LVGL display flush callback
void DisplayManager::disp_flush_callback(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    if (!instance || !instance->gfx) return;
    
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    instance->gfx->startWrite();
    instance->gfx->writeAddrWindow(area->x1, area->y1, w, h);
    instance->gfx->writePixels((uint16_t *)px_map, w * h);
    instance->gfx->endWrite();

    lv_display_flush_ready(disp);
}

// LVGL touch read callback
void DisplayManager::touchpad_read_callback(lv_indev_t *indev, lv_indev_data_t *data) {
    if (!instance) return;
    
    TOUCHINFO ti;
    if(instance->bbct.getSamples(&ti) && ti.count > 0) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = ti.x[0];
        data->point.y = ti.y[0];
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}


