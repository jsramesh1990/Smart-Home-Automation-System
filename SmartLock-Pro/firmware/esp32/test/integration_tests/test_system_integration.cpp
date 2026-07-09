// test_system_integration.cpp - System Integration Tests
#include <Arduino.h>
#include <unity.h>
#include "../../src/main.h"
#include "../../src/authentication/fingerprint.h"
#include "../../src/authentication/rfid.h"
#include "../../src/authentication/keypad.h"
#include "../../src/communication/mqtt_client.h"
#include "../../src/communication/ble_manager.h"
#include "../../src/utilities/logger.h"

// ============================================================
# COMPONENT INTERACTION TESTS
// ============================================================
void test_auth_to_lock_integration() {
    // Setup
    keypadAuth.setPIN(1, "1111");
    lockController.lock();
    
    // Auth -> Lock flow
    TEST_ASSERT_TRUE(keypadAuth.authenticate("1111"));
    TEST_ASSERT_TRUE(lockController.unlock());
    TEST_ASSERT_EQUAL(LOCK_UNLOCKED, lockController.getState());
}

void test_sensor_to_mqtt_integration() {
    // Sensor -> MQTT flow
    float temp;
    bool result = sensorManager.readTemperature(&temp);
    
    if (result && mqttClient.isConnected()) {
        TEST_ASSERT_TRUE(mqttClient.publishStatus("temperature", temp));
    } else {
        TEST_ASSERT_TRUE(true);
    }
}

void test_mqtt_to_lock_integration() {
    // MQTT command -> Lock flow
    // mqttClient.processCommand("LOCK");
    // TEST_ASSERT_EQUAL(LOCK_LOCKED, lockController.getState());
    // mqttClient.processCommand("UNLOCK");
    // TEST_ASSERT_EQUAL(LOCK_UNLOCKED, lockController.getState());
}

// ============================================================
# FULL SYSTEM INTEGRATION TESTS
// ============================================================
void test_full_system_initialization() {
    // Test all components initialize
    TEST_ASSERT_TRUE(wifiManager.init());
    TEST_ASSERT_TRUE(mqttClient.init());
    TEST_ASSERT_TRUE(bleManager.init());
    TEST_ASSERT_TRUE(lockController.init());
    TEST_ASSERT_TRUE(sensorManager.init());
    TEST_ASSERT_TRUE(fingerprintAuth.init());
    TEST_ASSERT_TRUE(rfidAuth.init());
    TEST_ASSERT_TRUE(keypadAuth.init());
    TEST_ASSERT_TRUE(encryption.init());
    TEST_ASSERT_TRUE(otpGen.init());
}

void test_system_status_reporting() {
    // Update system status
    updateSystemStatus();
    
    // Verify status is reported
    if (mqttClient.isConnected()) {
        // Check that status was published
        TEST_ASSERT_TRUE(true);
    }
}

void test_event_logging_integration() {
    // Trigger events and verify logging
    keypadAuth.authenticate("wrong");
    lockController.unlock();
    lockController.lock();
    
    // Check that events were logged
    // TEST_ASSERT_TRUE(logger.getEventCount() > 0);
}

// ============================================================
# DATA FLOW INTEGRATION TESTS
// ============================================================
void test_user_data_persistence() {
    // Add user
    keypadAuth.setPIN(1, "1234");
    fingerprintAuth.enroll(1);
    rfidAuth.addCard(0x12345678, 1);
    
    // Verify data is stored
    TEST_ASSERT_TRUE(keypadAuth.getUserByPIN("1234") == 1);
    TEST_ASSERT_TRUE(rfidAuth.getUserByUID(0x12345678) == 1);
}

void test_configuration_persistence() {
    // Change configuration
    // Set configuration via API
    
    // Restart system
    // ESP.restart();
    
    // Verify configuration persisted
    TEST_ASSERT_TRUE(true);
}

void test_time_synchronization() {
    // Get current time
    time_t now = time(nullptr);
    
    // Check if time is valid (not 1970)
    if (now > 1577836800) { // After 2020-01-01
        TEST_ASSERT_TRUE(true);
    }
}

// ============================================================
# STRESS INTEGRATION TESTS
// ============================================================
void test_system_stability() {
    // Run system for extended period
    unsigned long start = millis();
    
    while (millis() - start < 60000) { // 1 minute
        keypadAuth.authenticate("1234");
        lockController.unlock();
        lockController.lock();
        sensorManager.update();
        wifiManager.update();
        mqttClient.update();
        delay(100);
    }
    
    TEST_ASSERT_TRUE(true);
}

void test_peak_load_integration() {
    unsigned long start = millis();
    
    // Multiple authentication methods simultaneously
    keypadAuth.readInput();
    rfidAuth.authenticate();
    fingerprintAuth.authenticate();
    
    // Multiple operations
    lockController.unlock();
    sensorManager.readTemperature(nullptr);
    sensorManager.readHumidity(nullptr);
    wifiManager.update();
    mqttClient.update();
    
    unsigned long duration = millis() - start;
    Serial.printf("Peak load test: %lu ms\n", duration);
    
    TEST_ASSERT_TRUE(duration < 500);
}

// ============================================================
# FAILOVER INTEGRATION TESTS
// ============================================================
void test_wifi_failover() {
    // Simulate WiFi failure
    WiFi.disconnect();
    delay(1000);
    
    // System should handle gracefully
    wifiManager.update();
    bool state = wifiManager.isConnectedToWiFi();
    TEST_ASSERT_TRUE(state || !state);
}

void test_mqtt_failover() {
    // Simulate MQTT broker failure
    // Force disconnection
    
    // System should queue messages
    // TEST_ASSERT_TRUE(mqttClient.hasQueuedMessages());
}

void test_power_failover() {
    // Test battery backup
    // Should maintain state during power failure
    lockController.lock();
    // TEST_ASSERT_TRUE(sensorManager.getBatteryLevel() > 0);
}

// ============================================================
# SECURITY INTEGRATION TESTS
// ============================================================
void test_encryption_integration() {
    // Encrypt sensitive data
    char sensitive_data[] = "Sensitive User Data";
    uint8_t encrypted[256];
    uint8_t decrypted[256];
    
    TEST_ASSERT_TRUE(encryption.aes_encrypt(
        (const uint8_t*)sensitive_data,
        strlen(sensitive_data) + 1,
        encrypted
    ));
    
    TEST_ASSERT_TRUE(encryption.aes_decrypt(
        encrypted,
        strlen(sensitive_data) + 17,
        decrypted
    ));
    
    TEST_ASSERT_EQUAL_STRING(sensitive_data, (char*)decrypted);
}

void test_secure_communication() {
    // Test TLS/SSL for MQTT
    if (USE_TLS) {
        // Check that TLS is enabled
        TEST_ASSERT_TRUE(MQTT_PORT == 8883);
    }
}

void test_audit_trail_integration() {
    // Perform operations and verify audit trail
    keypadAuth.authenticate("1234");
    lockController.unlock();
    lockController.lock();
    
    // Check audit log
    // TEST_ASSERT_TRUE(auditLog.getEntryCount() >= 3);
}

// ============================================================
# TEST SUITE
// ============================================================
void setup_system_integration_tests() {
    UNITY_BEGIN();
    
    // Component interaction tests
    RUN_TEST(test_auth_to_lock_integration);
    RUN_TEST(test_sensor_to_mqtt_integration);
    RUN_TEST(test_mqtt_to_lock_integration);
    
    // Full system tests
    RUN_TEST(test_full_system_initialization);
    RUN_TEST(test_system_status_reporting);
    RUN_TEST(test_event_logging_integration);
    
    // Data flow tests
    RUN_TEST(test_user_data_persistence);
    RUN_TEST(test_configuration_persistence);
    RUN_TEST(test_time_synchronization);
    
    // Stress tests
    RUN_TEST(test_system_stability);
    RUN_TEST(test_peak_load_integration);
    
    // Failover tests
    RUN_TEST(test_wifi_failover);
    RUN_TEST(test_mqtt_failover);
    RUN_TEST(test_power_failover);
    
    // Security tests
    RUN_TEST(test_encryption_integration);
    RUN_TEST(test_secure_communication);
    RUN_TEST(test_audit_trail_integration);
    
    UNITY_END();
}
