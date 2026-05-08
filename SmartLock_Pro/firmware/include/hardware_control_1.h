#ifndef HARDWARE_CONTROL_H
#define HARDWARE_CONTROL_H

#include <Arduino.h>

enum LockState {
    LOCKED = 0,
    UNLOCKED = 1,
    UNKNOWN = 2
};

enum LedColor {
    LED_OFF = 0,
    LED_RED = 1,
    LED_GREEN = 2,
    LED_BLUE = 3,
    LED_YELLOW = 4,
    LED_CYAN = 5,
    LED_MAGENTA = 6
};

class HardwareController {
private:
    LockState currentState;
    unsigned long lastUnlockTime;
    int unlockAttempts;
    bool tamperDetected;
    
    void controlRelay(int pin, bool activate);
    void setRgbColor(LedColor color);
    void playBuzzer(int duration, int frequency);
    
public:
    HardwareController();
    void begin();
    bool lockDoor();
    bool unlockDoor();
    LockState getState();
    bool isAutoLockNeeded();
    void resetAttempts();
    void incrementAttempts();
    int getAttempts();
    bool checkTamper();
    void indicateSuccess();
    void indicateFailure();
    void indicateBusy();
    float readBatteryVoltage();
    int getBatteryPercentage();
};

#endif
