#pragma once

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <lvgl.h>
#include <kodedot/pin_config.h>
#include <bb_captouch.h>

/**
 * @brief High-level manager that initializes and wires up the display, LVGL, and touch input.
 *
 * Responsibilities:
 * - Bring up the panel using Arduino_GFX
 * - Allocate LVGL draw buffers (prefer PSRAM, fallback to SRAM)
 * - Register LVGL display and input drivers
 * - Provide simple helpers for brightness and touch reading
 */
class DisplayManager {
private:
    // Hardware interfaces
    Arduino_DataBus *bus;
    Arduino_CO5300 *gfx;
    BBCapTouch bbct;
    
    // LVGL display and draw buffers
    lv_display_t *display;
    lv_color_t *buf;
    lv_color_t *buf2;
    uint32_t last_tick_ms;
    
    // Static callbacks required by LVGL v9
    static void disp_flush_callback(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
    static void touchpad_read_callback(lv_indev_t *indev, lv_indev_data_t *data);
    
    // Singleton-like back-reference used by static callbacks
    static DisplayManager* instance;

public:
    DisplayManager();
    ~DisplayManager();
    
    /**
     * @brief Fully initialize display, LVGL, and touch.
     * @return true on success, false otherwise
     */
    bool init();
    
    /**
     * @brief Get the underlying Arduino_GFX panel instance.
     */
    Arduino_CO5300* getGfx() { return gfx; }
    
    /**
     * @brief Get the touch controller instance.
     */
    BBCapTouch* getTouch() { return &bbct; }
    
    /**
     * @brief Pump LVGL timers and tick. Call frequently in loop().
     */
    void update();
    
    /**
     * @brief Set backlight brightness.
     * @param brightness Range 0-255
     */
    void setBrightness(uint8_t brightness);
    
    /**
     * @brief Get current brightness percentage.
     * @return Brightness percentage (0-100)
     */
    uint8_t getBrightnessPercentage();
    
    /**
     * @brief Read current touch coordinates.
     * @param x Output X coordinate
     * @param y Output Y coordinate
     * @return true if there is an active touch
     */
    bool getTouchCoordinates(int16_t &x, int16_t &y);
};


