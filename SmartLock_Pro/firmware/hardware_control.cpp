#include "include/hardware_control.h"
#include "include/config.h"
#include <driver/adc.h>

HardwareController::HardwareController() {
    currentState = STATE_UNKNOWN;
    lastUnlockTime = 0;
    unlockAttempts = 0;
    tamperDetected = false;
    doorOpenDetected = false;
    lastDoorCheck = 0;
}

void HardwareController::begin() {
    pinMode(RELAY_LOCK_PIN, OUTPUT);
    pinMode(RELAY_UNLOCK_PIN, OUTPUT);
    pinMode(RGB_LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(TAMPER_PIN, INPUT_PULLUP);
    pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP);
    
    digitalWrite(RELAY_LOCK_PIN, LOW);
    digitalWrite(RELAY_UNLOCK_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    
    // Initialize ADC for battery
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
    
    currentState = STATE_LOCKED;
    indicateBoot();
    
    DEBUG_PRINTLN("Hardware controller initialized");
}

void HardwareController::controlRelay(int pin, bool activate) {
    digitalWrite(pin, activate ? HIGH : LOW);
    delay(300);
    digitalWrite(pin, LOW);
}

void HardwareController::setRgbColor(LedColor color) {
    switch(color) {
        case LED_OFF:    setRgbRaw(0, 0, 0); break;
        case LED_RED:    setRgbRaw(255, 0, 0); break;
        case LED_GREEN:  setRgbRaw(0, 255, 0); break;
        case LED_BLUE:   setRgbRaw(0, 0, 255); break;
        case LED_YELLOW: setRgbRaw(255, 255, 0); break;
        case LED_CYAN:   setRgbRaw(0, 255, 255); break;
        case LED_MAGENTA: setRgbRaw(255, 0, 255); break;
        case LED_WHITE:  setRgbRaw(255, 255, 255); break;
    }
}

void HardwareController::setRgbRaw(uint8_t r, uint8_t g, uint8_t b) {
    // For WS2812 or similar addressable LED
    #ifdef USE_WS2812
        // WS2812 implementation
    #else
        // Simple RGB LED using PWM
        analogWrite(RGB_LED_PIN, (r + g + b) / 3);
    #endif
}

void HardwareController::playBuzzer(int duration, int frequency) {
    tone(BUZZER_PIN, frequency, duration);
    delay(duration);
    noTone(BUZZER_PIN);
}

void HardwareController::playPattern(BuzzerPattern pattern) {
    switch(pattern) {
        case BUZZ_SINGLE:
            playBuzzer(100, 1000);
            break;
        case BUZZ_DOUBLE:
            playBuzzer(100, 1000);
            delay(100);
            playBuzzer(100, 1000);
            break;
        case BUZZ_TRIPLE:
            for(int i = 0; i < 3; i++) {
                playBuzzer(80, 1000);
                delay(80);
            }
            break;
        case BUZZ_CONTINUOUS:
            for(int i = 0; i < 10; i++) {
                playBuzzer(200, 500);
                delay(100);
            }
            break;
    }
}

bool HardwareController::lockDoor() {
    if (currentState == STATE_JAMMED) {
        DEBUG_PRINTLN("Door jammed, cannot lock");
        return false;
    }
    
    controlRelay(RELAY_LOCK_PIN, true);
    currentState = STATE_LOCKED;
    playPattern(BUZZ_SINGLE);
    setRgbColor(LED_RED);
    
    DEBUG_PRINTLN("Door locked");
    return true;
}

bool HardwareController::unlockDoor() {
    if (unlockAttempts >= MAX_UNLOCK_ATTEMPTS) {
        DEBUG_PRINTLN("Max unlock attempts reached");
        indicateWarning();
        return false;
    }
    
    if (currentState == STATE_JAMMED) {
        DEBUG_PRINTLN("Door jammed, cannot unlock");
        return false;
    }
    
    controlRelay(RELAY_UNLOCK_PIN, true);
    currentState = STATE_UNLOCKED;
    lastUnlockTime = millis();
    playPattern(BUZZ_DOUBLE);
    setRgbColor(LED_GREEN);
    
    DEBUG_PRINTLN("Door unlocked");
    return true;
}

bool HardwareController::isDoorClosed() {
    return digitalRead(DOOR_SENSOR_PIN) == HIGH;
}

LockState HardwareController::getState() {
    checkDoorSensor();
    return currentState;
}

String HardwareController::getStateString() {
    switch(currentState) {
        case STATE_LOCKED: return "locked";
        case STATE_UNLOCKED: return "unlocked";
        case STATE_JAMMED: return "jammed";
        case STATE_TAMPERED: return "tampered";
        default: return "unknown";
    }
}

bool HardwareController::isAutoLockNeeded() {
    if (currentState == STATE_UNLOCKED) {
        unsigned long elapsed = millis() - lastUnlockTime;
        if (elapsed >= AUTO_LOCK_DELAY_MS) {
            DEBUG_PRINTLN("Auto-lock triggered");
            return true;
        }
    }
    return false;
}

void HardwareController::resetAttempts() {
    unlockAttempts = 0;
    DEBUG_PRINTLN("Unlock attempts reset");
}

void HardwareController::incrementAttempts() {
    unlockAttempts++;
    DEBUG_PRINTF("Unlock attempts: %d\n", unlockAttempts);
}

int HardwareController::getAttempts() {
    return unlockAttempts;
}

bool HardwareController::checkTamper() {
    if (digitalRead(TAMPER_PIN) == LOW) {
        tamperDetected = true;
        currentState = STATE_TAMPERED;
        DEBUG_PRINTLN("TAMPER DETECTED!");
        indicateWarning();
        return true;
    }
    return false;
}

void HardwareController::resetTamper() {
    tamperDetected = false;
    if (currentState == STATE_TAMPERED) {
        currentState = STATE_LOCKED;
    }
    DEBUG_PRINTLN("Tamper reset");
}

void HardwareController::indicateSuccess() {
    setRgbColor(LED_GREEN);
    playPattern(BUZZ_SINGLE);
    delay(300);
    setRgbColor(LED_OFF);
}

void HardwareController::indicateFailure() {
    setRgbColor(LED_RED);
    playPattern(BUZZ_TRIPLE);
    delay(500);
    setRgbColor(LED_OFF);
}

void HardwareController::indicateBusy() {
    setRgbColor(LED_YELLOW);
    playPattern(BUZZ_DOUBLE);
    delay(200);
    setRgbColor(LED_OFF);
}

void HardwareController::indicateWarning() {
    for(int i = 0; i < 5; i++) {
        setRgbColor(LED_RED);
        delay(100);
        setRgbColor(LED_OFF);
        delay(100);
    }
    playPattern(BUZZ_CONTINUOUS);
}

void HardwareController::indicateBoot() {
    for(int i = 0; i < 3; i++) {
        setRgbColor(LED_BLUE);
        delay(100);
        setRgbColor(LED_OFF);
        delay(100);
    }
    playPattern(BUZZ_DOUBLE);
}

BatteryInfo HardwareController::readBattery() {
    BatteryInfo info;
    
    int raw = analogRead(BATTERY_PIN);
    float voltage = (raw / 4095.0) * 3.3 * 4.0;  // Voltage divider: 3.3V * (R1+R2)/R2
    
    info.voltage = voltage;
    
    // Battery percentage for 12V lead-acid
    if (voltage >= 12.6) info.percentage = 100;
    else if (voltage <= 10.5) info.percentage = 0;
    else info.percentage = (int)((voltage - 10.5) / 2.1 * 100);
    
    info.low = voltage < 11.0;
    info.critical = voltage < 10.5;
    
    return info;
}

void HardwareController::emergencyLock() {
    lockDoor();
    unlockAttempts = MAX_UNLOCK_ATTEMPTS;
    DEBUG_PRINTLN("Emergency lock activated");
}

void HardwareController::checkDoorSensor() {
    if (millis() - lastDoorCheck > 1000) {
        lastDoorCheck = millis();
        bool doorClosed = isDoorClosed();
        
        if (!doorClosed && currentState == STATE_UNLOCKED) {
            doorOpenDetected = true;
        } else if (doorClosed && doorOpenDetected) {
            doorOpenDetected = false;
            if (currentState == STATE_UNLOCKED) {
                lockDoor();
                DEBUG_PRINTLN("Door closed, auto-locking");
            }
        }
    }
}

void HardwareController::calibrate() {
    DEBUG_PRINTLN("Calibrating sensors...");
    // Calibration logic
}

void HardwareController::selfTest() {
    DEBUG_PRINTLN("Running self-test...");
    
    // Test LED
    setRgbColor(LED_RED);
    delay(200);
    setRgbColor(LED_GREEN);
    delay(200);
    setRgbColor(LED_BLUE);
    delay(200);
    setRgbColor(LED_OFF);
    
    // Test buzzer
    playPattern(BUZZ_SINGLE);
    
    // Test relays
    controlRelay(RELAY_LOCK_PIN, true);
    controlRelay(RELAY_UNLOCK_PIN, true);
    
    // Check sensors
    BatteryInfo batt = readBattery();
    DEBUG_PRINTF("Battery: %.2fV (%d%%)\n", batt.voltage, batt.percentage);
    
    DEBUG_PRINTLN("Self-test complete");
}

bool HardwareController::isJammed() {
    return currentState == STATE_JAMMED;
}
