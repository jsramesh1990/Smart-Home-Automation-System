// keypad.h - Keypad Authentication Header
#ifndef KEYPAD_H
#define KEYPAD_H

#include <Arduino.h>
#include <Keypad.h>
#include <map>

class KeypadAuth {
private:
    Keypad* keypad;
    bool initialized;
    String pinBuffer;
    unsigned long lastKeyTime;
    std::map<String, uint16_t> pinDatabase;
    
public:
    KeypadAuth();
    bool init();
    String readInput();
    bool authenticate(const char* pin);
    bool setPIN(uint16_t userId, const char* pin);
    bool removePIN(const char* pin);
    uint16_t getUserByPIN(const char* pin);
};

#endif // KEYPAD_H
