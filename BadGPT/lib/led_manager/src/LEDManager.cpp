#include "led_manager/LEDManager.h"

// Static color definitions
const uint32_t LEDManager::COLOR_OFF = 0x000000;
const uint32_t LEDManager::COLOR_RECORDING = 0x00FF00;    // Green
const uint32_t LEDManager::COLOR_PROCESSING = 0x1976D2;   // Blue (#1976D2)
const uint32_t LEDManager::COLOR_ERROR = 0xFF0000;        // Red

LEDManager::LEDManager() 
    : strip_(nullptr)
    , currentState_(LEDState::Off) {
}

LEDManager::~LEDManager() {
    if (strip_) {
        delete strip_;
        strip_ = nullptr;
    }
}

bool LEDManager::init(const LEDConfig& config) {
    config_ = config;
    
    Serial.printf("[LEDManager] Attempting to initialize %u NeoPixels on pin %u\n", 
                  config_.count, config_.pin);
    
    // Create NeoPixelBus with ESP32 bit-bang method (no RMT at all)
    strip_ = new NeoPixelBus<NeoGrbFeature, NeoEsp32BitBang800KbpsMethod>(config_.count, config_.pin);
    if (!strip_) {
        Serial.println("[LEDManager] Failed to create NeoPixelBus");
        return false;
    }
    strip_->Begin();
    strip_->Show(); // clear
    setBrightness(config_.brightness);
    turnOff();
    Serial.println("[LEDManager] NeoPixel initialization successful (NeoPixelBus)");
    
    Serial.printf("[LEDManager] Initialized %u NeoPixels on pin %u\n", 
                  config_.count, config_.pin);
    return true;  // Always return true to avoid blocking the system
}

void LEDManager::setState(LEDState state) {
    if (currentState_ != state) {
        currentState_ = state;
        updateDisplay();
    }
}

void LEDManager::setColor(uint8_t r, uint8_t g, uint8_t b) {
    if (!strip_) return;  // Safety check
    
    currentState_ = LEDState::Custom;
    cachedColor_ = RgbColor(r, g, b);
    for (uint16_t i = 0; i < config_.count; i++) {
        strip_->SetPixelColor(i, RgbColor(r, g, b).Dim(config_.brightness));
    }
    show();
}

void LEDManager::setPixelColor(uint16_t pixel, uint8_t r, uint8_t g, uint8_t b) {
    if (!strip_ || pixel >= config_.count) return;  // Safety check
    strip_->SetPixelColor(pixel, RgbColor(r, g, b).Dim(config_.brightness));
}

void LEDManager::setBrightness(uint8_t brightness) {
    config_.brightness = brightness;
    updateDisplay();
}

uint8_t LEDManager::getBrightness() const {
    return config_.brightness;
}

void LEDManager::turnOff() {
    currentState_ = LEDState::Off;
    if (!strip_) return;
    RgbColor off(0,0,0);
    for (uint16_t i = 0; i < config_.count; i++) strip_->SetPixelColor(i, off);
    strip_->Show();
}

void LEDManager::show() {
    if (!strip_) return;
    strip_->Show();
}

uint32_t LEDManager::getStateColor(LEDState state) {
    switch (state) {
        case LEDState::Recording:
            return COLOR_RECORDING;
        case LEDState::Processing:
            return COLOR_PROCESSING;
        case LEDState::Error:
            return COLOR_ERROR;
        case LEDState::Off:
        case LEDState::Custom:
        default:
            return COLOR_OFF;
    }
}

void LEDManager::updateDisplay() {
    if (!strip_) return;
    
    uint32_t color = getStateColor(currentState_);
    
    // Extract RGB components
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    
    // Set all pixels to the same color
    cachedColor_ = RgbColor(r, g, b);
    for (uint16_t i = 0; i < config_.count; i++) {
        strip_->SetPixelColor(i, RgbColor(r, g, b).Dim(config_.brightness));
    }
    show();
}
