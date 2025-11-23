#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <Arduino.h>
#ifdef LED_USE_NEOPIXELBUS
#include <NeoPixelBus.h>
#else
#include <Adafruit_NeoPixel.h>
#endif

enum class LEDState {
    Off,
    Recording,    // Green
    Processing,   // Blue  
    Error,        // Red
    Custom        // User-defined color
};

struct LEDConfig {
    uint8_t pin;
    uint16_t count;
    uint8_t brightness = 51; // ~20%
#ifdef LED_USE_NEOPIXELBUS
    uint8_t colorOrder = 0; // Not used with NeoPixelBus
#else
    uint8_t colorOrder = NEO_GRB + NEO_KHZ800;
#endif
};

class LEDManager {
public:
    LEDManager();
    ~LEDManager();
    
    bool init(const LEDConfig& config);
    
    // State-based control
    void setState(LEDState state);
    LEDState getState() const { return currentState_; }
    
    // Direct color control
    void setColor(uint8_t r, uint8_t g, uint8_t b);
    void setPixelColor(uint16_t pixel, uint8_t r, uint8_t g, uint8_t b);
    
    // Brightness control
    void setBrightness(uint8_t brightness);
    uint8_t getBrightness() const;
    
    // Compatibility helpers for NeoPixelBus
    inline void begin() { /* no-op wrapper */ }
    
    // Power control
    void turnOff();
    void show();
    
    // Predefined colors
    static const uint32_t COLOR_OFF;
    static const uint32_t COLOR_RECORDING;  // Green
    static const uint32_t COLOR_PROCESSING; // Blue
    static const uint32_t COLOR_ERROR;      // Red
    
private:
#ifdef LED_USE_NEOPIXELBUS
    using StripType = NeoPixelBus<NeoGrbFeature, NeoEsp32BitBang800KbpsMethod>;
    StripType* pixel_;
#else
    Adafruit_NeoPixel* pixel_;
#endif
    LEDConfig config_;
    LEDState currentState_;
    
    uint32_t getStateColor(LEDState state);
    void updateDisplay();
};

#endif // LED_MANAGER_H
