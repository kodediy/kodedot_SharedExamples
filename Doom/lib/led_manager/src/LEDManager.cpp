#include "led_manager/LEDManager.h"

// Static color definitions
const uint32_t LEDManager::COLOR_OFF = 0x000000;
const uint32_t LEDManager::COLOR_RECORDING = 0x00FF00;    // Green
const uint32_t LEDManager::COLOR_PROCESSING = 0x1976D2;   // Blue (#1976D2)
const uint32_t LEDManager::COLOR_ERROR = 0xFF0000;        // Red

LEDManager::LEDManager() 
    : pixel_(nullptr)
    , currentState_(LEDState::Off) {
}

LEDManager::~LEDManager() {
    if (pixel_) {
        delete pixel_;
    }
}

bool LEDManager::init(const LEDConfig& config) {
    config_ = config;
    
#ifdef LED_USE_NEOPIXELBUS
    // Use NeoPixelBus BitBang method to avoid RMT entirely
    pixel_ = new StripType(config_.count, config_.pin);
    if (!pixel_) {
        Serial.println("[LEDManager] Failed to create NeoPixelBus object");
        return false;
    }
    pixel_->Begin();
#else
    // Initialize Adafruit NeoPixel (RMT-based)
    pixel_ = new Adafruit_NeoPixel(config_.count, config_.pin, config_.colorOrder);
    if (!pixel_) {
        Serial.println("[LEDManager] Failed to create NeoPixel object");
        return false;
    }
    
    // Configure pin and initialize
    pinMode(config_.pin, OUTPUT);
    pixel_->begin();
    pixel_->setBrightness(config_.brightness);
#endif
    
    // Start with LEDs off
    turnOff();
    
    Serial.printf("[LEDManager] Initialized %u NeoPixels on pin %u\n", 
                  config_.count, config_.pin);
    return true;
}

void LEDManager::setState(LEDState state) {
    if (currentState_ != state) {
        currentState_ = state;
        updateDisplay();
    }
}

void LEDManager::setColor(uint8_t r, uint8_t g, uint8_t b) {
    currentState_ = LEDState::Custom;
    for (uint16_t i = 0; i < config_.count; i++) {
#ifdef LED_USE_NEOPIXELBUS
        uint8_t sr = (uint16_t)r * config_.brightness / 255;
        uint8_t sg = (uint16_t)g * config_.brightness / 255;
        uint8_t sb = (uint16_t)b * config_.brightness / 255;
        pixel_->SetPixelColor(i, RgbColor(sr, sg, sb));
#else
        pixel_->setPixelColor(i, pixel_->Color(r, g, b));
#endif
    }
    // show() diferido; será llamado por el lazo principal
}

void LEDManager::setPixelColor(uint16_t pixel, uint8_t r, uint8_t g, uint8_t b) {
    if (pixel < config_.count) {
#ifdef LED_USE_NEOPIXELBUS
        uint8_t sr = (uint16_t)r * config_.brightness / 255;
        uint8_t sg = (uint16_t)g * config_.brightness / 255;
        uint8_t sb = (uint16_t)b * config_.brightness / 255;
        pixel_->SetPixelColor(pixel, RgbColor(sr, sg, sb));
#else
        pixel_->setPixelColor(pixel, pixel_->Color(r, g, b));
#endif
    }
}

void LEDManager::setBrightness(uint8_t brightness) {
    config_.brightness = brightness;
    if (pixel_) {
#ifndef LED_USE_NEOPIXELBUS
        pixel_->setBrightness(brightness);
#endif
        updateDisplay();
    }
}

uint8_t LEDManager::getBrightness() const {
    return config_.brightness;
}

void LEDManager::turnOff() {
    currentState_ = LEDState::Off;
    if (pixel_) {
#ifdef LED_USE_NEOPIXELBUS
        for (uint16_t i = 0; i < config_.count; i++) {
            pixel_->SetPixelColor(i, RgbColor(0, 0, 0));
        }
#else
        pixel_->clear();
#endif
        // show() diferido; será llamado por el lazo principal
    }
}

void LEDManager::show() {
    if (pixel_) {
#ifdef LED_USE_NEOPIXELBUS
        pixel_->Show();
#else
        pixel_->show();
#endif
    }
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
    if (!pixel_) return;
    
    uint32_t color = getStateColor(currentState_);
    
    // Extract RGB components
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    
    // Set all pixels to the same color
    for (uint16_t i = 0; i < config_.count; i++) {
#ifdef LED_USE_NEOPIXELBUS
        uint8_t sr = (uint16_t)r * config_.brightness / 255;
        uint8_t sg = (uint16_t)g * config_.brightness / 255;
        uint8_t sb = (uint16_t)b * config_.brightness / 255;
        pixel_->SetPixelColor(i, RgbColor(sr, sg, sb));
#else
        pixel_->setPixelColor(i, pixel_->Color(r, g, b));
#endif
    }
    // show() diferido; será llamado por el lazo principal
}
