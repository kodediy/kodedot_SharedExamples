/*
 * DuckyScript Interpreter Implementation
 */

#include "DuckyScript.h"
#include <cstring>
#include <cctype>

DuckyScript::DuckyScript() : initialized(false) {}

void DuckyScript::begin() {
    keyboard.begin();
    USB.begin();
    initialized = true;
    delay(1000); // Wait for USB enumeration
}

void DuckyScript::executeLine(const char* line) {
    if (!initialized || !line || strlen(line) == 0) return;
    
    // Skip whitespace
    while (*line && isspace(*line)) line++;
    if (*line == '\0') return;
    
    // Parse command
    if (strncmp(line, "REM ", 4) == 0) {
        // Comment - do nothing
        return;
    }
    else if (strncmp(line, "DELAY ", 6) == 0) {
        int ms = atoi(line + 6);
        delay(ms);
    }
    else if (strncmp(line, "STRING ", 7) == 0) {
        typeString(line + 7);
    }
    else if (strcmp(line, "ENTER") == 0) {
        keyboard.press(KEY_RETURN);
        keyboard.releaseAll();
    }
    else if (strncmp(line, "GUI ", 4) == 0) {
        const char* keyStr = line + 4;
        // Skip whitespace
        while (*keyStr && isspace(*keyStr)) keyStr++;
        
        if (strcmp(keyStr, "SPACE") == 0) {
            keyboard.press(KEY_LEFT_GUI);
            keyboard.press(' ');
            keyboard.releaseAll();
        } else {
            char key = parseKey(keyStr);
            if (key) {
                keyboard.press(KEY_LEFT_GUI);
                keyboard.press(key);
                keyboard.releaseAll();
            }
        }
    }
    else if (strncmp(line, "CTRL-ALT ", 9) == 0) {
        char key = parseKey(line + 9);
        if (key) {
            keyboard.press(KEY_LEFT_CTRL);
            keyboard.press(KEY_LEFT_ALT);
            keyboard.press(key);
            keyboard.releaseAll();
        }
    }
    else if (strncmp(line, "CTRL-SHIFT ", 11) == 0) {
        char key = parseKey(line + 11);
        if (key) {
            keyboard.press(KEY_LEFT_CTRL);
            keyboard.press(KEY_LEFT_SHIFT);
            keyboard.press(key);
            keyboard.releaseAll();
        }
    }
    else if (strncmp(line, "CTRL ", 5) == 0) {
        char key = parseKey(line + 5);
        if (key) {
            keyboard.press(KEY_LEFT_CTRL);
            keyboard.press(key);
            keyboard.releaseAll();
        }
    }
    else if (strncmp(line, "ALT ", 4) == 0) {
        char key = parseKey(line + 4);
        if (key) {
            keyboard.press(KEY_LEFT_ALT);
            keyboard.press(key);
            keyboard.releaseAll();
        }
    }
    else if (strncmp(line, "SHIFT ", 6) == 0) {
        char key = parseKey(line + 6);
        if (key) {
            keyboard.press(KEY_LEFT_SHIFT);
            keyboard.press(key);
            keyboard.releaseAll();
        }
    }
    else if (strcmp(line, "TAB") == 0) {
        keyboard.press(KEY_TAB);
        keyboard.releaseAll();
    }
    else if (strcmp(line, "SPACE") == 0) {
        keyboard.press(' ');
        keyboard.releaseAll();
    }
    else if (strcmp(line, "ESCAPE") == 0 || strcmp(line, "ESC") == 0) {
        keyboard.press(KEY_ESC);
        keyboard.releaseAll();
    }
    else if (strcmp(line, "DELETE") == 0) {
        keyboard.press(KEY_DELETE);
        keyboard.releaseAll();
    }
    else if (strcmp(line, "BACKSPACE") == 0) {
        keyboard.press(KEY_BACKSPACE);
        keyboard.releaseAll();
    }
    else if (strcmp(line, "UPARROW") == 0) {
        keyboard.press(KEY_UP_ARROW);
        keyboard.releaseAll();
    }
    else if (strcmp(line, "DOWNARROW") == 0) {
        keyboard.press(KEY_DOWN_ARROW);
        keyboard.releaseAll();
    }
    else if (strcmp(line, "LEFTARROW") == 0) {
        keyboard.press(KEY_LEFT_ARROW);
        keyboard.releaseAll();
    }
    else if (strcmp(line, "RIGHTARROW") == 0) {
        keyboard.press(KEY_RIGHT_ARROW);
        keyboard.releaseAll();
    }
    
    delay(10); // Small delay between commands
}

void DuckyScript::executeScript(const char* script) {
    if (!script) return;
    
    char buffer[256];
    const char* ptr = script;
    int idx = 0;
    
    while (*ptr) {
        if (*ptr == '\n' || *ptr == '\r') {
            if (idx > 0) {
                buffer[idx] = '\0';
                executeLine(buffer);
                idx = 0;
            }
            ptr++;
            continue;
        }
        
        if (idx < 255) {
            buffer[idx++] = *ptr;
        }
        ptr++;
    }
    
    // Execute last line if no trailing newline
    if (idx > 0) {
        buffer[idx] = '\0';
        executeLine(buffer);
    }
}

void DuckyScript::typeString(const char* text) {
    if (!text) return;
    while (*text) {
        keyboard.write(*text);
        text++;
        delay(5); // Typing delay for reliability
    }
}

char DuckyScript::parseKey(const char* str) {
    while (*str && isspace(*str)) str++;
    if (*str) {
        // Convert to lowercase for consistency
        return tolower(*str);
    }
    return 0;
}
