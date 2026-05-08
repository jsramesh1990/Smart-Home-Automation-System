#ifndef HARDWARE_CONTROL_H
#define HARDWARE_CONTROL_H

#include <Arduino.h>

enum LockState {
    STATE_LOCKED = 0,
    STATE_UNLOCKED = 1,
    STATE_UNKNOWN = 2,
    STATE_JAMMED = 3,
    STATE_TAMPERED = 4
};

enum LedColor {
    LED_OFF = 0,
    LED_RED = 1,
    LED_GREEN = 2,
    LED_BLUE = 3,
    LED_YELLOW = 4,
    LED_CYAN = 5,
    LED_MAGENTA = 6,
    LED_WHITE = 7
};

enum BuzzerPattern {
    BUZZ_SINGLE = 0,
    BUZZ_DOUBLE = 1,
    BUZZ_TRIPLE = 2,
    BUZZ_CONTINUOUS = 3
};

struct BatteryInfo {
    float voltage;
    int percentage;
    bool low;
    bool critical;
};

class HardwareController {
private:
    LockState currentState;
    unsigned long lastUnlockTime;
    int unlockAttempts;
    bool tamperDetected;
    bool doorOpenDetected;
    unsigned long lastDoorCheck;
    
    void controlRelay(int pin, bool activate);
    void setRgbColor(LedColor color);
    void setRgbRaw(uint8_t r, uint8_t g, uint8_t b);
    void playBuzzer(int duration, int frequency);
    void playPattern(BuzzerPattern pattern);
    void checkDoorSensor();
    
public:
    HardwareController();
    void begin();
    bool lockDoor();
    bool unlockDoor();
    bool isDoorClosed();
    LockState getState();
    String getStateString();
    bool isAutoLockNeeded();
    void resetAttempts();
    void incrementAttempts();
    int getAttempts();
    bool checkTamper();
    void resetTamper();
    void indicateSuccess();
    void indicateFailure();
    void indicateBusy();
    void indicateWarning();
    void indicateBoot();
    BatteryInfo readBattery();
    void emergencyLock();
    void calibrate();
    void selfTest();
    bool isJammed();
};

#endif
