#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <Arduino.h>
#include <lvgl.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

enum class UIState {
    Connecting,
    Ready,
    Recording,
    Processing,
    ShowingResponse,
    Error
};

struct UIMessage {
    char text[256];
    enum Type {
        STATUS,
        RESPONSE,
        STATE_CHANGE
    } type;
    UIState state; // Used only for STATE_CHANGE type
};

class UIManager {
public:
    UIManager();
    ~UIManager();
    
    bool init();
    void update(); // Call from main loop
    
    // Message posting (thread-safe)
    void postStatus(const char* text);
    void postResponse(const char* text);
    void postStateChange(UIState state);
    
    // Direct setters (main thread only)
    void setStatus(const char* text);
    void setResponse(const char* text);
    void setState(UIState state);
    
    // Typewriter animation control
    void setTypewriterSpeed(uint32_t delayMs); // Set delay between characters
    
    // Bottom USB connection status
    void setUSBConnectionStatus(bool connected);
    
    // Touch event handling
    bool isTouchPressed();
    
private:
    // UI elements - Eyes
    lv_obj_t* leftEye_;
    lv_obj_t* rightEye_;
    lv_obj_t* leftEyelid_;
    lv_obj_t* rightEyelid_;
    
    // UI elements - Text display
    lv_obj_t* statusLabel_;
    lv_obj_t* responseTextArea_;
    lv_obj_t* usbStatusBottom_;
    // UI elements - GPT logo (top-right)
    lv_obj_t* gptLogo_;
    
    // Display mode
    bool showingEyes_;
    
    // Eye animation
    lv_timer_t* eyeAnimationTimer_;
    lv_timer_t* blinkTimer_;
    // Both eyes move together maintaining horizontal distance
    int32_t eyesCenterTargetX_;  // Center point of both eyes
    int32_t eyesCenterTargetY_;  // Center point of both eyes
    int32_t eyesCenterCurrentX_; // Current center position
    int32_t eyesCenterCurrentY_; // Current center position
    bool isBlinking_;
    uint32_t nextBlinkTime_;
    
    // GPT logo animation
    lv_timer_t* gptLogoTimer_;
    bool gptLogoToggle_;
    
    // Styles
    lv_style_t styleScreenBg_;
    lv_style_t styleText_;
    lv_style_t styleTextArea_;
    lv_style_t styleStatus_;
    lv_style_t styleEye_;
    lv_style_t styleEyelid_;
    
    // Message queue for thread-safe communication
    QueueHandle_t messageQueue_;
    
    // Animation timer and state
    UIState currentState_;
    
    // Typewriter animation system
    lv_timer_t* typewriterTimer_;
    String pendingText_;
    size_t currentCharIndex_;
    bool isTypewriting_;
    uint32_t typewriterDelayMs_;
    uint32_t typewriterFinishedTime_;  // Tiempo cuando termina de escribir
    
    // Processing state animation (thinking)
    lv_timer_t* thinkingTimer_;
    uint32_t nextThinkingMoveTime_;
    uint32_t nextThinkingBlinkTime_;
    
    // Touch state
    bool lastTouchState_;
    
    // Colors
    static const lv_color_t KODE_BG_DARK;
    static const lv_color_t KODE_TEXT_LIGHT;
    
    // Internal methods
    void setupStyles();
    void createUI();
    void createEyes();
    void createTextDisplay();
    void createGPTLogo();
    void processMessages();
    void updateStatusBadge(UIState state);
    void centerResponseText();
    void showEyes(bool show);
    void setGPTLogoVisible(bool show);
    void startGPTLogoAnimation(uint32_t periodMs = 180);
    void stopGPTLogoAnimation();
    
    // Eye animation methods
    void startEyeAnimation();
    void stopEyeAnimation();
    static void eyeAnimationTimerCallback(lv_timer_t* timer);
    static void blinkTimerCallback(lv_timer_t* timer);
    void updateEyePositions();
    void performBlink();
    void setNewEyeTarget();
    void updateEyeColor(UIState state);
    void setEyePosition(int32_t leftX, int32_t leftY, int32_t rightX, int32_t rightY);
    
    // Typewriter animation methods
    void startTypewriterAnimation(const char* text);
    void stopTypewriterAnimation();
    static void typewriterTimerCallback(lv_timer_t* timer);
    void updateTypewriterText();
    
    // Helper methods
    static lv_color_t getStateColor(UIState state);
    static const char* getStateLabel(UIState state);
};

// External font references
extern const lv_font_t Inter_30;

#endif // UI_MANAGER_H
