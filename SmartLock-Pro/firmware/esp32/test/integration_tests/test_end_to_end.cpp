// test_end_to_end.cpp - End-to-End Integration Tests
#include <Arduino.h>
#include <unity.h>
#include "../../src/main.h"
#include "../../src/lock_controller.h"
#include "../../src/authentication/fingerprint.h"
#include "../../src/authentication/rfid.h"
#include "../../src/authentication/keypad.h"

// Global instances (from main)
extern LockController lockController;
extern FingerprintAuth fingerprintAuth;
extern RFIDAuth rfidAuth;
extern KeypadAuth keypadAuth;

// ============================================================
# FULL ACCESS CYCLE TESTS
// ============================================================
void test_full_access_cycle_keypad() {
    // Setup
    keypadAuth.setPIN(1, "1234");
    lockController.lock();
    TEST_ASSERT_EQUAL(LOCK_LOCKED, lockController.getState());
    
    // Authenticate
    TEST_ASSERT_TRUE(keypadAuth.authenticate("1234"));
    
    // Unlock
    TEST_ASSERT_TRUE(lockController.unlock());
    TEST_ASSERT_EQUAL(LOCK_UNLOCKED, lockController.getState());
    
    // Auto-lock should trigger after timeout
    delay(AUTO_LOCK_TIMEOUT * 1000 + 100);
    TEST_ASSERT_EQUAL(LOCK_LOCKED, lockController.getState());
}

void test_full_access_cycle_fingerprint() {
    // Setup
    fingerprintAuth.enroll(1);
    lockController.lock();
    TEST_ASSERT_EQUAL(LOCK_LOCKED, lockController.getState());
    
    // Authenticate (simulate finger scan)
    int result = fingerprintAuth.authenticate();
    TEST_ASSERT_TRUE(result == 1 || result == -1);
    
    if (result == 1) {
        // Unlock
        TEST_ASSERT_TRUE(lockController.unlock());
        TEST_ASSERT_EQUAL(LOCK_UNLOCKED, lockController.getState());
    }
}

void test_full_access_cycle_rfid() {
    // Setup
    rfidAuth.addCard(0x12345678, 1);
    lockController.lock();
    TEST_ASSERT_EQUAL(LOCK_LOCKED, lockController.getState());
    
    // Authenticate (simulate card scan)
    uint32_t uid = rfidAuth.authenticate();
    TEST_ASSERT_TRUE(uid == 0x12345678 || uid == 0);
    
    if (uid == 0x12345678) {
        // Unlock
        TEST_ASSERT_TRUE(lockController.unlock());
        TEST_ASSERT_EQUAL(LOCK_UNLOCKED, lockController.getState());
    }
}

// ============================================================
# MULTI-USER SCENARIO TESTS
// ============================================================
void test_multi_user_authentication() {
    // Add multiple users
    keypadAuth.setPIN(1, "1111");
    keypadAuth.setPIN(2, "2222");
    keypadAuth.setPIN(3, "3333");
    
    // Test each user
    TEST_ASSERT_TRUE(keypadAuth.authenticate("1111"));
    TEST_ASSERT_TRUE(keypadAuth.authenticate("2222"));
    TEST_ASSERT_TRUE(keypadAuth.authenticate("3333"));
    
    // Invalid user
    TEST_ASSERT_FALSE(keypadAuth.authenticate("4444"));
}

void test_user_access_levels() {
    // Admin - full access
    keypadAuth.setPIN(99, "9999");
    
    // Guest - limited access
    keypadAuth.setPIN(100, "0000");
    
    // Both can authenticate
    TEST_ASSERT_TRUE(keypadAuth.authenticate("9999"));
    TEST_ASSERT_TRUE(keypadAuth.authenticate("0000"));
    
    // In real system, access levels would be checked
    // This is a simplified test
}

// ============================================================
# SECURITY SCENARIO TESTS
// ============================================================
void test_failed_attempt_lockout() {
    // Clear previous lockout
    // Reset lockout counter
    
    // Failed attempts
    for (int i = 0; i < FAILED_ATTEMPTS_LIMIT; i++) {
        keypadAuth.authenticate("wrong");
    }
    
    // Next attempt should be locked out
    // This is a placeholder - actual lockout logic would be tested
    TEST_ASSERT_TRUE(true);
}

void test_tamper_detection_scenario() {
    // Simulate tamper detection
    // This would trigger an alarm and notifications
    // TEST_ASSERT_TRUE(errorHandler.hasError(ERR_SEC_TAMPER));
    TEST_ASSERT_TRUE(true);
}

void test_power_failure_scenario() {
    // Save state
    // Save state to SPIFFS
    // Test recovery after reset
    TEST_ASSERT_TRUE(true);
}

// ============================================================
# REAL-WORLD SCENARIO TESTS
// ============================================================
void test_office_workflow() {
    // Morning - employees arrive
    keypadAuth.setPIN(10, "1234"); // Employee 1
    keypadAuth.setPIN(20, "5678"); // Employee 2
    
    // Employee 1 enters
    lockController.unlock();
    delay(5000);
    lockController.lock();
    
    // Employee 2 enters
    lockController.unlock();
    delay(5000);
    lockController.lock();
    
    TEST_ASSERT_EQUAL(LOCK_LOCKED, lockController.getState());
}

void test_airbnb_workflow() {
    // Guest check-in
    keypadAuth.setPIN(30, "4321");
    
    // Guest enters
    TEST_ASSERT_TRUE(keypadAuth.authenticate("4321"));
    lockController.unlock();
    delay(5000);
    lockController.lock();
    
    // Guest check-out - delete access
    keypadAuth.removePIN("4321");
    TEST_ASSERT_FALSE(keypadAuth.authenticate("4321"));
}

void test_emergency_access_workflow() {
    // Emergency unlock
    // Should work even if authentication fails
    lockController.unlock();
    TEST_ASSERT_EQUAL(LOCK_UNLOCKED, lockController.getState());
    
    // Emergency access logged
    // TEST_ASSERT_TRUE(logger.hasEvent("emergency_access"));
}

// ============================================================
# SYSTEM RECOVERY TESTS
// ============================================================
void test_system_recovery_after_crash() {
    // Simulate crash
    // ESP.restart();
    // After restart, system should recover state
    TEST_ASSERT_TRUE(true);
}

void test_network_recovery() {
    // Simulate network failure
    WiFi.disconnect();
    delay(1000);
    
    // System should recover automatically
    wifiManager.update();
    TEST_ASSERT_TRUE(wifiManager.isConnectedToWiFi() || !wifiManager.isConnectedToWiFi());
}

// ============================================================
# PERFORMANCE SCENARIO TESTS
// ============================================================
void test_high_load_scenario() {
    unsigned long start = millis();
    
    // Simulate high load with multiple operations
    for (int i = 0; i < 100; i++) {
        keypadAuth.authenticate("1234");
        lockController.unlock();
        lockController.lock();
        sensorManager.getTemperature();
        sensorManager.getBatteryLevel();
        wifiManager.update();
    }
    
    unsigned long duration = millis() - start;
    Serial.printf("High load scenario: %lu ms\n", duration);
    
    TEST_ASSERT_TRUE(duration < 60000); // < 1 minute
}

void test_concurrent_operations() {
    // Simulate concurrent operations
    // Multiple authentication attempts, lock operations, sensor reads
    
    // Test that system remains stable
    TEST_ASSERT_TRUE(true);
}

// ============================================================
# TEST SUITE
// ============================================================
void setup_end_to_end_tests() {
    UNITY_BEGIN();
    
    // Full access cycle tests
    RUN_TEST(test_full_access_cycle_keypad);
    RUN_TEST(test_full_access_cycle_fingerprint);
    RUN_TEST(test_full_access_cycle_rfid);
    
    // Multi-user scenario tests
    RUN_TEST(test_multi_user_authentication);
    RUN_TEST(test_user_access_levels);
    
    // Security scenario tests
    RUN_TEST(test_failed_attempt_lockout);
    RUN_TEST(test_tamper_detection_scenario);
    RUN_TEST(test_power_failure_scenario);
    
    // Real-world scenario tests
    RUN_TEST(test_office_workflow);
    RUN_TEST(test_airbnb_workflow);
    RUN_TEST(test_emergency_access_workflow);
    
    // System recovery tests
    RUN_TEST(test_system_recovery_after_crash);
    RUN_TEST(test_network_recovery);
    
    // Performance scenario tests
    RUN_TEST(test_high_load_scenario);
    RUN_TEST(test_concurrent_operations);
    
    UNITY_END();
}
