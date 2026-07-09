// lock_controller.cpp - Lock Controller Implementation
#include "lock_controller.h"
#include "pins.h"
#include "constants.h"
#include "logger.h"
#include <ESP32Servo.h>

extern Logger logger;

LockController::LockController() {
    currentState = LOCK_UNKNOWN;
    targetState = LOCK_UNKNOWN;
    isMoving = false;
    lastActionTime = 0;
}

bool LockController::init() {
    logger.log(LOG_LEVEL_INFO, "Lock", "Initializing...");
    
    // Attach servo
    servo.attach(PIN_SERVO, SERVO_MIN_PULSE, SERVO_MAX_PULSE);
    
    // Set initial position
    servo.write(LOCK_ANGLE);
    delay(500);
    
    currentState = LOCK_LOCKED;
    targetState = LOCK_LOCKED;
    isMoving = false;
    
    logger.log(LOG_LEVEL_INFO, "Lock", "Initialized in LOCKED state");
    return true;
}

bool LockController::lock() {
    if (currentState == LOCK_LOCKED) {
        logger.log(LOG_LEVEL_DEBUG, "Lock", "Already locked");
        return true;
    }
    
    if (isMoving) {
        logger.log(LOG_LEVEL_WARNING, "Lock", "Already moving");
        return false;
    }
    
    logger.log(LOG_LEVEL_INFO, "Lock", "Locking...");
    
    targetState = LOCK_LOCKED;
    isMoving = true;
    
    // Move servo to lock position
    for (int pos = UNLOCK_ANGLE; pos >= LOCK_ANGLE; pos -= 2) {
        servo.write(pos);
        delay(10);
    }
    
    // Verify lock position
    delay(100);
    if (servo.read() == LOCK_ANGLE) {
        currentState = LOCK_LOCKED;
        isMoving = false;
        lastActionTime = millis();
        logger.log(LOG_LEVEL_INFO, "Lock", "Locked successfully");
        
        // Publish status
        return true;
    }
    
    // If not at lock angle, try again
    servo.write(LOCK_ANGLE);
    delay(200);
    currentState = LOCK_LOCKED;
    isMoving = false;
    lastActionTime = millis();
    
    logger.log(LOG_LEVEL_INFO, "Lock", "Locked (with retry)");
    return true;
}

bool LockController::unlock() {
    if (currentState == LOCK_UNLOCKED) {
        logger.log(LOG_LEVEL_DEBUG, "Lock", "Already unlocked");
        return true;
    }
    
    if (isMoving) {
        logger.log(LOG_LEVEL_WARNING, "Lock", "Already moving");
        return false;
    }
    
    logger.log(LOG_LEVEL_INFO, "Lock", "Unlocking...");
    
    targetState = LOCK_UNLOCKED;
    isMoving = true;
    
    // Move servo to unlock position
    for (int pos = LOCK_ANGLE; pos <= UNLOCK_ANGLE; pos += 2) {
        servo.write(pos);
        delay(10);
    }
    
    // Verify unlock position
    delay(100);
    if (servo.read() == UNLOCK_ANGLE) {
        currentState = LOCK_UNLOCKED;
        isMoving = false;
        lastActionTime = millis();
        logger.log(LOG_LEVEL_INFO, "Lock", "Unlocked successfully");
        return true;
    }
    
    // If not at unlock angle, try again
    servo.write(UNLOCK_ANGLE);
    delay(200);
    currentState = LOCK_UNLOCKED;
    isMoving = false;
    lastActionTime = millis();
    
    logger.log(LOG_LEVEL_INFO, "Lock", "Unlocked (with retry)");
    return true;
}

bool LockController::toggle() {
    if (currentState == LOCK_LOCKED) {
        return unlock();
    } else {
        return lock();
    }
}

LockState LockController::getState() {
    return currentState;
}

bool LockController::isLocked() {
    return currentState == LOCK_LOCKED;
}

bool LockController::isUnlocked() {
    return currentState == LOCK_UNLOCKED;
}

bool LockController::isJammed() {
    return currentState == LOCK_JAMMED;
}

void LockController::setState(LockState state) {
    currentState = state;
}

uint32_t LockController::getLastActionTime() {
    return lastActionTime;
}

bool LockController::isMovingNow() {
    return isMoving;
}
