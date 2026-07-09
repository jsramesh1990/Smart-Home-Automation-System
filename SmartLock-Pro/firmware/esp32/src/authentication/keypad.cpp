// keypad.cpp - Keypad Authentication Implementation
#include "keypad.h"
#include "pins.h"
#include "constants.h"
#include "logger.h"
#include <Keypad.h>

extern Logger logger;

// Keypad layout
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {PIN_KEYPAD_R1, PIN_KEYPAD_R2, PIN_KEYPAD_R3, PIN_KEYPAD_R4};
byte colPins[COLS] = {PIN_KEYPAD_C1, PIN_KEYPAD_C2, PIN_KEYPAD_C3, PIN_KEYPAD_C4};

KeypadAuth::KeypadAuth() {
    keypad = nullptr;
    initialized = false;
    pinBuffer = "";
    lastKeyTime = 0;
}

bool KeypadAuth::init() {
    logger.log(LOG_LEVEL_INFO, "Keypad", "Initializing...");
    
    keypad = new Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
    
    if (keypad) {
        initialized = true;
        logger.log(LOG_LEVEL_INFO, "Keypad", "Initialized");
        return true;
    }
    
    return false;
}

String KeypadAuth::readInput() {
    if (!initialized || !keypad) {
        return "";
    }
    
    char key = keypad->getKey();
    
    if (key) {
        lastKeyTime = millis();
        
        if (key == '#') {
            // Enter key - submit PIN
            String pin = pinBuffer;
            pinBuffer = "";
            return pin;
        } else if (key == '*') {
            // Clear key
            pinBuffer = "";
            return "";
        } else {
            // Regular key - add to buffer
            pinBuffer += key;
            return "";
        }
    }
    
    // Timeout - clear buffer if no activity
    if (millis() - lastKeyTime > 5000 && pinBuffer.length() > 0) {
        pinBuffer = "";
    }
    
    return "";
}

bool KeypadAuth::authenticate(const char* pin) {
    // Check against stored PINs
    for (auto& user : pinDatabase) {
        if (strcmp(user.first, pin) == 0) {
            return true;
        }
    }
    return false;
}

bool KeypadAuth::setPIN(uint16_t userId, const char* pin) {
    // Store PIN in database
    pinDatabase[pin] = userId;
    return true;
}

bool KeypadAuth::removePIN(const char* pin) {
    if (pinDatabase.find(pin) != pinDatabase.end()) {
        pinDatabase.erase(pin);
        return true;
    }
    return false;
}

uint16_t KeypadAuth::getUserByPIN(const char* pin) {
    if (pinDatabase.find(pin) != pinDatabase.end()) {
        return pinDatabase[pin];
    }
    return 0;
}
