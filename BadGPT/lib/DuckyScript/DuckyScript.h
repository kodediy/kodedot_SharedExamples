/*
 * DuckyScript Interpreter for ESP32-S3 USB Rubber Ducky
 * ------------------------------------------------------------
 * Implements a subset of DuckyScript commands for keyboard emulation
 * 
 * Supported Commands:
 *  - REM <comment>         : Comment (ignored)
 *  - DELAY <ms>            : Wait milliseconds
 *  - STRING <text>         : Type text as keyboard
 *  - ENTER                 : Press Enter key
 *  - GUI <key>             : Windows/Command key + key (e.g., GUI r)
 *  - CTRL <key>            : Control + key
 *  - ALT <key>             : Alt + key
 *  - CTRL-ALT <key>        : Ctrl + Alt + key
 *  - CTRL-SHIFT <key>      : Ctrl + Shift + key
 *  - SHIFT <key>           : Shift + key
 *  - TAB                   : Tab key
 *  - SPACE                 : Space key
 *  - ESCAPE                : Escape key
 *  - DELETE                : Delete key
 *  - BACKSPACE             : Backspace key
 *  - UPARROW               : Up arrow
 *  - DOWNARROW             : Down arrow
 *  - LEFTARROW             : Left arrow
 *  - RIGHTARROW            : Right arrow
 */

#ifndef DUCKYSCRIPT_H
#define DUCKYSCRIPT_H

#include <Arduino.h>
#include <USB.h>
#include <USBHIDKeyboard.h>

class DuckyScript {
public:
    DuckyScript();
    void begin();
    void executeLine(const char* line);
    void executeScript(const char* script);
    
private:
    USBHIDKeyboard keyboard;
    bool initialized;
    
    void typeString(const char* text);
    void pressKey(uint8_t key, uint8_t modifiers = 0);
    void pressSpecialKey(uint8_t key);
    
    // Helper to parse single character after command
    char parseKey(const char* str);
};

#endif // DUCKYSCRIPT_H
