// lock_controller.h - Lock Controller Header
#ifndef LOCK_CONTROLLER_H
#define LOCK_CONTROLLER_H

#include <Arduino.h>
#include <ESP32Servo.h>
#include "constants.h"

class LockController {
private:
    Servo servo;
    LockState currentState;
    LockState targetState;
    bool isMoving;
    uint32_t lastActionTime;
    
public:
    LockController();
    bool init();
    bool lock();
    bool unlock();
    bool toggle();
    LockState getState();
    bool isLocked();
    bool isUnlocked();
    bool isJammed();
    void setState(LockState state);
    uint32_t getLastActionTime();
    bool isMovingNow();
};

#endif // LOCK_CONTROLLER_H
