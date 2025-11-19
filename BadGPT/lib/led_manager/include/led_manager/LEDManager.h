#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <Arduino.h>
#include <NeoPixelBus.h>

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
    // Default brightness (~10%). NeoPixelBus uses GRB @800kHz by default.
    uint8_t brightness = 25; 
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
    
    // Power control
    void turnOff();
    void show();
    
    // Predefined colors
    static const uint32_t COLOR_OFF;
    static const uint32_t COLOR_RECORDING;  // Green
    static const uint32_t COLOR_PROCESSING; // Blue
    static const uint32_t COLOR_ERROR;      // Red
    
private:
    // Use explicit ESP32 bit-bang method to avoid RMT/LightSleep issues
    NeoPixelBus<NeoGrbFeature, NeoEsp32BitBang800KbpsMethod>* strip_ = nullptr;
    LEDConfig config_;
    LEDState currentState_;
    // Local cache of color to reapply on brightness changes
    RgbColor cachedColor_ = RgbColor(0,0,0);
    
    uint32_t getStateColor(LEDState state);
    void updateDisplay();
};

#endif // LED_MANAGER_H
