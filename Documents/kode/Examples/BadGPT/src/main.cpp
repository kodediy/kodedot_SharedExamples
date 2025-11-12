/*
 * CuteAssistant - AI Voice Assistant with ESP32-S3 Kode Dot
 * ---------------------------------------------------------
 * - Touch screen to talk to the assistant
 * - Animated cute eyes respond to interaction
 * - AI responses shown on screen with typewriter effect
 * - Eyes disappear when showing text responses
 * - GPIO control via voice commands
 */
#include <Arduino.h>
#include <kodedot/display_manager.h>
#include <kodedot/pin_config.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <vector>
#include <ESP32Servo.h>
#include <Wire.h>
#include <PMIC_BQ25896.h>
#include <Wire.h>
#include <PMIC_BQ25896.h>
// USB HID for keyboard actions (BadUSB style)
#ifdef USE_USB_KEYBOARD_ACTIONS
#include "USB.h"
#include "USBHIDKeyboard.h"
#include "tusb.h"
#endif

// ==================== API SELECTION ====================
// Uncomment ONE of these to choose API implementation:
// #define USE_REALTIME_API      // WebSocket streaming (ultra-low latency)
#define USE_CHAT_API          // HTTP POST (stable, proven)

// Project libraries
#include <audio_manager/AudioManager.h>
#include <ui_manager/UIManager.h>
#include <led_manager/LEDManager.h>
#include <wifi_manager_lib/WiFiManager.h>

#ifdef USE_REALTIME_API
#include <realtime_client/RealtimeClient.h>
#else
#include <basicgpt_client/BasicGPTClient.h>
#endif

// Generated resources
extern const lv_font_t Inter_30;

// Core managers
DisplayManager display;
AudioManager audioManager;
UIManager uiManager;
LEDManager ledManager;
PMIC_BQ25896 pmic;
#ifdef USE_USB_KEYBOARD_ACTIONS
static USBHIDKeyboard Keyboard; // HID keyboard instance
enum OSType { OS_UNKNOWN = 0, OS_WINDOWS, OS_MAC };
static OSType current_os = OS_UNKNOWN;
enum KeyboardLayout { LAYOUT_DEFAULT = 0, LAYOUT_ES, LAYOUT_US };
static KeyboardLayout current_layout = LAYOUT_ES; // default to ES for special chars

// Forward declarations for USB keyboard helpers
static void sendKeyCombination(uint8_t modifiers, uint8_t key);
static void sendCombo(const uint8_t* mods, size_t modCount, uint8_t key);
static void sendHotkeyFromSpec(const String &spec);
static void typeText(const char* text);
static void openRunDialog();
static void openSearch();
static void typeSpanishChar(char c);
static void typeSpanishCharWindows(char c);
static void typeSpanishCharMac(char c);
#endif

// Configuration
static const uint32_t GUI_LOOP_DELAY_MS = 5;
static const uint32_t WIFI_CHECK_INTERVAL_MS = 15000;
static const uint32_t TOUCH_DEBOUNCE_MS = 200;    // Prevent rapid touches

// Available GPIO pins for user control (from pinout diagram)
static const int AVAILABLE_GPIOS[] = {1, 2, 3, 11, 12, 13, 39, 40, 41, 42};
static const int NUM_AVAILABLE_GPIOS = sizeof(AVAILABLE_GPIOS) / sizeof(AVAILABLE_GPIOS[0]);

// ==================== PMIC BQ25896 - 5V BUS CONTROL ====================
static void initPMIC() {
    Serial.println("[PMIC] Inicializando BQ25896 para habilitar bus 5V...");
    
    // NO llamar a Wire.begin() aquí - ya está inicializado por el display
    // Solo inicializar el objeto PMIC
    pmic.begin();
    delay(200);
    
    // Habilitar conversión continua de ADC (1 Hz) para refrescar medidas
    pmic.setCONV_RATE(true);
    delay(50);
    
    // CONFIGURAR BOOST VOLTAGE (5V típico)
    // El registro BOOSTV controla el voltaje de salida del boost
    pmic.setBOOST_LIM(true);  // Set current limit for boost
    delay(50);
    
    // HABILITAR OTG/BOOST MODE - 5V de salida en el bus
    Serial.println("[PMIC] Habilitando modo OTG/Boost para 5V...");
    pmic.setOTG_CONFIG(true);  // enable OTG/boost (5V out)
    delay(100);  // Dar tiempo para que se estabilice
    
    // Verificar estado
    auto vstat = pmic.get_VBUS_STAT_reg();
    bool haveUsb = vstat.pg_stat;  // Power Good on VBUS
    
    Serial.printf("[PMIC] USB=%s, Estado carga=%d\n", haveUsb ? "conectado" : "no conectado", vstat.chrg_stat);
    Serial.println("[PMIC] ✅ Bus de 5V HABILITADO - Modo OTG/Boost activado");
}

// Servo management (max 10 servos)
#define CUTEASSISTANT_MAX_SERVOS 10
static Servo g_servos[CUTEASSISTANT_MAX_SERVOS];
static int g_servo_pins[CUTEASSISTANT_MAX_SERVOS] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

// Command execution state machine
enum class CommandState {
    IDLE,
    EXECUTING,
    WAITING
};

struct ActionCommand {
    enum Type {
        // Keyboard actions
        KB_HOTKEY,
        KB_STRING,
        KB_ENTER,
        KB_RUN,
        KB_SEARCH,
        OS_SET,
        LAYOUT_SET,
        KB_SWITCH_LAYOUT,
        // Timing
        DELAY,
        // Unknown
        UNKNOWN
    } type;

    String str;        // spec/text/OS
    uint32_t delay_ms; // For DELAY command
};

struct CommandSequence {
    std::vector<ActionCommand> commands;
    size_t currentIndex;
    uint32_t waitUntil;
    CommandState state;
    
    CommandSequence() : currentIndex(0), waitUntil(0), state(CommandState::IDLE) {}
};

static CommandSequence g_active_sequence;

#ifdef USE_USB_KEYBOARD_ACTIONS
// Keyboard timing constants (must be declared before use in executeNextCommand)
static const uint16_t KB_TYPING_DELAY_MS = 5;           // per character base delay
static const uint16_t KB_COMBO_PRESS_DELAY_MS = 30;     // key hold before release in combos
static const uint16_t KB_COMBO_RELEASE_DELAY_MS = 20;   // small gap after combo release
static const uint16_t KB_ENTER_GUARD_DELAY_MS = 300;    // wait after typing before sending ENTER
static const uint16_t KB_APP_OPEN_MIN_DELAY_MS = 600;   // wait after opening RUN/SEARCH/launcher before typing
#endif

// OpenAI Configuration
static const char* OPENAI_HOST = "api.openai.com";
static const int OPENAI_PORT = 443;
static const char* OPENAI_MODEL = "gpt-4o-audio-preview";
// Optimized assistant prompt with structured output
// Optimized assistant prompt with structured output and GPIO control
static const char* SYSTEM_PROMPT =
    "You are CuteAssistant, a friendly AI companion who imagines having a human body to interact with the world. "
    "You communicate in clear, conversational English with warmth and subtle wit. "
    "Keep responses concise (2-3 sentences) and engaging. Never use emojis - keep text clean. "
    "\n\nCRITICAL OUTPUT FORMAT - You MUST ALWAYS respond EXACTLY like this:"
    "\nResponse: [Your conversational reply ONLY - no actions here]"
    "\nActions: [BEGIN; keyboard commands ;END] or 'none'"
    "\n\nIMPORTANT RULES:"
    "\n1. The Response line is what the USER SEES on screen - keep it conversational and clean"
    "\n2. The Actions line is for USB keyboard commands - the user NEVER sees this"
    "\n3. NEVER mix control commands into your Response text"
    "\n4. ALWAYS include both lines, even if Actions is just 'none'"
    "\n\nUSB KEYBOARD CONTROL FORMAT:"
    "\nBEGIN;HOTKEY(spec);STRING(text);TYPE(text);ENTER;RUN;SEARCH;DELAY(ms);OS(mac|windows);LAYOUT(es|us);SWITCH_LAYOUT;END"
    "\n- HOTKEY(spec): spec like 'CTRL+SHIFT+P', 'COMMAND+SPACE', 'ALT+F4', sequences with comma: 'CTRL+C, CTRL+V'"
    "\n- STRING(text) or TYPE(text): type the given text literally"
    "\n- ENTER: press Enter/Return"
    "\n- RUN: open Run dialog (Windows) or Spotlight (macOS)"
    "\n- SEARCH: open Search (Windows) or Spotlight (macOS)"
    "\n- DELAY(ms): wait between actions"
    "\n- OS(mac|windows): set target OS if the user mentions it"
    "\n- LAYOUT(es|us): prefer Spanish or US layout handling for special characters"
    "\n- SWITCH_LAYOUT: toggle the host OS keyboard input source (uses typical OS hotkeys)"
    "\n\nTIMING RULES (MANDATORY):"
    "\n- Always insert DELAY(600) or more immediately after RUN, SEARCH, or a launcher HOTKEY (COMMAND+SPACE, GUI r) before typing."
    "\n- Always insert DELAY(300) or more between finishing STRING/TYPE and sending ENTER."
    "\n- Prefer longer DELAY(800-1200) for heavier apps (e.g., notepad, terminal, browser) before typing inside them."
    "\n- If uncertain, choose a safe delay (e.g., DELAY(900)) rather than too short."
    "\n\nMACOS TERMINAL COMMANDS - CRITICAL RULES:"
    "\n- Use absolute path /Users/username/Desktop instead of shortcuts"
    "\n- Keep folder names SIMPLE: letters, numbers, underscore, hyphen ONLY"
    "\n- NO special characters: avoid ~ $ & * ( ) [ ] { } | ; < > ? ! @ # % ^ space"
    "\n- Good names: test_folder, my-folder, folder123"
    "\n- Bad names: my folder, test?folder, ~folder, $test"
    "\n\nEXAMPLES:"
    "\nUser: 'Open Spotlight and search for Terminal'"
    "\nResponse: Opening Spotlight and searching for Terminal."
    "\nActions: BEGIN;OS(mac);LAYOUT(es);HOTKEY(COMMAND+SPACE);DELAY(600);STRING(Terminal);ENTER;END"
    "\n\nUser: 'Open terminal and create a folder on desktop'"
    "\nResponse: Opening Terminal and creating a folder on your Desktop."
    "\nActions: BEGIN;OS(mac);LAYOUT(es);HOTKEY(COMMAND+SPACE);DELAY(600);STRING(Terminal);ENTER;DELAY(1200);STRING(mkdir test_folder);ENTER;END"
    "\n\nUser: 'Copy then paste'"
    "\nResponse: Copying and pasting now."
    "\nActions: BEGIN;HOTKEY(CTRL+C, CTRL+V);END"
    "\n\nUser: 'Run notepad and type hello'"
    "\nResponse: Launching Notepad and typing your message."
    "\nActions: BEGIN;OS(windows);LAYOUT(es);RUN;DELAY(600);STRING(notepad);ENTER;DELAY(800);TYPE(hello);END"
    "\n\nUser: 'Hello!'"
    "\nResponse: Hey there! How can I help you today?"
    "\nActions: none"
    "\n\nREMEMBER:"
    "\n- Response = what user sees (conversational text)"
    "\n- Actions = USB keyboard commands (never shown to user)"
    "\n- macOS terminal: SIMPLE folder names, NO special chars"
    "\n- You can chain multiple commands with DELAYs"
    "\n- Keep them separate!";
static const uint32_t HTTP_TIMEOUT_MS = 20000;
static const uint32_t MAX_CONVERSATION_HISTORY = 6; // Keep last 3 exchanges (user + assistant)

// Conversational memory structure
struct ConversationMessage {
    String role;    // "user" or "assistant"
    String content;
};

// Audio streaming structures
struct AudioChunk {
    uint8_t* data;
    size_t size;
};

#define MAX_AUDIO_CHUNKS 32
#define CHUNK_POOL_SIZE 8192  // 8KB per chunk

// State management
static TaskHandle_t g_openai_task = nullptr;
static uint32_t g_last_wifi_check_ms = 0;
static uint32_t g_bench_start_ms = 0;
static QueueHandle_t g_led_queue = nullptr; // async LED requests
static QueueHandle_t g_audio_chunk_queue = nullptr; // streaming audio chunks
static uint32_t g_last_touch_ms = 0; // Touch debouncing
static std::vector<ChatMessage> g_conversation_history; // Memory for context
static bool g_streaming_active = false; // Flag to control streaming

// Forward declarations
static void openaiTask(void *arg); // OpenAI task declaration

// Structure to hold parsed GPT response
struct ParsedResponse {
    String displayText;  // Text to show on screen
    String action;       // Action command for device
    bool valid;          // Whether parsing was successful
};

// Parse structured GPT response into display text and action
static ParsedResponse parseGPTResponse(const String& rawResponse) {
    ParsedResponse result;
    result.valid = false;
    result.action = "none";
    result.displayText = "";
    
    Serial.println("\n=== PARSING GPT RESPONSE ===");
    Serial.printf("Raw response: '%s'\n", rawResponse.c_str());
    
    String workingText = rawResponse;
    workingText.trim();
    
    // Step 1: Extract GPIO/SERVO commands (BEGIN...END blocks)
    int beginIdx = workingText.indexOf("BEGIN;");
    int endIdx = workingText.indexOf(";END");
    
    if (beginIdx != -1 && endIdx != -1 && endIdx > beginIdx) {
        // Extract the command block
        result.action = workingText.substring(beginIdx, endIdx + 4); // Include ";END"
        result.action.trim();
        
        // Remove it from display text
        String beforeCmd = workingText.substring(0, beginIdx);
        String afterCmd = workingText.substring(endIdx + 4);
        workingText = beforeCmd + afterCmd;
        
        Serial.printf("📦 Extracted command block: '%s'\n", result.action.c_str());
    }
    
    // Step 2: Find "Response:" and "Actions:" markers
    int responseIdx = workingText.indexOf("Response:");
    int actionIdx = workingText.indexOf("Actions:");
    
    Serial.printf("Response marker: %d, Actions marker: %d\n", responseIdx, actionIdx);
    
    // Step 3: Extract response text
    if (responseIdx != -1) {
        int responseStart = responseIdx + 9; // Length of "Response:"
        int responseEnd = (actionIdx != -1) ? actionIdx : workingText.length();
        result.displayText = workingText.substring(responseStart, responseEnd);
    } else {
        // No "Response:" marker - use everything except "Actions:" line
        if (actionIdx != -1) {
            result.displayText = workingText.substring(0, actionIdx);
        } else {
            result.displayText = workingText;
        }
    }
    
    result.displayText.trim();
    
    // Step 4: Extract action from "Actions:" line if present and no BEGIN...END found
    if (actionIdx != -1 && result.action == "none") {
        int actionStart = actionIdx + 8; // Length of "Actions:"
        String actionText = workingText.substring(actionStart);
        actionText.trim();
        
        // Check if it's a line break, then skip it
        int newlinePos = actionText.indexOf('\n');
        if (newlinePos != -1) {
            actionText = actionText.substring(0, newlinePos);
            actionText.trim();
        }
        
        if (actionText.length() > 0 && actionText != "none") {
            result.action = actionText;
        }
    }
    
    // Step 5: Final cleanup - remove any remaining "Actions:" lines from display text
    int actionsLineIdx = result.displayText.indexOf("Actions:");
    if (actionsLineIdx != -1) {
        result.displayText = result.displayText.substring(0, actionsLineIdx);
        result.displayText.trim();
    }
    
    result.valid = true;
    Serial.printf("✅ Display text: '%s'\n", result.displayText.c_str());
    Serial.printf("✅ Action: '%s'\n", result.action.c_str());
    Serial.println("============================\n");
    
    return result;
}

// Execute device action based on GPT command
// Forward declarations
static void openaiTask(void *arg); // OpenAI task declaration

// GPIO Command Parser and Executor
// Format: BEGIN;GPIO(11,ON);DELAY(1000);GPIO(11,OFF);END

static bool isGPIOAvailable(int pin) {
    for (int i = 0; i < NUM_AVAILABLE_GPIOS; i++) {
        if (AVAILABLE_GPIOS[i] == pin) return true;
    }
    return false;
}

// Find servo index for a pin, or -1 if not found
static int findServoIndex(int pin) {
    for (int i = 0; i < CUTEASSISTANT_MAX_SERVOS; i++) {
        if (g_servo_pins[i] == pin) return i;
    }
    return -1;
}

// Attach servo to pin (reuse existing or find empty slot)
static bool attachServo(int pin, int& servoIndex) {
    if (!isGPIOAvailable(pin)) {
        Serial.printf("[SERVO] Error: Pin %d not available\n", pin);
        return false;
    }
    
    // Check if servo already attached to this pin
    servoIndex = findServoIndex(pin);
    if (servoIndex != -1) {
        return true; // Already attached
    }
    
    // Find empty slot
    for (int i = 0; i < CUTEASSISTANT_MAX_SERVOS; i++) {
        if (g_servo_pins[i] == -1) {
            g_servos[i].attach(pin);
            g_servo_pins[i] = pin;
            servoIndex = i;
            Serial.printf("[SERVO] Attached servo to pin %d (slot %d)\n", pin, i);
            return true;
        }
    }
    
    Serial.println("[SERVO] Error: No free servo slots");
    return false;
}

static ActionCommand parseCommand(const String& cmd) {
    ActionCommand action;
    action.type = ActionCommand::UNKNOWN;
    
    String trimmed = cmd;
    trimmed.trim();
    
    // HOTKEY(spec)
    if (trimmed.startsWith("HOTKEY(")) {
        int openParen = trimmed.indexOf('(');
        int closeParen = trimmed.lastIndexOf(')');
        if (openParen != -1 && closeParen != -1 && closeParen > openParen) {
            String spec = trimmed.substring(openParen + 1, closeParen);
            spec.trim();
            action.str = spec;
            action.type = ActionCommand::KB_HOTKEY;
            Serial.printf("[CMD] Parsed HOTKEY: %s\n", action.str.c_str());
        }
    }
    // STRING(text) or TYPE(text)
    else if (trimmed.startsWith("STRING(") || trimmed.startsWith("TYPE(")) {
        int openParen = trimmed.indexOf('(');
        int closeParen = trimmed.lastIndexOf(')');
        if (openParen != -1 && closeParen != -1 && closeParen > openParen) {
            String text = trimmed.substring(openParen + 1, closeParen);
            text.trim();
            action.str = text;
            action.type = ActionCommand::KB_STRING;
            Serial.printf("[CMD] Parsed STRING/TYPE: '%s'\n", action.str.c_str());
        }
    }
    // ENTER
    else if (trimmed.equalsIgnoreCase("ENTER") || trimmed.startsWith("ENTER(")) {
        action.type = ActionCommand::KB_ENTER;
        Serial.println("[CMD] Parsed ENTER");
    }
    // RUN
    else if (trimmed.equalsIgnoreCase("RUN") || trimmed.startsWith("RUN(")) {
        action.type = ActionCommand::KB_RUN;
        Serial.println("[CMD] Parsed RUN");
    }
    // SEARCH
    else if (trimmed.equalsIgnoreCase("SEARCH") || trimmed.startsWith("SEARCH(")) {
        action.type = ActionCommand::KB_SEARCH;
        Serial.println("[CMD] Parsed SEARCH");
    }
    // OS(mac|windows)
    else if (trimmed.startsWith("OS(")) {
        int openParen = trimmed.indexOf('(');
        int closeParen = trimmed.indexOf(')');
        if (openParen != -1 && closeParen != -1) {
            String osName = trimmed.substring(openParen + 1, closeParen);
            osName.trim(); osName.toLowerCase();
            action.str = osName;
            action.type = ActionCommand::OS_SET;
            Serial.printf("[CMD] Parsed OS: %s\n", action.str.c_str());
        }
    }
    // LAYOUT(es|us)
    else if (trimmed.startsWith("LAYOUT(")) {
        int openParen = trimmed.indexOf('(');
        int closeParen = trimmed.indexOf(')');
        if (openParen != -1 && closeParen != -1) {
            String layoutName = trimmed.substring(openParen + 1, closeParen);
            layoutName.trim(); layoutName.toLowerCase();
            action.str = layoutName; // reuse str field
            action.type = ActionCommand::LAYOUT_SET;
            Serial.printf("[CMD] Parsed LAYOUT: %s\n", action.str.c_str());
        }
    }
    // SWITCH_LAYOUT (toggle system input source)
    else if (trimmed.equalsIgnoreCase("SWITCH_LAYOUT") || trimmed.startsWith("SWITCH_LAYOUT(")) {
        action.type = ActionCommand::KB_SWITCH_LAYOUT;
        Serial.println("[CMD] Parsed SWITCH_LAYOUT");
    }
    // DELAY(milliseconds)
    else if (trimmed.startsWith("DELAY(")) {
        int openParen = trimmed.indexOf('(');
        int closeParen = trimmed.indexOf(')');
        
        if (openParen != -1 && closeParen != -1) {
            String delayStr = trimmed.substring(openParen + 1, closeParen);
            delayStr.trim();
            
            action.delay_ms = delayStr.toInt();
            action.type = ActionCommand::DELAY;
            
            Serial.printf("[CMD] Parsed DELAY: %u ms\n", action.delay_ms);
        }
    }
    
    return action;
}

static void parseActionSequence(const String& actionStr, CommandSequence& sequence) {
    sequence.commands.clear();
    sequence.currentIndex = 0;
    sequence.waitUntil = 0;
    sequence.state = CommandState::IDLE;
    
    // Check for BEGIN and END markers
    int beginIdx = actionStr.indexOf("BEGIN");
    int endIdx = actionStr.indexOf("END");
    
    if (beginIdx == -1 || endIdx == -1) {
        Serial.println("[CMD] Warning: No BEGIN/END markers found in action sequence");
        return;
    }
    
    // Extract commands between BEGIN and END
    String commandsStr = actionStr.substring(beginIdx + 5, endIdx);
    commandsStr.trim();
    
    Serial.printf("[CMD] Parsing action sequence: %s\n", commandsStr.c_str());
    
    // Split by semicolon
    int lastPos = 0;
    int pos = 0;
    while ((pos = commandsStr.indexOf(';', lastPos)) != -1) {
        String cmd = commandsStr.substring(lastPos, pos);
        cmd.trim();
        
        if (cmd.length() > 0) {
            ActionCommand action = parseCommand(cmd);
            if (action.type != ActionCommand::UNKNOWN) {
                sequence.commands.push_back(action);
            }
        }
        
        lastPos = pos + 1;
    }
    
    // Process last command if no trailing semicolon
    if (lastPos < commandsStr.length()) {
        String cmd = commandsStr.substring(lastPos);
        cmd.trim();
        if (cmd.length() > 0) {
            ActionCommand action = parseCommand(cmd);
            if (action.type != ActionCommand::UNKNOWN) {
                sequence.commands.push_back(action);
            }
        }
    }
    
    Serial.printf("[CMD] Parsed %d commands in sequence\n", sequence.commands.size());
    
    if (sequence.commands.size() > 0) {
        sequence.state = CommandState::EXECUTING;
    }
}

static void executeNextCommand(CommandSequence& sequence) {
    if (sequence.state != CommandState::EXECUTING) return;
    if (sequence.currentIndex >= sequence.commands.size()) {
        Serial.println("[CMD] Sequence completed");
        sequence.state = CommandState::IDLE;
        return;
    }
    
    ActionCommand& cmd = sequence.commands[sequence.currentIndex];
    
    switch (cmd.type) {
    case ActionCommand::KB_HOTKEY: {
#ifdef USE_USB_KEYBOARD_ACTIONS
        Serial.printf("[KB] HOTKEY: %s\n", cmd.str.c_str());
        sendHotkeyFromSpec(cmd.str);
#else
        Serial.println("[KB] HOTKEY skipped (USB disabled)");
#endif
        sequence.currentIndex++;
        break;
    }
    case ActionCommand::KB_STRING: {
#ifdef USE_USB_KEYBOARD_ACTIONS
        Serial.printf("[KB] STRING: '%s'\n", cmd.str.c_str());
        typeText(cmd.str.c_str());
#else
        Serial.println("[KB] STRING skipped (USB disabled)");
#endif
        sequence.currentIndex++;
        break;
    }
        case ActionCommand::KB_ENTER: {
#ifdef USE_USB_KEYBOARD_ACTIONS
            // guard time to ensure last typed chars are received by host
            delay(KB_ENTER_GUARD_DELAY_MS);
            Keyboard.write(KEY_RETURN);
#endif
            Serial.println("[KB] ENTER");
            sequence.currentIndex++;
            break;
        }
    case ActionCommand::KB_RUN: {
#ifdef USE_USB_KEYBOARD_ACTIONS
        openRunDialog();
#endif
        Serial.println("[KB] RUN");
        sequence.currentIndex++;
        break;
    }
    case ActionCommand::KB_SEARCH: {
#ifdef USE_USB_KEYBOARD_ACTIONS
        openSearch();
#endif
        Serial.println("[KB] SEARCH");
        sequence.currentIndex++;
        break;
    }
        case ActionCommand::OS_SET: {
#ifdef USE_USB_KEYBOARD_ACTIONS
            String os = cmd.str; os.toLowerCase();
            if (os.indexOf("mac") >= 0) current_os = OS_MAC;
            else if (os.indexOf("win") >= 0) current_os = OS_WINDOWS;
            else current_os = OS_UNKNOWN;
            Serial.printf("[KB] OS set to %s\n", current_os == OS_MAC ? "MAC" : (current_os == OS_WINDOWS ? "WINDOWS" : "UNKNOWN"));
#else
            Serial.println("[KB] OS_SET skipped (USB disabled)");
#endif
            sequence.currentIndex++;
            break;
        }
        case ActionCommand::LAYOUT_SET: {
#ifdef USE_USB_KEYBOARD_ACTIONS
            String layout = cmd.str; layout.toLowerCase();
            if (layout.indexOf("es") >= 0 || layout.indexOf("spanish") >= 0) current_layout = LAYOUT_ES;
            else if (layout.indexOf("us") >= 0 || layout.indexOf("english") >= 0) current_layout = LAYOUT_US;
            else current_layout = LAYOUT_DEFAULT;
            Serial.printf("[KB] Layout set to %s\n", current_layout == LAYOUT_ES ? "ES" : (current_layout == LAYOUT_US ? "US" : "DEFAULT"));
#else
            Serial.println("[KB] LAYOUT_SET skipped (USB disabled)");
#endif
            sequence.currentIndex++;
            break;
        }
        case ActionCommand::KB_SWITCH_LAYOUT: {
#ifdef USE_USB_KEYBOARD_ACTIONS
            // Attempt typical OS hotkeys for switching layout
            if (current_os == OS_WINDOWS) {
                // Windows: SHIFT+ALT or WIN+SPACE
                uint8_t mods1[] = {KEY_LEFT_SHIFT, KEY_LEFT_ALT};
                sendCombo(mods1, 2, 0);
            } else if (current_os == OS_MAC) {
                // macOS default: CTRL+SPACE or COMMAND+SPACE (Spotlight conflicts). Try CTRL+SPACE then COMMAND+SPACE.
                uint8_t mods2[] = {KEY_LEFT_CTRL};
                sendCombo(mods2, 1, ' ');
            }
            Serial.println("[KB] SWITCH_LAYOUT attempted");
#else
            Serial.println("[KB] SWITCH_LAYOUT skipped (USB disabled)");
#endif
            sequence.currentIndex++;
            break;
        }
        case ActionCommand::DELAY:
            Serial.printf("[CMD] Executing: DELAY %u ms\n", cmd.delay_ms);
            sequence.waitUntil = millis() + cmd.delay_ms;
            sequence.state = CommandState::WAITING;
            sequence.currentIndex++;
            break;
            
        default:
            Serial.println("[CMD] Error: Unknown command type");
            sequence.currentIndex++;
            break;
    }
}

static void updateCommandSequence() {
    if (g_active_sequence.state == CommandState::IDLE) return;
    
    if (g_active_sequence.state == CommandState::WAITING) {
        if (millis() >= g_active_sequence.waitUntil) {
            g_active_sequence.state = CommandState::EXECUTING;
        } else {
            return; // Still waiting
        }
    }
    
    if (g_active_sequence.state == CommandState::EXECUTING) {
        executeNextCommand(g_active_sequence);
    }
}

// Helper function to normalize Unicode text for font compatibility
static String normalizeTextForDisplay(const String& text) {
    String normalized = text;
    
    // Replace UTF-8 curly quotes with straight quotes
    normalized.replace("\xE2\x80\x99", "'");  // U+2019 RIGHT SINGLE QUOTATION MARK
    normalized.replace("\xE2\x80\x98", "'");  // U+2018 LEFT SINGLE QUOTATION MARK
    normalized.replace("\xE2\x80\x9C", "\""); // U+201C LEFT DOUBLE QUOTATION MARK
    normalized.replace("\xE2\x80\x9D", "\""); // U+201D RIGHT DOUBLE QUOTATION MARK
    normalized.replace("\xE2\x80\x93", "-");  // U+2013 EN DASH
    normalized.replace("\xE2\x80\x94", "-");  // U+2014 EM DASH
    normalized.replace("\xE2\x80\xA6", "..."); // U+2026 HORIZONTAL ELLIPSIS
    
    return normalized;
}

// Helper function to update display immediately
static void updateDisplayNow() {
    display.update();
    uiManager.update();
}

// Helper to request LED state changes from any task, applied in main loop
static void ledRequest(LEDState state) {
    if (!g_led_queue) return;
    uint8_t v = static_cast<uint8_t>(state);
    xQueueSend(g_led_queue, &v, 0);
}

static void ledDrainRequests() {
    if (!g_led_queue) return;
    uint8_t v;
    while (xQueueReceive(g_led_queue, &v, 0) == pdTRUE) {
        ledManager.setState(static_cast<LEDState>(v));
    }
}

#ifdef USE_USB_KEYBOARD_ACTIONS
// ===== USB Keyboard helpers =====
// (Moved timing constants earlier above executeNextCommand)

static void sendKeyCombination(uint8_t modifiers, uint8_t key) {
    if (modifiers) Keyboard.press(modifiers);
    if (key) Keyboard.press(key);
    delay(KB_COMBO_PRESS_DELAY_MS);
    if (key) Keyboard.release(key);
    if (modifiers) Keyboard.release(modifiers);
    delay(KB_COMBO_RELEASE_DELAY_MS);
}

static void sendCombo(const uint8_t* mods, size_t modCount, uint8_t key) {
    for (size_t i = 0; i < modCount; ++i) if (mods[i]) Keyboard.press(mods[i]);
    if (key) Keyboard.press(key);
    delay(KB_COMBO_PRESS_DELAY_MS);
    if (key) Keyboard.release(key);
    for (size_t i = 0; i < modCount; ++i) {
        size_t idx = modCount - 1 - i;
        if (mods[idx]) Keyboard.release(mods[idx]);
    }
    delay(KB_COMBO_RELEASE_DELAY_MS);
}

// Determine if a char is simple (direct write) for fast path
static inline bool kbIsSimple(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == ' ';
}

static void typeText(const char* text) {
    for (int i = 0; text[i] != '\0'; ++i) {
        char c = text[i];
        // Spanish layout handling for special chars and accents
        if (!kbIsSimple(c)) {
            typeSpanishChar(c); // will route per OS
        } else {
            Keyboard.write((uint8_t)c);
        }
        delay(KB_TYPING_DELAY_MS);
    }
}

// Spanish layout dispatch
static void typeSpanishChar(char c) {
    if (current_os == OS_MAC) {
        typeSpanishCharMac(c);
    } else {
        typeSpanishCharWindows(c);
    }
}

// Common Windows (ES) mappings for frequent symbols
static void typeSpanishCharWindows(char c) {
    switch (c) {
        case '|': // AltGr + 1
            Keyboard.press(KEY_RIGHT_ALT); Keyboard.press('1'); delay(KB_COMBO_PRESS_DELAY_MS); Keyboard.releaseAll(); delay(KB_COMBO_RELEASE_DELAY_MS); break;
        case '@': // AltGr + 2
            Keyboard.press(KEY_RIGHT_ALT); Keyboard.press('2'); delay(KB_COMBO_PRESS_DELAY_MS); Keyboard.releaseAll(); delay(KB_COMBO_RELEASE_DELAY_MS); break;
        case '#': // AltGr + 3
            Keyboard.press(KEY_RIGHT_ALT); Keyboard.press('3'); delay(KB_COMBO_PRESS_DELAY_MS); Keyboard.releaseAll(); delay(KB_COMBO_RELEASE_DELAY_MS); break;
        case '~': // AltGr + 4 (approx)
            Keyboard.press(KEY_RIGHT_ALT); Keyboard.press('4'); delay(KB_COMBO_PRESS_DELAY_MS); Keyboard.releaseAll(); delay(KB_COMBO_RELEASE_DELAY_MS); break;
        case '\\': // AltGr + º
            Keyboard.press(KEY_RIGHT_ALT); Keyboard.press(0xBA); delay(KB_COMBO_PRESS_DELAY_MS); Keyboard.releaseAll(); delay(KB_COMBO_RELEASE_DELAY_MS); break;
        case '{': 
            Keyboard.press(KEY_RIGHT_ALT); Keyboard.press('['); delay(KB_COMBO_PRESS_DELAY_MS); Keyboard.releaseAll(); delay(KB_COMBO_RELEASE_DELAY_MS); break;
        case '}': 
            Keyboard.press(KEY_RIGHT_ALT); Keyboard.press(']'); delay(KB_COMBO_PRESS_DELAY_MS); Keyboard.releaseAll(); delay(KB_COMBO_RELEASE_DELAY_MS); break;
        case '[': 
            Keyboard.press(KEY_RIGHT_ALT); Keyboard.press('['); delay(KB_COMBO_PRESS_DELAY_MS); Keyboard.releaseAll(); delay(KB_COMBO_RELEASE_DELAY_MS); break; // fallback
        case ']': 
            Keyboard.press(KEY_RIGHT_ALT); Keyboard.press(']'); delay(KB_COMBO_PRESS_DELAY_MS); Keyboard.releaseAll(); delay(KB_COMBO_RELEASE_DELAY_MS); break; // fallback
        case '^': 
            Keyboard.press(KEY_RIGHT_ALT); Keyboard.press('6'); delay(KB_COMBO_PRESS_DELAY_MS); Keyboard.releaseAll(); delay(KB_COMBO_RELEASE_DELAY_MS); break; // approximate
        default:
            Keyboard.write((uint8_t)c);
            break;
    }
}

// Mac (ES-ISO) mappings for frequent symbols
static void typeSpanishCharMac(char c) {
    switch (c) {
        case '|': 
            Keyboard.press(KEY_LEFT_ALT); Keyboard.press('1'); delay(KB_COMBO_PRESS_DELAY_MS); Keyboard.releaseAll(); delay(KB_COMBO_RELEASE_DELAY_MS); break;
        case '@': 
            Keyboard.press(KEY_LEFT_ALT); Keyboard.press('2'); delay(KB_COMBO_PRESS_DELAY_MS); Keyboard.releaseAll(); delay(KB_COMBO_RELEASE_DELAY_MS); break;
        case '#': 
            Keyboard.press(KEY_LEFT_ALT); Keyboard.press('3'); delay(KB_COMBO_PRESS_DELAY_MS); Keyboard.releaseAll(); delay(KB_COMBO_RELEASE_DELAY_MS); break;
        case '$': { 
            uint8_t m[]={KEY_LEFT_SHIFT}; sendCombo(m,1,'4'); 
        } break;
        case '%': { 
            uint8_t m[]={KEY_LEFT_SHIFT}; sendCombo(m,1,'5'); 
        } break;
        case '&': { 
            uint8_t m[]={KEY_LEFT_SHIFT}; sendCombo(m,1,'6'); 
        } break;
        case '/': { 
            uint8_t m[]={KEY_LEFT_SHIFT}; sendCombo(m,1,'7'); 
        } break;
        case '(': { 
            uint8_t m[]={KEY_LEFT_SHIFT}; sendCombo(m,1,'8'); 
        } break;
        case ')': { 
            uint8_t m[]={KEY_LEFT_SHIFT}; sendCombo(m,1,'9'); 
        } break;
        case '=': { 
            uint8_t m[]={KEY_LEFT_SHIFT}; sendCombo(m,1,'0'); 
        } break;
        case '[': 
            Keyboard.press(KEY_LEFT_ALT); Keyboard.press('('); delay(KB_COMBO_PRESS_DELAY_MS); Keyboard.releaseAll(); delay(KB_COMBO_RELEASE_DELAY_MS); break;
        case ']': 
            Keyboard.press(KEY_LEFT_ALT); Keyboard.press(')'); delay(KB_COMBO_PRESS_DELAY_MS); Keyboard.releaseAll(); delay(KB_COMBO_RELEASE_DELAY_MS); break;
        case '{': { 
            uint8_t m[]={KEY_LEFT_ALT,KEY_LEFT_SHIFT}; sendCombo(m,2,'8'); 
        } break;
        case '}': { 
            uint8_t m[]={KEY_LEFT_ALT,KEY_LEFT_SHIFT}; sendCombo(m,2,'9'); 
        } break;
        case '\\': { 
            uint8_t m[]={KEY_LEFT_ALT,KEY_LEFT_SHIFT}; sendCombo(m,2,'7'); 
        } break;
        case ';': { 
            uint8_t m[]={KEY_LEFT_SHIFT}; sendCombo(m,1,','); 
        } break;
        case ':': { 
            uint8_t m[]={KEY_LEFT_SHIFT}; sendCombo(m,1,'.'); 
        } break;
        case '_': { 
            uint8_t m[]={KEY_LEFT_SHIFT}; sendCombo(m,1,'-'); 
        } break;
        case '"': { 
            uint8_t m[]={KEY_LEFT_SHIFT}; sendCombo(m,1,'2'); 
        } break;
        case '\'': 
            Keyboard.write('\''); Keyboard.write(' '); break;
        case '~': 
            Keyboard.press(KEY_LEFT_ALT); Keyboard.press('n'); delay(KB_COMBO_PRESS_DELAY_MS); Keyboard.releaseAll(); delay(KB_COMBO_RELEASE_DELAY_MS); Keyboard.write(' '); break;
        case '^': 
            Keyboard.press(KEY_LEFT_ALT); Keyboard.press('i'); delay(KB_COMBO_PRESS_DELAY_MS); Keyboard.releaseAll(); delay(KB_COMBO_RELEASE_DELAY_MS); Keyboard.write(' '); break;
        case '`': 
            Keyboard.press(KEY_LEFT_ALT); Keyboard.press('`'); delay(KB_COMBO_PRESS_DELAY_MS); Keyboard.releaseAll(); delay(KB_COMBO_RELEASE_DELAY_MS); Keyboard.write(' '); break;
        case '*': { 
            uint8_t m[]={KEY_LEFT_ALT,KEY_LEFT_SHIFT}; sendCombo(m,2,'8'); 
        } break;
        default: 
            Keyboard.write((uint8_t)c); 
            break;
    }
}

static void openRunDialog() {
    if (current_os == OS_MAC) {
        sendKeyCombination(KEY_LEFT_GUI, ' '); // Spotlight
    } else {
        sendKeyCombination(KEY_LEFT_GUI, 'r'); // Win+R
    }
    // Give UI time to appear
    delay(KB_APP_OPEN_MIN_DELAY_MS);
}

static void openSearch() {
    if (current_os == OS_MAC) {
        sendKeyCombination(KEY_LEFT_GUI, ' '); // Spotlight
    } else {
        sendKeyCombination(KEY_LEFT_GUI, 's'); // Windows Search
    }
    // Give UI time to appear
    delay(KB_APP_OPEN_MIN_DELAY_MS);
}

// Parse and send a hotkey from spec like "CTRL+SHIFT+K" or sequences "CTRL+C, CTRL+V"
static void sendHotkeyStrokeFromSegment(const String &segment) {
    uint8_t mods[5]; size_t modCount = 0; uint8_t key = 0;
    int start = 0; String token;
    while (true) {
        int plus = segment.indexOf('+', start);
        if (plus < 0) token = segment.substring(start); else token = segment.substring(start, plus);
        token.trim(); String t = token; t.toUpperCase();

        if (t == "CTRL" || t == "CONTROL") mods[modCount++] = KEY_LEFT_CTRL;
        else if (t == "SHIFT") mods[modCount++] = KEY_LEFT_SHIFT;
        else if (t == "ALT") mods[modCount++] = KEY_LEFT_ALT;
        else if (t == "ALTGR") mods[modCount++] = KEY_RIGHT_ALT;
        else if (t == "GUI" || t == "WIN" || t == "WINDOWS" || t == "COMMAND" || t == "CMD" || t == "SUPER") mods[modCount++] = KEY_LEFT_GUI;
        else if (t == "OPTION") mods[modCount++] = KEY_LEFT_ALT; // mac Option
        else if (t == "SPACE") key = ' ';
        else if (t == "ENTER" || t == "RETURN") key = KEY_RETURN;
        else if (t == "TAB") key = KEY_TAB;
        else if (t == "ESC" || t == "ESCAPE") key = KEY_ESC;
        else if (t == "BACKSPACE") key = KEY_BACKSPACE;
        else if (t == "DELETE" || t == "DEL") key = KEY_DELETE;
        else if (t == "INSERT" || t == "INS") key = KEY_INSERT;
        else if (t == "HOME") key = KEY_HOME;
        else if (t == "END") key = KEY_END;
        else if (t == "PAGEUP" || t == "PGUP") key = KEY_PAGE_UP;
        else if (t == "PAGEDOWN" || t == "PGDN") key = KEY_PAGE_DOWN;
        else if (t == "LEFT" || t == "LEFTARROW") key = KEY_LEFT_ARROW;
        else if (t == "RIGHT" || t == "RIGHTARROW") key = KEY_RIGHT_ARROW;
        else if (t == "UP" || t == "UPARROW") key = KEY_UP_ARROW;
        else if (t == "DOWN" || t == "DOWNARROW") key = KEY_DOWN_ARROW;
        else if (t.startsWith("F") && t.length() <= 3) {
            int fn = t.substring(1).toInt();
            switch (fn) {
                case 1: key = KEY_F1; break;   case 2: key = KEY_F2; break;   case 3: key = KEY_F3; break;
                case 4: key = KEY_F4; break;   case 5: key = KEY_F5; break;   case 6: key = KEY_F6; break;
                case 7: key = KEY_F7; break;   case 8: key = KEY_F8; break;   case 9: key = KEY_F9; break;
                case 10: key = KEY_F10; break; case 11: key = KEY_F11; break; case 12: key = KEY_F12; break;
                default: break;
            }
        } else if (t.length() == 1) {
            char c = t.charAt(0);
            if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a'); // normalize
            key = (uint8_t)c;
        }

        if (plus < 0) break; else start = plus + 1;
    }
    sendCombo(mods, modCount, key);
}

static void sendHotkeyFromSpec(const String &spec) {
    String s = spec;
    // Allow sequences separated by comma or the word THEN
    s.replace(" then ", ","); s.replace(" THEN ", ","); s.replace("Then", ",");
    int begin = 0;
    while (begin < (int)s.length()) {
        int comma = s.indexOf(',', begin);
        String segment = (comma < 0) ? s.substring(begin) : s.substring(begin, comma);
        segment.trim();
        if (segment.length() > 0) sendHotkeyStrokeFromSegment(segment);
        if (comma < 0) break;
        begin = comma + 1;
        delay(120); // small gap between strokes
    }
}
#endif // USE_USB_KEYBOARD_ACTIONS

// Callback handlers for manager interactions
static void onAudioChunkReady(const uint8_t* data, size_t size) {
    if (!g_streaming_active || !g_audio_chunk_queue) return;
    
    // Allocate chunk memory
    AudioChunk chunk;
    chunk.data = (uint8_t*)malloc(size);
    if (!chunk.data) {
        Serial.println("[Stream] Failed to allocate chunk memory");
        return;
    }
    
    memcpy(chunk.data, data, size);
    chunk.size = size;
    
    // Send to queue (non-blocking)
    if (xQueueSend(g_audio_chunk_queue, &chunk, 0) != pdTRUE) {
        Serial.println("[Stream] Chunk queue full, dropping chunk");
        free(chunk.data);
    } else {
        Serial.printf("[Stream] Chunk queued: %zu bytes (queue has space)\n", size);
    }
}

static void onAudioStateChanged(RecordingState state) {
    switch (state) {
        case RecordingState::Idle:
            // Don't auto-return to Ready - let main loop handle it
            ledRequest(LEDState::Off);
            break;
        case RecordingState::Recording:
            uiManager.postStateChange(UIState::Recording);
            ledRequest(LEDState::Recording);
            break;
        case RecordingState::Saving:
            uiManager.postStateChange(UIState::Processing);
            ledRequest(LEDState::Processing);
            break;
        case RecordingState::Saved:
            // Audio is ready - will be processed by main loop
            g_bench_start_ms = millis();
            break;
        case RecordingState::Error:
            uiManager.postStateChange(UIState::Error);
            ledRequest(LEDState::Error);
            break;
    }
}

// WiFi helper functions
static void wifiEnsureConnected() {
    if (WiFi.status() == WL_CONNECTED) return;

    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.setSleep(false);
    esp_wifi_set_ps(WIFI_PS_NONE);
    
    // Country/protocol optimization
    wifi_country_t country = {"ES", 1, 13, WIFI_COUNTRY_POLICY_AUTO};
    esp_wifi_set_country(&country);
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
    esp_wifi_set_bandwidth(WIFI_IF_STA, WIFI_BW_HT20);
    WiFi.setHostname("BasicGPT");

    if (WiFi.getMode() != WIFI_MODE_NULL) {
        WiFi.disconnect(true, true);
        delay(100);
    }

    // Use WiFiManager for connection with display updates
    const uint8_t maxAttempts = 2;
    for (uint8_t attempt = 1; attempt <= maxAttempts; ++attempt) {
        Serial.printf("[WiFi] Connecting (attempt %u/%u)...\n", attempt, maxAttempts);
        
        if (wifiManager.connectToWiFi()) {
            Serial.printf("[WiFi] Connected: IP=%s RSSI=%d\n", 
                         WiFi.localIP().toString().c_str(), WiFi.RSSI());
            return;
        }
        
        Serial.println("[WiFi] Connection timeout, resetting...");
        
        esp_wifi_disconnect();
        esp_wifi_stop();
        delay(250);
        esp_wifi_start();
        delay(500U << (attempt - 1));
    }
    
    Serial.println("[WiFi] Connection failed after all attempts");
}

// OpenAI query task - NOW WITH STREAMING SUPPORT
static void openaiTask(void *arg) {
    Serial.println("[OpenAI] Task started - STREAMING MODE");
    ledRequest(LEDState::Processing);
    
    // Validation checks
    if (OPENAI_API_KEY_STR.length() == 0) {
        Serial.println("[OpenAI] Error: No API key found");
        uiManager.postStatus("No API key");
        audioManager.releasePCMBuffer();
        audioManager.resetToIdle();
        uiManager.postStateChange(UIState::Error);
        ledRequest(LEDState::Error);
        g_streaming_active = false;
        g_openai_task = nullptr;
        vTaskDelete(nullptr);
        return;
    }
    
    // NOTE: Audio chunks are being streamed in real-time via queue
    // We'll still use the final buffer for the complete request
    
    // Get PCM data SAFELY
    uint8_t* pcmBuffer = audioManager.getPCMBuffer();
    size_t pcmSize = audioManager.getPCMSize();
    
    Serial.printf("[OpenAI] Got PCM buffer: %p, size: %zu\n", pcmBuffer, pcmSize);
    Serial.printf("[OpenAI] Chunks were streamed during recording for pre-processing\n");
    
    if (!pcmBuffer || pcmSize == 0) {
        Serial.println("[OpenAI] Error: No valid audio data");
        audioManager.resetToIdle();
        uiManager.postStatus("No audio");
        uiManager.postStateChange(UIState::Error);
        ledRequest(LEDState::Error);
        g_streaming_active = false;
        g_openai_task = nullptr;
        vTaskDelete(nullptr);
        return;
    }
    
    // Stop streaming flag
    g_streaming_active = false;
    
    // Drain any remaining chunks from queue
    AudioChunk chunk;
    int drainedChunks = 0;
    while (xQueueReceive(g_audio_chunk_queue, &chunk, 0) == pdTRUE) {
        free(chunk.data);
        drainedChunks++;
    }
    if (drainedChunks > 0) {
        Serial.printf("[OpenAI] Drained %d remaining chunks from queue\n", drainedChunks);
    }
    
    // Configure OpenAI client
    BasicGPTClient::Config cfg;
    cfg.host = OPENAI_HOST;
    cfg.port = OPENAI_PORT;
    cfg.apiKey = OPENAI_API_KEY_STR.c_str();
    cfg.model = OPENAI_MODEL;
    cfg.systemPrompt = SYSTEM_PROMPT;
    cfg.httpTimeoutMs = HTTP_TIMEOUT_MS;

    wifiEnsureConnected();
    
    String response;
    BasicGPTClient client(cfg);
    
    Serial.println("[OpenAI] Sending audio to OpenAI...");
    Serial.printf("[OpenAI] Conversation history: %d messages\n", g_conversation_history.size());
    
    // Use conversation history for context
    bool success = client.askAudioFromPCMWithHistory(pcmBuffer, pcmSize, 32000, 16, 1, 
                                                     g_conversation_history, response);
    Serial.printf("[OpenAI] Request completed, success: %s\n", success ? "true" : "false");
    
    // Release audio buffer AFTER processing
    Serial.println("[OpenAI] Releasing PCM buffer...");
    audioManager.releasePCMBuffer();
    audioManager.resetToIdle();
    Serial.println("[OpenAI] PCM buffer released and state reset");

    uint32_t benchElapsed = (g_bench_start_ms > 0) ? (millis() - g_bench_start_ms) : 0;
    
    if (success && response.length() > 0) {
        // Parse structured response
        ParsedResponse parsed = parseGPTResponse(response);
        
        if (!parsed.valid || parsed.displayText.length() == 0) {
            Serial.println("[OpenAI] Error: Failed to parse response");
            uiManager.postStatus("Parse error");
            uiManager.postStateChange(UIState::Error);
            ledRequest(LEDState::Error);
            delay(2000);
            uiManager.postStateChange(UIState::Ready);
            g_bench_start_ms = 0;
            g_openai_task = nullptr;
            vTaskDelete(nullptr);
            return;
        }
        
        // Normalize display text for font compatibility
        String displayText = normalizeTextForDisplay(parsed.displayText);
        
        if (benchElapsed > 0) {
            char msg[64];
            snprintf(msg, sizeof(msg), "Response (%.1fs)", benchElapsed / 1000.0f);
            Serial.printf("[Benchmark] Total time: %u ms\n", benchElapsed);
            uiManager.postStatus(msg);
        } else {
            uiManager.postStatus("");
        }
        
        // Add to conversation history (store full response for context)
        ChatMessage userMsg;
        userMsg.role = "user";
        userMsg.content = "[Audio message]";
        g_conversation_history.push_back(userMsg);
        
        ChatMessage assistantMsg;
        assistantMsg.role = "assistant";
        assistantMsg.content = response; // Store full structured response
        g_conversation_history.push_back(assistantMsg);
        
        // Limit history
        while (g_conversation_history.size() > MAX_CONVERSATION_HISTORY) {
            g_conversation_history.erase(g_conversation_history.begin());
        }
        
        Serial.printf("[Memory] Conversation history: %d messages\n", g_conversation_history.size());
        
        // Show ONLY display text on screen
        Serial.printf("[CuteAssistant] Displaying: %s\n", displayText.c_str());
        Serial.printf("[CuteAssistant] Physical Action: %s\n", parsed.action.c_str());
        
        uiManager.postStateChange(UIState::ShowingResponse);
        uiManager.postResponse(displayText.c_str());
        
        ledRequest(LEDState::Off);
        
        // Parse and execute keyboard command sequence if present
        if (parsed.action.length() > 0 && parsed.action != "none") {
            Serial.println("=== PHYSICAL ACTION ===");
            Serial.printf("ACTION: %s\n", parsed.action.c_str());
            Serial.println("=======================");
            
            // Wait 1 second to let text start appearing on screen first
            delay(1000);
            
            // Check if action contains keyboard commands
            if (parsed.action.indexOf("BEGIN") != -1 && parsed.action.indexOf("END") != -1) {
                Serial.println("[KB] Starting keyboard command sequence execution...");
                parseActionSequence(parsed.action, g_active_sequence);
            } else {
                Serial.println("[ACTION] Human-like action (no keyboard control)");
            }
        }
        
        // UIManager will automatically return to Ready after 2s (handled in UIManager update loop)
        // No blocking delay needed - user can interact immediately when ready
        
    } else {
        char msg[80];
        if (benchElapsed > 0) {
            snprintf(msg, sizeof(msg), "Error (%.1fs)", benchElapsed / 1000.0f);
        } else {
            snprintf(msg, sizeof(msg), "Assistant Error");
        }
        Serial.printf("[OpenAI] Query failed. Time=%u ms\n", benchElapsed);
        uiManager.postStatus(msg);
        uiManager.postStateChange(UIState::Error);
        ledRequest(LEDState::Error);
        
        delay(2000);
        uiManager.postStateChange(UIState::Ready);
    }
    
    g_bench_start_ms = 0;
    g_openai_task = nullptr;
    vTaskDelete(nullptr);
}

void setup() {
    Serial.begin(115200);
    Serial.println("CuteAssistant starting...");

#ifdef USE_USB_KEYBOARD_ACTIONS
    USB.begin();
    Keyboard.begin();
    delay(100);
    Serial.println("[USB] HID Keyboard initialized");
#endif

    // Initialize I2C bus FIRST (shared by display, touch, PMIC)
    Wire.begin(TOUCH_I2C_SDA, TOUCH_I2C_SCL);
    delay(100);
    
    // Initialize PMIC early to enable 5V bus for servos/peripherals
    initPMIC();

    // Initialize display and UI so user sees something immediately
    if (!display.init()) {
        Serial.println("Error: Display initialization failed");
        while (1) delay(1000);
    }

    // Initialize UI manager immediately after display
    if (!uiManager.init()) {
        Serial.println("Error: UI manager initialization failed");
        while (1) delay(1000);
    }
    
    // Show initial status immediately
    uiManager.postStateChange(UIState::Connecting);
    
    // Force display update so user sees the message
    updateDisplayNow();
    delay(100); // Brief pause to ensure rendering

    // Initialize LED manager
    LEDConfig ledConfig;
    ledConfig.pin = NEO_PIXEL_PIN;
    ledConfig.count = NEO_PIXEL_COUNT;
    ledConfig.brightness = 51; // 20% brightness
    
    if (!ledManager.init(ledConfig)) {
        Serial.println("Warning: LED manager initialization failed - continuing without LEDs");
        uiManager.postStatus("Starting...");
    } else {
        uiManager.postStatus("Starting...");
        ledManager.setState(LEDState::Processing); // Blue while system boots/connects
    }

    // Initialize audio manager
    updateDisplayNow();
    delay(50);
    
    AudioConfig audioConfig;
    audioConfig.sampleRate = 32000;
    audioConfig.bitsPerSample = 16;
    audioConfig.numChannels = 1;
    audioConfig.maxRecordingMs = 30000; // 30 seconds max, but user can release earlier
    audioConfig.sckPin = MIC_I2S_SCK;
    audioConfig.wsPin = MIC_I2S_WS;
    audioConfig.dinPin = MIC_I2S_DIN;
    
    audioManager.setStateCallback(onAudioStateChanged);
    audioManager.setChunkCallback(onAudioChunkReady);
    if (!audioManager.init(audioConfig)) {
        Serial.println("Error: Audio manager initialization failed");
        uiManager.postStatus("Audio initialization failed");
        uiManager.postStateChange(UIState::Error);
        while (1) delay(1000);
    }

    // Create LED request queue
    g_led_queue = xQueueCreate(8, sizeof(uint8_t));
    
    // Create audio chunk streaming queue
    g_audio_chunk_queue = xQueueCreate(MAX_AUDIO_CHUNKS, sizeof(AudioChunk));
    if (!g_audio_chunk_queue) {
        Serial.println("[Setup] Error: Failed to create audio chunk queue");
    } else {
        Serial.println("[Setup] Audio streaming queue created successfully");
    }

    // Initialize top button (GPIO 0) as input with pull-up
    pinMode(BUTTON_TOP, INPUT_PULLUP);
    Serial.println("[Setup] Button TOP configured on GPIO 0");

    // NOW initialize WiFi and API credentials
    updateDisplayNow();
    delay(50);
    
    // Set up progress callback for WiFi connection
    wifiManager.setProgressCallback([](const char* message) {
        // Keep "Connecting..." state
    });
    
    bool wifiOk = wifiManager.loadCredentialsFromSD();
    bool apiOk = wifiManager.loadApiKeysFromSD();
    
    if (!wifiOk) {
        Serial.println("[WiFi] Could not load credentials from SD");
    }
    if (!apiOk) {
        Serial.println("[API] Could not load OpenAI key from SD (/apis.txt)");
    }
    
    // Connect to WiFi
    updateDisplayNow();
    delay(100);
    
    wifiEnsureConnected();
    
    if (WiFi.status() == WL_CONNECTED) {
        uiManager.postStateChange(UIState::Ready);
        Serial.println("[Setup] CuteAssistant ready!");
    } else {
        uiManager.postStatus("WiFi failed");
        uiManager.postStateChange(UIState::Error);
    }
    
    if (OPENAI_API_KEY_STR.length() == 0) {
        Serial.println("[API] Warning: No API key found, OpenAI queries will fail");
        uiManager.postStatus("No API key");
    }
}

void loop() {
    // Update all managers
    display.update();
    uiManager.update();
    audioManager.service();
    ledDrainRequests();
    
    // Update GPIO command sequence state machine
    updateCommandSequence();
    
    // Touch/Button handling for recording: press to start, release to stop
    static bool wasTouching = false;
    static bool wasButtonPressed = false;
    bool isTouching = false;
    bool isButtonPressed = false;
    
    // Check current touch state
    lv_indev_t* indev = lv_indev_get_next(nullptr);
    if (indev) {
        lv_indev_state_t state = lv_indev_get_state(indev);
        isTouching = (state == LV_INDEV_STATE_PRESSED);
    }
    
    // Check top button state (active LOW with pull-up)
    isButtonPressed = (digitalRead(BUTTON_TOP) == LOW);
    
    // Combined input: either touch or button
    bool isInputActive = isTouching || isButtonPressed;
    bool wasInputActive = wasTouching || wasButtonPressed;
    
    // Detect input press edge (touch or button) - start recording
    if (isInputActive && !wasInputActive && 
        audioManager.getState() == RecordingState::Idle &&
        g_openai_task == nullptr &&
        millis() - g_last_touch_ms > TOUCH_DEBOUNCE_MS) {
        
        g_last_touch_ms = millis();
        
        // Clear chunk queue before starting
        AudioChunk chunk;
        while (xQueueReceive(g_audio_chunk_queue, &chunk, 0) == pdTRUE) {
            free(chunk.data);
        }
        
        // Enable streaming
        g_streaming_active = true;
        
        // Start recording
        uiManager.setResponse("");
        if (audioManager.startRecording()) {
            uiManager.postStatus("Listening...");
            if (isButtonPressed) {
                Serial.println("[Input] Recording started by TOP button - STREAMING ENABLED");
            } else {
                Serial.println("[Input] Recording started by touch - STREAMING ENABLED");
            }
        } else {
            uiManager.postStatus("Record failed");
            uiManager.postStateChange(UIState::Error);
            g_streaming_active = false;
        }
    }
    
    // Detect input release edge (touch or button) - stop recording
    if (!isInputActive && wasInputActive && 
        audioManager.getState() == RecordingState::Recording) {
        audioManager.stopRecording();
        uiManager.postStatus("Processing...");
        Serial.println("[Input] Recording stopped - streaming will continue until task starts");
    }
    
    wasTouching = isTouching;
    wasButtonPressed = isButtonPressed;

    // Handle audio ready for OpenAI processing (with safety delay)
    static uint32_t audioReadyTime = 0;
    if (audioManager.getState() == RecordingState::Saved && 
        g_openai_task == nullptr && 
        audioManager.getPCMBuffer() != nullptr && 
        audioManager.getPCMSize() > 0) {
        
        if (audioReadyTime == 0) {
            audioReadyTime = millis();
        } else if (millis() - audioReadyTime > 100) { // 100ms safety delay
            // Start OpenAI query task and immediately update UI
            Serial.printf("[Main] Creating OpenAI task for %zu bytes of audio\n", audioManager.getPCMSize());
            uiManager.postStateChange(UIState::Processing);
            
            BaseType_t result = xTaskCreatePinnedToCore(
                openaiTask, 
                "openai_query", 
                12288, 
                nullptr, 
                1, 
                &g_openai_task, 
                1
            );
            
            if (result != pdPASS) {
                Serial.println("[Main] Failed to create OpenAI task");
                audioManager.resetToIdle();
                uiManager.postStatus("Failed to create OpenAI task");
                uiManager.postStateChange(UIState::Error);
                ledManager.setState(LEDState::Error);
            }
            audioReadyTime = 0;
        }
    } else if (audioManager.getState() != RecordingState::Saved) {
        audioReadyTime = 0;
    }

    // WiFi connectivity watchdog
    if (millis() - g_last_wifi_check_ms > WIFI_CHECK_INTERVAL_MS) {
        g_last_wifi_check_ms = millis();
        if (WiFi.status() != WL_CONNECTED) {
            uiManager.postStatus("Reconnecting WiFi...");
            wifiEnsureConnected();
        }
    }

    delay(GUI_LOOP_DELAY_MS);
}