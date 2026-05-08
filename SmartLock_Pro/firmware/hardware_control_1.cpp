#include "include/hardware_control.h"
#include "include/config.h"

HardwareController::HardwareController() {
    currentState = UNKNOWN;
    lastUnlockTime = 0;
    unlockAttempts = 0;
    tamperDetected = false;
}

void HardwareController::begin() {
    pinMode(RELAY_LOCK_PIN, OUTPUT);
    pinMode(RELAY_UNLOCK_PIN, OUTPUT);
    pinMode(RGB_LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(TAMPER_PIN, INPUT_PULLUP);
    pinMode(BATTERY_PIN, INPUT);
    
    digitalWrite(RELAY_LOCK_PIN, LOW);
    digitalWrite(RELAY_UNLOCK_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    
    currentState = LOCKED;
    indicateSuccess();
}

void HardwareController::controlRelay(int pin, bool activate) {
    digitalWrite(pin, activate ? HIGH : LOW);
    delay(500);  // Keep relay active for 500ms
    digitalWrite(pin, LOW);
}

bool HardwareController::lockDoor() {
    controlRelay(RELAY_LOCK_PIN, true);
    currentState = LOCKED;
    playBuzzer(100, 1000);
    setRgbColor(LED_RED);
    return true;
}

bool HardwareController::unlockDoor() {
    if (unlockAttempts >= MAX_UNLOCK_ATTEMPTS) {
        indicateFailure();
        return false;
    }
    
    controlRelay(RELAY_UNLOCK_PIN, true);
    currentState = UNLOCKED;
    lastUnlockTime = millis();
    playBuzzer(200, 1500);
    setRgbColor(LED_GREEN);
    return true;
}

LockState HardwareController::getState() {
    return currentState;
}

bool HardwareController::isAutoLockNeeded() {
    if (currentState == UNLOCKED && (millis() - lastUnlockTime) >= AUTO_LOCK_DELAY_MS) {
        return true;
    }
    return false;
}

void HardwareController::resetAttempts() {
    unlockAttempts = 0;
}

void HardwareController::incrementAttempts() {
    unlockAttempts++;
}

int HardwareController::getAttempts() {
    return unlockAttempts;
}

bool HardwareController::checkTamper() {
    if (digitalRead(TAMPER_PIN) == LOW) {
        tamperDetected = true;
        return true;
    }
    return false;
}

void HardwareController::setRgbColor(LedColor color) {
    // WS2812 or similar RGB LED control
    switch(color) {
        case LED_OFF:
            // Send off signal
            break;
        case LED_RED:
            // Send red signal
            break;
        case LED_GREEN:
            // Send green signal
            break;
        case LED_BLUE:
            // Send blue signal
            break;
        default:
            break;
    }
}

void HardwareController::playBuzzer(int duration, int frequency) {
    tone(BUZZER_PIN, frequency, duration);
    delay(duration);
    noTone(BUZZER_PIN);
}

void HardwareController::indicateSuccess() {
    playBuzzer(100, 2000);
    setRgbColor(LED_GREEN);
    delay(500);
    setRgbColor(LED_OFF);
}

void HardwareController::indicateFailure() {
    for (int i = 0; i < 3; i++) {
        playBuzzer(100, 500);
        delay(100);
    }
    setRgbColor(LED_RED);
    delay(500);
    setRgbColor(LED_OFF);
}

void HardwareController::indicateBusy() {
    for (int i = 0; i < 2; i++) {
        playBuzzer(50, 1000);
        delay(50);
    }
    setRgbColor(LED_YELLOW);
}

float HardwareController::readBatteryVoltage() {
    int raw = analogRead(BATTERY_PIN);
    return (raw / 4095.0) * 3.3 * 4.0;  // Assuming voltage divider
}

int HardwareController::getBatteryPercentage() {
    float voltage = readBatteryVoltage();
    // Assuming 12V is 100%, 10.5V is 0%
    float percentage = ((voltage - 10.5) / 1.5) * 100;
    if (percentage > 100) percentage = 100;
    if (percentage < 0) percentage = 0;
    return (int)percentage;
}
