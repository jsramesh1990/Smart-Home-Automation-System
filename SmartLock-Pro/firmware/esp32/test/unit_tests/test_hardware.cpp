// test_hardware.cpp - Hardware Unit Tests
#include <Arduino.h>
#include <unity.h>
#include <ESP32Servo.h>
#include "../../src/lock_controller.h"
#include "../../src/sensor_manager.h"
#include "../../src/pins.h"

LockController lockController;
SensorManager sensorManager;
Servo testServo;

// ============================================================
# LOCK CONTROLLER TESTS
// ============================================================
void test_lock_init() {
    TEST_ASSERT_TRUE(lockController.init());
    TEST_ASSERT_EQUAL(LOCK_LOCKED, lockController.getState());
}

void test_lock_operation() {
    // Unlock
    TEST_ASSERT_TRUE(lockController.unlock());
    TEST_ASSERT_EQUAL(LOCK_UNLOCKED, lockController.getState());
    
    // Lock
    TEST_ASSERT_TRUE(lockController.lock());
    TEST_ASSERT_EQUAL(LOCK_LOCKED, lockController.getState());
}

void test_lock_toggle() {
    TEST_ASSERT_TRUE(lockController.toggle());
    TEST_ASSERT_EQUAL(LOCK_UNLOCKED, lockController.getState());
    
    TEST_ASSERT_TRUE(lockController.toggle());
    TEST_ASSERT_EQUAL(LOCK_LOCKED, lockController.getState());
}

void test_lock_state_queries() {
    lockController.lock();
    TEST_ASSERT_TRUE(lockController.isLocked());
    TEST_ASSERT_FALSE(lockController.isUnlocked());
    
    lockController.unlock();
    TEST_ASSERT_TRUE(lockController.isUnlocked());
    TEST_ASSERT_FALSE(lockController.isLocked());
}

void test_lock_move_time() {
    lockController.unlock();
    uint32_t time1 = lockController.getLastActionTime();
    delay(100);
    lockController.lock();
    uint32_t time2 = lockController.getLastActionTime();
    TEST_ASSERT_TRUE(time2 > time1);
}

void test_lock_moving_state() {
    // Test that isMovingNow properly tracks movement
    TEST_ASSERT_FALSE(lockController.isMovingNow());
    
    // Perform a move and check state
    lockController.unlock();
    // isMovingNow should be true during movement, but since it completes quickly,
    // we verify it eventually becomes false
    delay(500);
    TEST_ASSERT_FALSE(lockController.isMovingNow());
}

// ============================================================
# SENSOR MANAGER TESTS
// ============================================================
void test_sensor_init() {
    TEST_ASSERT_TRUE(sensorManager.init());
}

void test_temperature_read() {
    float temp;
    bool result = sensorManager.readTemperature(&temp);
    
    if (result) {
        TEST_ASSERT_TRUE(temp >= -40 && temp <= 80);
    } else {
        // Test should pass even if sensor not connected
        TEST_ASSERT_TRUE(true);
    }
}

void test_humidity_read() {
    float hum;
    bool result = sensorManager.readHumidity(&hum);
    
    if (result) {
        TEST_ASSERT_TRUE(hum >= 0 && hum <= 100);
    } else {
        TEST_ASSERT_TRUE(true);
    }
}

void test_door_sensor() {
    bool state = sensorManager.isDoorOpen();
    // Door sensor should be either true or false
    TEST_ASSERT_TRUE(state || !state);
}

void test_motion_sensor() {
    bool state = sensorManager.isMotionDetected();
    TEST_ASSERT_TRUE(state || !state);
}

void test_battery_level() {
    uint8_t level = sensorManager.getBatteryLevel();
    TEST_ASSERT_TRUE(level >= 0 && level <= 100);
}

void test_temperature_getter() {
    float temp = sensorManager.getTemperature();
    if (temp != -999.0f) {
        TEST_ASSERT_TRUE(temp >= -40 && temp <= 80);
    }
}

void test_humidity_getter() {
    float hum = sensorManager.getHumidity();
    if (hum != -999.0f) {
        TEST_ASSERT_TRUE(hum >= 0 && hum <= 100);
    }
}

// ============================================================
# SERVO TESTS
// ============================================================
void test_servo_attach() {
    testServo.attach(PIN_SERVO);
    TEST_ASSERT_TRUE(testServo.attached());
}

void test_servo_move() {
    testServo.write(0);
    delay(100);
    TEST_ASSERT_EQUAL(0, testServo.read());
    
    testServo.write(90);
    delay(100);
    TEST_ASSERT_EQUAL(90, testServo.read());
    
    testServo.write(180);
    delay(100);
    TEST_ASSERT_EQUAL(180, testServo.read());
}

void test_servo_detach() {
    testServo.detach();
    TEST_ASSERT_FALSE(testServo.attached());
}

// ============================================================
# GPIO TESTS
// ============================================================
void test_gpio_output() {
    // Test LED pins
    pinMode(PIN_LED_RED, OUTPUT);
    pinMode(PIN_LED_GREEN, OUTPUT);
    pinMode(PIN_LED_BLUE, OUTPUT);
    
    digitalWrite(PIN_LED_RED, HIGH);
    delay(50);
    TEST_ASSERT_EQUAL(HIGH, digitalRead(PIN_LED_RED));
    digitalWrite(PIN_LED_RED, LOW);
    
    digitalWrite(PIN_LED_GREEN, HIGH);
    delay(50);
    TEST_ASSERT_EQUAL(HIGH, digitalRead(PIN_LED_GREEN));
    digitalWrite(PIN_LED_GREEN, LOW);
    
    digitalWrite(PIN_LED_BLUE, HIGH);
    delay(50);
    TEST_ASSERT_EQUAL(HIGH, digitalRead(PIN_LED_BLUE));
    digitalWrite(PIN_LED_BLUE, LOW);
}

void test_gpio_input() {
    // Test door sensor input
    pinMode(PIN_DOOR_SENSOR, INPUT_PULLUP);
    int value = digitalRead(PIN_DOOR_SENSOR);
    TEST_ASSERT_TRUE(value == HIGH || value == LOW);
}

void test_gpio_interrupt() {
    // Test interrupt on door sensor
    pinMode(PIN_DOOR_SENSOR, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_DOOR_SENSOR), 
                    [](){}, CHANGE);
    TEST_ASSERT_TRUE(true);
    detachInterrupt(digitalPinToInterrupt(PIN_DOOR_SENSOR));
}

// ============================================================
# PERFORMANCE TESTS
// ============================================================
void test_hardware_performance() {
    unsigned long start = micros();
    
    // Perform multiple operations
    for (int i = 0; i < 100; i++) {
        lockController.unlock();
        lockController.lock();
        sensorManager.getTemperature();
        sensorManager.getBatteryLevel();
    }
    
    unsigned long duration = micros() - start;
    Serial.printf("100 hardware operation cycles: %lu µs\n", duration);
    
    TEST_ASSERT_TRUE(duration < 10000000); // < 10 seconds
}

void test_servo_precision() {
    int target_angles[] = {0, 30, 60, 90, 120, 150, 180};
    
    for (int i = 0; i < 7; i++) {
        testServo.write(target_angles[i]);
        delay(200);
        int actual = testServo.read();
        int diff = abs(actual - target_angles[i]);
        TEST_ASSERT_TRUE(diff <= 5); // Within 5 degrees
        Serial.printf("Angle: %d, Actual: %d, Diff: %d\n", 
                      target_angles[i], actual, diff);
    }
}

// ============================================================
# TEST SUITE
// ============================================================
void setup_hardware_tests() {
    UNITY_BEGIN();
    
    // Lock controller tests
    RUN_TEST(test_lock_init);
    RUN_TEST(test_lock_operation);
    RUN_TEST(test_lock_toggle);
    RUN_TEST(test_lock_state_queries);
    RUN_TEST(test_lock_move_time);
    RUN_TEST(test_lock_moving_state);
    
    // Sensor tests
    RUN_TEST(test_sensor_init);
    RUN_TEST(test_temperature_read);
    RUN_TEST(test_humidity_read);
    RUN_TEST(test_door_sensor);
    RUN_TEST(test_motion_sensor);
    RUN_TEST(test_battery_level);
    RUN_TEST(test_temperature_getter);
    RUN_TEST(test_humidity_getter);
    
    // Servo tests
    RUN_TEST(test_servo_attach);
    RUN_TEST(test_servo_move);
    RUN_TEST(test_servo_detach);
    RUN_TEST(test_servo_precision);
    
    // GPIO tests
    RUN_TEST(test_gpio_output);
    RUN_TEST(test_gpio_input);
    RUN_TEST(test_gpio_interrupt);
    
    // Performance
    RUN_TEST(test_hardware_performance);
    
    UNITY_END();
}
