#include "ui_manager/UIManager.h"
#include <cstring>

// External LVGL images for GPT logo
extern "C" {
    extern const lv_image_dsc_t GPT_basic;
    extern const lv_image_dsc_t GPT_basic_turn;
}

// Static color definitions
const lv_color_t UIManager::KODE_BG_DARK = lv_color_hex(0x000000);
const lv_color_t UIManager::KODE_TEXT_LIGHT = lv_color_hex(0xFFFAF5);

UIManager::UIManager() 
    : leftEye_(nullptr)
    , rightEye_(nullptr)
    , leftEyelid_(nullptr)
    , rightEyelid_(nullptr)
    , statusLabel_(nullptr)
    , responseTextArea_(nullptr)
    , usbStatusBottom_(nullptr)
    , gptLogo_(nullptr)
    , showingEyes_(true)
    , eyeAnimationTimer_(nullptr)
    , blinkTimer_(nullptr)
    , eyesCenterTargetX_(0)
    , eyesCenterTargetY_(0)
    , eyesCenterCurrentX_(0)
    , eyesCenterCurrentY_(0)
    , isBlinking_(false)
    , nextBlinkTime_(0)
    , messageQueue_(nullptr)
    , currentState_(UIState::Connecting)
    , typewriterTimer_(nullptr)
    , pendingText_("")
    , currentCharIndex_(0)
    , isTypewriting_(false)
    , typewriterDelayMs_(40)
    , typewriterFinishedTime_(0)
    , thinkingTimer_(nullptr)
    , nextThinkingMoveTime_(0)
    , nextThinkingBlinkTime_(0)
    , lastTouchState_(false)
    , gptLogoTimer_(nullptr)
    , gptLogoToggle_(false) {
}

UIManager::~UIManager() {
    if (eyeAnimationTimer_) {
        lv_timer_del(eyeAnimationTimer_);
    }
    if (blinkTimer_) {
        lv_timer_del(blinkTimer_);
    }
    if (gptLogoTimer_) {
        lv_timer_del(gptLogoTimer_);
    }
    if (typewriterTimer_) { 
        lv_timer_del(typewriterTimer_); 
    }
    if (thinkingTimer_) {
        lv_timer_del(thinkingTimer_);
    }
    if (messageQueue_) {
        vQueueDelete(messageQueue_);
    }
}

bool UIManager::init() {
    // Create message queue for thread-safe communication
    messageQueue_ = xQueueCreate(8, sizeof(UIMessage));
    if (!messageQueue_) {
        Serial.println("[UIManager] Failed to create message queue");
        return false;
    }
    
    setupStyles();
    createUI();
    
    Serial.println("[UIManager] Initialized successfully");
    return true;
}

void UIManager::update() {
    processMessages();
    
    // Auto-ocultar texto después de 2 segundos de terminar de escribir
    if (typewriterFinishedTime_ > 0 && !isTypewriting_) {
        if (millis() - typewriterFinishedTime_ >= 2000) {
            // Han pasado 2 segundos desde que terminó de escribir
            typewriterFinishedTime_ = 0;
            // Volver a estado Ready
            postStateChange(UIState::Ready);
        }
    }
}

void UIManager::postStatus(const char* text) {
    if (!messageQueue_ || !text) return;
    
    UIMessage msg;
    msg.type = UIMessage::STATUS;
    strncpy(msg.text, text, sizeof(msg.text) - 1);
    msg.text[sizeof(msg.text) - 1] = '\0';
    
    xQueueSend(messageQueue_, &msg, 0);
}

void UIManager::postResponse(const char* text) {
    if (!messageQueue_ || !text) return;
    
    UIMessage msg;
    msg.type = UIMessage::RESPONSE;
    strncpy(msg.text, text, sizeof(msg.text) - 1);
    msg.text[sizeof(msg.text) - 1] = '\0';
    
    xQueueSend(messageQueue_, &msg, 0);
}

void UIManager::postStateChange(UIState state) {
    if (!messageQueue_) return;
    
    UIMessage msg;
    msg.type = UIMessage::STATE_CHANGE;
    msg.state = state;
    msg.text[0] = '\0';
    
    xQueueSend(messageQueue_, &msg, 0);
}

void UIManager::setStatus(const char* text) {
    if (statusLabel_ && text) {
        lv_label_set_text(statusLabel_, text);
    }
}

void UIManager::setResponse(const char* text) {
    if (!text) return;
    
    // Switch to text display mode
    showEyes(false);
    
    // Start typewriter animation with the response
    if (responseTextArea_) {
        startTypewriterAnimation(text);
    }
}

void UIManager::setState(UIState state) {
    currentState_ = state;

    // Update eye color based on state
    updateEyeColor(state);

    // Handle different states
    if (state == UIState::ShowingResponse) {
        // Text will be shown via setResponse, just stop eye/thinking and hide logo
        if (thinkingTimer_) {
            lv_timer_del(thinkingTimer_);
            thinkingTimer_ = nullptr;
        }
        stopEyeAnimation();
        stopGPTLogoAnimation();
        setGPTLogoVisible(false);
    } else if (state == UIState::Ready) {
        // Show eyes when ready - ORANGE, active movement
        if (!showingEyes_) {
            showEyes(true);
        }
        if (thinkingTimer_) {
            lv_timer_del(thinkingTimer_);
            thinkingTimer_ = nullptr;
        }
        startEyeAnimation();
        // Logo static and visible
        stopGPTLogoAnimation();
        setGPTLogoVisible(true);
    } else if (state == UIState::Processing) {
        // Thinking/processing - eyes special motion and faster logo animation
        showEyes(true);
        stopEyeAnimation();
        if (thinkingTimer_) {
            lv_timer_del(thinkingTimer_);
        }
        // Initial thinking pose (top-right)
        eyesCenterTargetX_ = 25;
        eyesCenterTargetY_ = -30;
        eyesCenterCurrentX_ = eyesCenterTargetX_;
        eyesCenterCurrentY_ = eyesCenterTargetY_;
        updateEyePositions();

        // Timer for side-to-side motion and blinking while thinking
        thinkingTimer_ = lv_timer_create([](lv_timer_t* t) {
            UIManager* ui = static_cast<UIManager*>(lv_timer_get_user_data(t));
            if (!ui) return;

            uint32_t now = millis();

            if (now >= ui->nextThinkingMoveTime_) {
                static bool lookingRight = true;
                ui->eyesCenterTargetX_ = lookingRight ? -25 : 25;
                ui->eyesCenterTargetY_ = -30;
                lookingRight = !lookingRight;
                ui->nextThinkingMoveTime_ = now + 1500;
            }

            if (now >= ui->nextThinkingBlinkTime_ && !ui->isBlinking_) {
                ui->performBlink();
                ui->nextThinkingBlinkTime_ = now + 2000;
            }

            ui->updateEyePositions();
        }, 30, this);

        nextThinkingMoveTime_ = millis() + 1500;
        nextThinkingBlinkTime_ = millis() + 2000;

        // Logo: animate faster while thinking
        setGPTLogoVisible(true);
    startGPTLogoAnimation(250); // slower thinking animation
    } else if (state == UIState::Recording) {
        // Listening - cross-eyed pose and slower logo animation
        showEyes(true);
        if (thinkingTimer_) {
            lv_timer_del(thinkingTimer_);
            thinkingTimer_ = nullptr;
        }
        stopEyeAnimation();

        // Cross-eyed effect
        if (leftEye_ && rightEye_) {
            const lv_coord_t eyeWidth = 140;
            const lv_coord_t eyeSpacing = 60;
            lv_obj_align(leftEye_, LV_ALIGN_CENTER, -(eyeWidth/2 + eyeSpacing/2) + 15, -5);
            lv_obj_align(rightEye_, LV_ALIGN_CENTER, (eyeWidth/2 + eyeSpacing/2) - 15, 5);
            if (leftEyelid_) lv_obj_align_to(leftEyelid_, leftEye_, LV_ALIGN_CENTER, 0, 0);
            if (rightEyelid_) lv_obj_align_to(rightEyelid_, rightEye_, LV_ALIGN_CENTER, 0, 0);
        }

        // Logo: animate slower while listening
        setGPTLogoVisible(true);
    startGPTLogoAnimation(400); // very slow listening animation
    } else {
        // Other states (Connecting, Error)
        showEyes(true);
        stopEyeAnimation();
        if (thinkingTimer_) {
            lv_timer_del(thinkingTimer_);
            thinkingTimer_ = nullptr;
        }
        // Logo visible and static
        stopGPTLogoAnimation();
        setGPTLogoVisible(true);
    }

    updateStatusBadge(state);
    setStatus(getStateLabel(state));
}

void UIManager::setupStyles() {
    // Screen background style
    lv_style_init(&styleScreenBg_);
    lv_style_set_bg_color(&styleScreenBg_, KODE_BG_DARK);
    
    // Text style
    lv_style_init(&styleText_);
    lv_style_set_text_color(&styleText_, KODE_TEXT_LIGHT);
    lv_style_set_text_font(&styleText_, &Inter_30);
    
    // Text area style for responses
    lv_style_init(&styleTextArea_);
    lv_style_set_bg_opa(&styleTextArea_, LV_OPA_TRANSP);
    lv_style_set_text_color(&styleTextArea_, KODE_TEXT_LIGHT);
    lv_style_set_text_font(&styleTextArea_, &Inter_30);
    
    // Status badge style
    lv_style_init(&styleStatus_);
    lv_style_set_bg_opa(&styleStatus_, LV_OPA_COVER);
    lv_style_set_radius(&styleStatus_, 12);
    lv_style_set_pad_top(&styleStatus_, 6);
    lv_style_set_pad_bottom(&styleStatus_, 6);
    lv_style_set_pad_left(&styleStatus_, 10);
    lv_style_set_pad_right(&styleStatus_, 10);
    lv_style_set_border_width(&styleStatus_, 0);
    lv_style_set_text_font(&styleStatus_, &Inter_30);
    
        // Eye style - will be updated dynamically based on state
    // Default: Orange (Ready state)
    lv_style_init(&styleEye_);
    lv_style_set_bg_color(&styleEye_, lv_color_hex(0xFF8C42)); // Bright orange base
    lv_style_set_bg_grad_color(&styleEye_, lv_color_hex(0xFFB366)); // Lighter orange gradient
    lv_style_set_bg_grad_dir(&styleEye_, LV_GRAD_DIR_VER);
    lv_style_set_bg_grad_stop(&styleEye_, 192);
    lv_style_set_bg_opa(&styleEye_, LV_OPA_COVER);
    // Rounded rectangle
    lv_style_set_radius(&styleEye_, 18);
    // Subtle border
    lv_style_set_border_width(&styleEye_, 2);
    lv_style_set_border_color(&styleEye_, lv_color_hex(0xCC6F33));
    // Outer shadow for depth
    lv_style_set_shadow_width(&styleEye_, 8);
    lv_style_set_shadow_color(&styleEye_, lv_color_hex(0x000000));
    lv_style_set_shadow_ofs_x(&styleEye_, 0);
    lv_style_set_shadow_ofs_y(&styleEye_, 4);
    
    // Eyelid style - matches background perfectly so blinking hides the eye without showing box
    lv_style_init(&styleEyelid_);
    lv_style_set_bg_color(&styleEyelid_, KODE_BG_DARK);
    lv_style_set_bg_opa(&styleEyelid_, LV_OPA_COVER);
    lv_style_set_border_width(&styleEyelid_, 0);
    lv_style_set_radius(&styleEyelid_, 18);
}

void UIManager::createUI() {
    lv_obj_t* scr = lv_scr_act();
    lv_obj_add_style(scr, &styleScreenBg_, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);

    // Create eyes (shown by default)
    createEyes();
    
    // Create text display area (hidden by default)
    createTextDisplay();

    // Create GPT logo (top-right)
    createGPTLogo();

    // Status badge overlayed on screen (hidden - no longer used)
    statusLabel_ = lv_label_create(scr);
    lv_obj_add_style(statusLabel_, &styleStatus_, 0);
    lv_obj_set_style_bg_color(statusLabel_, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(statusLabel_, LV_OPA_80, 0);
    lv_obj_set_style_text_color(statusLabel_, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(statusLabel_, "Starting...");
    lv_label_set_long_mode(statusLabel_, LV_LABEL_LONG_WRAP);
    lv_obj_align(statusLabel_, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_add_flag(statusLabel_, LV_OBJ_FLAG_HIDDEN);  // Always hidden

    // No bottom text - cleaner interface
}

void UIManager::createGPTLogo() {
    lv_obj_t* scr = lv_scr_act();
    if (!scr) return;
    if (!gptLogo_) {
        gptLogo_ = lv_image_create(scr);
    }
    if (!gptLogo_) return;
    lv_image_set_src(gptLogo_, &GPT_basic);
    // 20px from top and right margins
    lv_obj_align(gptLogo_, LV_ALIGN_TOP_RIGHT, -20, 20);
}

void UIManager::setGPTLogoVisible(bool show) {
    if (!gptLogo_) return;
    if (show) lv_obj_clear_flag(gptLogo_, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(gptLogo_, LV_OBJ_FLAG_HIDDEN);
}

void UIManager::startGPTLogoAnimation(uint32_t periodMs) {
    if (!gptLogo_) return;
    if (gptLogoTimer_) {
        lv_timer_set_period(gptLogoTimer_, periodMs);
        return;
    }
    gptLogoToggle_ = false;
    gptLogoTimer_ = lv_timer_create([](lv_timer_t* t){
        auto* ui = static_cast<UIManager*>(lv_timer_get_user_data(t));
        if (!ui || !ui->gptLogo_) return;
        ui->gptLogoToggle_ = !ui->gptLogoToggle_;
        lv_image_set_src(ui->gptLogo_, ui->gptLogoToggle_ ? &GPT_basic_turn : &GPT_basic);
    }, periodMs, this);
}

void UIManager::stopGPTLogoAnimation() {
    if (gptLogoTimer_) {
        lv_timer_del(gptLogoTimer_);
        gptLogoTimer_ = nullptr;
    }
    if (gptLogo_) lv_image_set_src(gptLogo_, &GPT_basic);
}

void UIManager::processMessages() {
    if (!messageQueue_) return;
    
    UIMessage msg;
    while (xQueueReceive(messageQueue_, &msg, 0) == pdTRUE) {
        switch (msg.type) {
            case UIMessage::STATUS:
                setStatus(msg.text);
                break;
                
            case UIMessage::RESPONSE:
                setResponse(msg.text);
                break;
                
            case UIMessage::STATE_CHANGE:
                setState(msg.state);
                break;
        }
    }
}

void UIManager::updateStatusBadge(UIState state) {
    if (!statusLabel_) return;
    
    lv_obj_add_style(statusLabel_, &styleStatus_, 0);
    lv_obj_set_style_bg_color(statusLabel_, getStateColor(state), 0);
    
    // Set text color based on background for better contrast
    if (state == UIState::Ready) {
        lv_obj_set_style_text_color(statusLabel_, lv_color_hex(0x000000), 0); // Black text on orange
    } else {
        lv_obj_set_style_text_color(statusLabel_, lv_color_hex(0xFFFFFF), 0); // White text on other colors
    }
}

void UIManager::centerResponseText() {
    if (!responseTextArea_) return; // kept for compatibility if used elsewhere
    
    const char* currentText = lv_label_get_text(responseTextArea_);
    if (currentText && strlen(currentText) > 0) {
        // Remove cursor character if present for centering calculation
        String textForCentering = String(currentText);
        if (textForCentering.endsWith("|")) {
            textForCentering = textForCentering.substring(0, textForCentering.length() - 1);
        }

        // Fetch current style values used for measuring and centering
        lv_coord_t pad_left   = lv_obj_get_style_pad_left(responseTextArea_, LV_PART_MAIN);
        lv_coord_t pad_right  = lv_obj_get_style_pad_right(responseTextArea_, LV_PART_MAIN);
        lv_coord_t pad_top_min    = lv_obj_get_style_pad_top(responseTextArea_, LV_PART_MAIN);
        lv_coord_t pad_bottom = lv_obj_get_style_pad_bottom(responseTextArea_, LV_PART_MAIN);
        lv_coord_t lineSpacing = lv_obj_get_style_text_line_space(responseTextArea_, LV_PART_MAIN);
        lv_coord_t letterSpace = lv_obj_get_style_text_letter_space(responseTextArea_, LV_PART_MAIN);

        // Compute the maximum text width inside the label (account for paddings)
        lv_coord_t containerWidth = lv_obj_get_width(responseTextArea_);
        lv_coord_t max_text_w = containerWidth - pad_left - pad_right;
        if (max_text_w < 10) max_text_w = containerWidth; // Fallback safety

        // Measure rendered text block size with LVGL's text engine (accounts for wrapping)
    lv_point_t txt_size;
    lv_text_flag_t flags = LV_TEXT_FLAG_NONE;
        lv_txt_get_size(&txt_size,
                        textForCentering.c_str(),
                        &Inter_30,
                        letterSpace,
                        lineSpacing,
                        max_text_w,
                        flags);

    // Calculate vertical centering based on measured text height
    lv_coord_t containerHeight = lv_obj_get_height(responseTextArea_);
    lv_coord_t verticalSpace = containerHeight - txt_size.y;
    lv_coord_t symmetricPad = verticalSpace / 2; // equal top & bottom for perfect centering

    // Clamp to reasonable bounds (avoid negative). Keep at least 4px when overflowing
    if (symmetricPad < 0) symmetricPad = 4;

    // Apply symmetric padding for true vertical centering
    lv_obj_set_style_pad_top(responseTextArea_, symmetricPad, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(responseTextArea_, symmetricPad, LV_PART_MAIN);

        // Ensure horizontal centering remains
        lv_obj_set_style_text_align(responseTextArea_, LV_TEXT_ALIGN_CENTER, 0);

        // Handle opacity: no fade during typewriter
        if (!isTypewriting_) {
            lv_obj_set_style_text_opa(responseTextArea_, LV_OPA_COVER, 0);
        } else {
            lv_obj_set_style_text_opa(responseTextArea_, LV_OPA_COVER, 0);
        }
    } else {
        // Reset to default padding when no text
        lv_obj_set_style_pad_top(responseTextArea_, 15, LV_PART_MAIN);
        lv_obj_set_style_text_opa(responseTextArea_, LV_OPA_COVER, 0);
    }
}

lv_color_t UIManager::getStateColor(UIState state) {
    switch (state) {
        case UIState::Ready:
            return lv_color_hex(0xFF9800); // Orange
        case UIState::Recording:
            return lv_color_hex(0x1E7D32); // Green
        case UIState::Processing:
            return lv_color_hex(0x1976D2); // Blue
        case UIState::ShowingResponse:
            return lv_color_hex(0x9C27B0); // Purple
        case UIState::Error:
            return lv_color_hex(0xB71C1C); // Red
        case UIState::Connecting:
        default:
            return lv_color_hex(0x2B2B2B); // Gray
    }
}

const char* UIManager::getStateLabel(UIState state) {
    switch (state) {
        case UIState::Ready:
            return "";  // Sin título
        case UIState::Recording:
            return "";  // Sin título
        case UIState::Processing:
            return "";  // Sin título
        case UIState::ShowingResponse:
            return ""; // No label when showing response
        case UIState::Error:
            return "Error";
        case UIState::Connecting:
        default:
            return "Starting...";
    }
}

// Eye creation and management
void UIManager::createEyes() {
    lv_obj_t* scr = lv_scr_act();
    
    // Eye dimensions - wider rectangular eyes (robotic look)
    const lv_coord_t eyeWidth = 140;  // Much wider
    const lv_coord_t eyeHeight = 100; // Same height
    const lv_coord_t eyeSpacing = 30; // Closer spacing
    
    // Left eye
    leftEye_ = lv_obj_create(scr);
    lv_obj_set_size(leftEye_, eyeWidth, eyeHeight);
    lv_obj_add_style(leftEye_, &styleEye_, 0);
    lv_obj_clear_flag(leftEye_, LV_OBJ_FLAG_SCROLLABLE); // Not scrollable
    lv_obj_align(leftEye_, LV_ALIGN_CENTER, -(eyeWidth/2 + eyeSpacing/2), 0);
    
    // Right eye
    rightEye_ = lv_obj_create(scr);
    lv_obj_set_size(rightEye_, eyeWidth, eyeHeight);
    lv_obj_add_style(rightEye_, &styleEye_, 0);
    lv_obj_clear_flag(rightEye_, LV_OBJ_FLAG_SCROLLABLE); // Not scrollable
    lv_obj_align(rightEye_, LV_ALIGN_CENTER, (eyeWidth/2 + eyeSpacing/2), 0);
    
    // Eyelids (for blinking) - cover eyes EXACTLY, no visible background
    leftEyelid_ = lv_obj_create(scr);
    lv_obj_set_size(leftEyelid_, eyeWidth, eyeHeight); // Exact same size as eye
    lv_obj_add_style(leftEyelid_, &styleEyelid_, 0);
    lv_obj_clear_flag(leftEyelid_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align_to(leftEyelid_, leftEye_, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(leftEyelid_, LV_OBJ_FLAG_HIDDEN);
    
    rightEyelid_ = lv_obj_create(scr);
    lv_obj_set_size(rightEyelid_, eyeWidth, eyeHeight); // Exact same size as eye
    lv_obj_add_style(rightEyelid_, &styleEyelid_, 0);
    lv_obj_clear_flag(rightEyelid_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align_to(rightEyelid_, rightEye_, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(rightEyelid_, LV_OBJ_FLAG_HIDDEN);
    
    // Initialize eye positions (center of both eyes)
    eyesCenterCurrentX_ = 0;
    eyesCenterCurrentY_ = 0;
    
    setNewEyeTarget();
}

void UIManager::createTextDisplay() {
    lv_obj_t* scr = lv_scr_act();
    
    // Response text area - centered, word wrap enabled
    responseTextArea_ = lv_label_create(scr);
    lv_obj_set_size(responseTextArea_, LV_HOR_RES - 40, LV_VER_RES - 100);
    lv_obj_add_style(responseTextArea_, &styleTextArea_, 0);
    lv_label_set_long_mode(responseTextArea_, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(responseTextArea_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(responseTextArea_);
    lv_obj_add_flag(responseTextArea_, LV_OBJ_FLAG_HIDDEN); // Hidden by default
}

void UIManager::showEyes(bool show) {
    showingEyes_ = show;
    
    if (show) {
        // Show eyes, hide text
        if (leftEye_) lv_obj_clear_flag(leftEye_, LV_OBJ_FLAG_HIDDEN);
        if (rightEye_) lv_obj_clear_flag(rightEye_, LV_OBJ_FLAG_HIDDEN);
        if (responseTextArea_) {
            lv_obj_add_flag(responseTextArea_, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(responseTextArea_, ""); // Clear text
        }
        stopTypewriterAnimation();
        startEyeAnimation();
        // Ensure logo visible when eyes are visible (unless a later state hides it)
        if (gptLogo_) lv_obj_clear_flag(gptLogo_, LV_OBJ_FLAG_HIDDEN);
    } else {
        // Hide eyes, show text
        if (leftEye_) lv_obj_add_flag(leftEye_, LV_OBJ_FLAG_HIDDEN);
        if (rightEye_) lv_obj_add_flag(rightEye_, LV_OBJ_FLAG_HIDDEN);
        if (leftEyelid_) lv_obj_add_flag(leftEyelid_, LV_OBJ_FLAG_HIDDEN);
        if (rightEyelid_) lv_obj_add_flag(rightEyelid_, LV_OBJ_FLAG_HIDDEN);
        if (responseTextArea_) lv_obj_clear_flag(responseTextArea_, LV_OBJ_FLAG_HIDDEN);
        stopEyeAnimation();
        // Hide logo during text screen and stop its animation
        if (gptLogo_) lv_obj_add_flag(gptLogo_, LV_OBJ_FLAG_HIDDEN);
        if (gptLogoTimer_) { lv_timer_del(gptLogoTimer_); gptLogoTimer_ = nullptr; }
    }
}

void UIManager::startEyeAnimation() {
    if (eyeAnimationTimer_ || !showingEyes_) return;
    
    // Create timers for eye movement and blinking
    eyeAnimationTimer_ = lv_timer_create(eyeAnimationTimerCallback, 30, this); // 30ms updates = fluid movement
    blinkTimer_ = lv_timer_create(blinkTimerCallback, 100, this);
    
    nextBlinkTime_ = millis() + random(1500, 3500); // More frequent blinking (curiosity)
    setNewEyeTarget();
}

void UIManager::stopEyeAnimation() {
    if (eyeAnimationTimer_) {
        lv_timer_del(eyeAnimationTimer_);
        eyeAnimationTimer_ = nullptr;
    }
    if (blinkTimer_) {
        lv_timer_del(blinkTimer_);
        blinkTimer_ = nullptr;
    }
}

void UIManager::eyeAnimationTimerCallback(lv_timer_t* timer) {
    UIManager* uiManager = static_cast<UIManager*>(lv_timer_get_user_data(timer));
    if (uiManager && uiManager->showingEyes_) {
        uiManager->updateEyePositions();
    }
}

void UIManager::blinkTimerCallback(lv_timer_t* timer) {
    UIManager* uiManager = static_cast<UIManager*>(lv_timer_get_user_data(timer));
    if (uiManager && uiManager->showingEyes_) {
        // Check if it's time to blink
        if (millis() >= uiManager->nextBlinkTime_ && !uiManager->isBlinking_) {
            uiManager->performBlink();
            uiManager->nextBlinkTime_ = millis() + random(2000, 6000);
        }
    }
}

void UIManager::updateEyePositions() {
    if (!leftEye_ || !rightEye_) return;
    
    // Fast interpolation for snappier movement (curiosity behavior)
    const float smoothing = 0.25f;  // Higher = faster movement
    
    // Move the center point of both eyes together
    eyesCenterCurrentX_ += (eyesCenterTargetX_ - eyesCenterCurrentX_) * smoothing;
    eyesCenterCurrentY_ += (eyesCenterTargetY_ - eyesCenterCurrentY_) * smoothing;
    
    // Apply positions - both eyes move together maintaining fixed horizontal distance
    const lv_coord_t eyeWidth = 140;
    
    // NUEVA MEJORA: Ajustar separación según estado
    lv_coord_t eyeSpacing = 30;  // Default para Ready
    if (currentState_ == UIState::Recording) {
        eyeSpacing = 70;  // Más separados cuando escucha (verde)
    }
    
    // Left eye: center position minus half the spacing
    lv_obj_align(leftEye_, LV_ALIGN_CENTER, 
                 -(eyeWidth/2 + eyeSpacing/2) + eyesCenterCurrentX_, 
                 eyesCenterCurrentY_);
    
    // Right eye: center position plus half the spacing
    lv_obj_align(rightEye_, LV_ALIGN_CENTER, 
                 (eyeWidth/2 + eyeSpacing/2) + eyesCenterCurrentX_, 
                 eyesCenterCurrentY_);
    
    // Update eyelid positions to follow eyes EXACTLY
    if (leftEyelid_) {
        lv_obj_align_to(leftEyelid_, leftEye_, LV_ALIGN_CENTER, 0, 0);
    }
    if (rightEyelid_) {
        lv_obj_align_to(rightEyelid_, rightEye_, LV_ALIGN_CENTER, 0, 0);
    }
    
    // Check if reached target, set new target with shorter delay
    if (abs(eyesCenterCurrentX_ - eyesCenterTargetX_) < 2 && 
        abs(eyesCenterCurrentY_ - eyesCenterTargetY_) < 2) {
        // Quick movements for curious behavior
        static uint32_t lastTargetChange = 0;
        if (millis() - lastTargetChange > random(300, 800)) {  // Shorter intervals
            setNewEyeTarget();
            lastTargetChange = millis();
        }
    }
}

void UIManager::performBlink() {
    if (!leftEyelid_ || !rightEyelid_ || isBlinking_) return;
    
    isBlinking_ = true;
    
    // Show eyelids briefly
    lv_obj_clear_flag(leftEyelid_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(rightEyelid_, LV_OBJ_FLAG_HIDDEN);
    
    // Create a one-shot timer to hide eyelids after blink duration
    lv_timer_t* blinkEndTimer = lv_timer_create([](lv_timer_t* t) {
        UIManager* ui = static_cast<UIManager*>(lv_timer_get_user_data(t));
        if (ui && ui->leftEyelid_ && ui->rightEyelid_) {
            lv_obj_add_flag(ui->leftEyelid_, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui->rightEyelid_, LV_OBJ_FLAG_HIDDEN);
            ui->isBlinking_ = false;
            
            // NUEVA MEJORA: Cambiar posición cada vez que parpadea (solo en estado Ready)
            if (ui->currentState_ == UIState::Ready) {
                ui->setNewEyeTarget();
            }
        }
    }, 150, this); // 150ms blink duration
    
    // Set to run only once and auto-delete
    lv_timer_set_repeat_count(blinkEndTimer, 1);
}

void UIManager::setNewEyeTarget() {
    // Random eye movement - both eyes move together to explore the environment
    const int32_t maxOffsetX = 40; // Wide horizontal scanning
    const int32_t maxOffsetY = 25; // Vertical exploration
    
    // Set a random target for the center point of both eyes
    eyesCenterTargetX_ = random(-maxOffsetX, maxOffsetX);
    eyesCenterTargetY_ = random(-maxOffsetY, maxOffsetY);
}

void UIManager::updateEyeColor(UIState state) {
    if (!leftEye_ || !rightEye_) return;
    
    lv_color_t baseColor, gradColor, borderColor;
    const lv_coord_t normalEyeHeight = 90;
    const lv_coord_t squintedEyeHeight = 65;  // NUEVA MEJORA: Ojos achinados
    
    switch (state) {
        case UIState::Ready:
            // Orange - Active and ready
            baseColor = lv_color_hex(0xFF8C42);
            gradColor = lv_color_hex(0xFFB366);
            borderColor = lv_color_hex(0xCC6F33);
            // Altura normal
            lv_obj_set_height(leftEye_, normalEyeHeight);
            lv_obj_set_height(rightEye_, normalEyeHeight);
            break;
            
        case UIState::Recording:
            // Green - Listening attentively
            baseColor = lv_color_hex(0x4CAF50);
            gradColor = lv_color_hex(0x66BB6A);
            borderColor = lv_color_hex(0x388E3C);
            // Altura normal
            lv_obj_set_height(leftEye_, normalEyeHeight);
            lv_obj_set_height(rightEye_, normalEyeHeight);
            break;
            
        case UIState::Processing:
            // Blue - Thinking/processing
            baseColor = lv_color_hex(0x42A5F5);
            gradColor = lv_color_hex(0x64B5F6);
            borderColor = lv_color_hex(0x1E88E5);
            // NUEVA MEJORA: Ojos achinados cuando piensa (azul)
            lv_obj_set_height(leftEye_, squintedEyeHeight);
            lv_obj_set_height(rightEye_, squintedEyeHeight);
            break;
            
        default:
            // Default to orange
            baseColor = lv_color_hex(0xFF8C42);
            gradColor = lv_color_hex(0xFFB366);
            borderColor = lv_color_hex(0xCC6F33);
            lv_obj_set_height(leftEye_, normalEyeHeight);
            lv_obj_set_height(rightEye_, normalEyeHeight);
            break;
    }
    
    // Update eye colors
    lv_obj_set_style_bg_color(leftEye_, baseColor, 0);
    lv_obj_set_style_bg_grad_color(leftEye_, gradColor, 0);
    lv_obj_set_style_border_color(leftEye_, borderColor, 0);
    
    lv_obj_set_style_bg_color(rightEye_, baseColor, 0);
    lv_obj_set_style_bg_grad_color(rightEye_, gradColor, 0);
    lv_obj_set_style_border_color(rightEye_, borderColor, 0);
}

void UIManager::setEyePosition(int32_t leftX, int32_t leftY, int32_t rightX, int32_t rightY) {
    // NOTE: This function is kept for compatibility but not used with new system
    // For states that need custom positioning, handle directly in setState()
    // Set center position (average of both eyes)
    eyesCenterTargetX_ = (leftX + rightX) / 2;
    eyesCenterTargetY_ = (leftY + rightY) / 2;
    
    // Immediately apply for faster response
    eyesCenterCurrentX_ = eyesCenterTargetX_;
    eyesCenterCurrentY_ = eyesCenterTargetY_;
}

// Typewriter animation implementation
void UIManager::startTypewriterAnimation(const char* text) {
    if (!text || !responseTextArea_) return;
    
    // Stop any existing typewriter animation
    stopTypewriterAnimation();
    
    // Setup typewriter animation
    pendingText_ = String(text);
    currentCharIndex_ = 0;
    isTypewriting_ = true;
    
    // Clear the label and show cursor effect
    lv_label_set_text(responseTextArea_, "");
    
    // Apply active styling immediately
    lv_obj_set_style_border_color(responseTextArea_, lv_color_hex(0x00D4FF), 0); // Bright cyan when active
    lv_obj_set_style_shadow_opa(responseTextArea_, LV_OPA_60, 0); // More pronounced shadow
    lv_obj_set_style_bg_color(responseTextArea_, lv_color_hex(0x1F1F1F), 0);
    lv_obj_set_style_text_color(responseTextArea_, lv_color_hex(0xF5F5F5), 0);  // Restore white text for responses
    
    // Create timer for typewriter effect
    typewriterTimer_ = lv_timer_create(typewriterTimerCallback, typewriterDelayMs_, this);
    
    Serial.printf("[UIManager] Started typewriter animation for %d characters\n", pendingText_.length());
}

void UIManager::stopTypewriterAnimation() {
    if (typewriterTimer_) {
        lv_timer_del(typewriterTimer_);
        typewriterTimer_ = nullptr;
    }
    isTypewriting_ = false;
    currentCharIndex_ = 0;
    pendingText_ = "";
}

void UIManager::typewriterTimerCallback(lv_timer_t* timer) {
    UIManager* uiManager = static_cast<UIManager*>(lv_timer_get_user_data(timer));
    if (!uiManager) return;
    
    uiManager->updateTypewriterText();
}

void UIManager::updateTypewriterText() {
    if (!isTypewriting_ || !responseTextArea_ || currentCharIndex_ >= pendingText_.length()) {
        // Animation complete
        if (isTypewriting_) {
            isTypewriting_ = false;
            if (typewriterTimer_) {
                lv_timer_del(typewriterTimer_);
                typewriterTimer_ = nullptr;
            }
            // Ensure final text is complete and centered
            lv_label_set_text(responseTextArea_, pendingText_.c_str());
            centerResponseText();
            // Guardar tiempo de finalización para auto-ocultar después de 2s
            typewriterFinishedTime_ = millis();
            Serial.println("[UIManager] Typewriter animation completed");
        }
        return;
    }
    
    // Add next character(s) - handle UTF-8 properly
    String currentText = pendingText_.substring(0, currentCharIndex_ + 1);
    
    // Add typing cursor effect
    String displayText = currentText + "|";
    lv_label_set_text(responseTextArea_, displayText.c_str());
    
    // Update centering as text grows
    centerResponseText();
    
    currentCharIndex_++;
    
    // Variable speed: slower for punctuation, faster for regular characters
    char currentChar = pendingText_.charAt(currentCharIndex_ - 1);
    uint32_t nextDelay = typewriterDelayMs_;
    
    if (currentChar == '.' || currentChar == '!' || currentChar == '?') {
        nextDelay = typewriterDelayMs_ * 8; // Pause longer after sentences
    } else if (currentChar == ',' || currentChar == ';' || currentChar == ':') {
        nextDelay = typewriterDelayMs_ * 4; // Medium pause after commas
    } else if (currentChar == ' ') {
        nextDelay = typewriterDelayMs_ / 2; // Faster through spaces
    }
    
    // Update timer period for next character
    if (typewriterTimer_) {
        lv_timer_set_period(typewriterTimer_, nextDelay);
    }
}

void UIManager::setTypewriterSpeed(uint32_t delayMs) {
    // Clamp the delay to reasonable bounds (10ms to 200ms)
    if (delayMs < 10) delayMs = 10;
    if (delayMs > 200) delayMs = 200;
    
    typewriterDelayMs_ = delayMs;
    Serial.printf("[UIManager] Typewriter speed set to %d ms per character\n", delayMs);
}

void UIManager::setUSBConnectionStatus(bool connected) {
    // Not used in CuteAssistant mode
    (void)connected;
}

bool UIManager::isTouchPressed() {
    lv_indev_t* indev = lv_indev_get_next(nullptr);
    if (!indev) return false;
    
    // Get touch state directly from indev
    lv_indev_state_t state = lv_indev_get_state(indev);
    bool currentTouch = (state == LV_INDEV_STATE_PRESSED);
    
    // Simple debouncing - detect press edge
    bool pressed = currentTouch && !lastTouchState_;
    lastTouchState_ = currentTouch;
    
    return pressed;
}
