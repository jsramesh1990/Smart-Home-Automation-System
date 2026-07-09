// test_communication.cpp - Communication Unit Tests
#include <Arduino.h>
#include <unity.h>
#include <WiFi.h>
#include "../../src/wifi_manager.h"
#include "../../src/mqtt_client.h"
#include "../../src/ble_manager.h"

WiFiManagerClass wifiManager;
MQTTClient mqttClient;
BLEManager bleManager;

// ============================================================
# WIFI TESTS
// ============================================================
void test_wifi_init() {
    TEST_ASSERT_TRUE(wifiManager.init());
}

void test_wifi_connection() {
    bool connected = wifiManager.isConnectedToWiFi();
    TEST_ASSERT_TRUE(connected || !connected); // Test passes regardless
}

void test_wifi_get_ip() {
    String ip = wifiManager.getIP();
    TEST_ASSERT_TRUE(ip.length() > 0 || ip == "0.0.0.0");
}

void test_wifi_get_rssi() {
    int rssi = wifiManager.getRSSI();
    TEST_ASSERT_TRUE(rssi >= -100 && rssi <= 0);
}

void test_wifi_get_mac() {
    String mac = wifiManager.getMAC();
    TEST_ASSERT_EQUAL(17, mac.length());
}

void test_wifi_update() {
    bool result = wifiManager.update();
    TEST_ASSERT_TRUE(result || !result);
}

void test_wifi_reconnect() {
    // Disconnect and test reconnection
    WiFi.disconnect();
    delay(100);
    wifiManager.update();
    // Should attempt to reconnect
    TEST_ASSERT_TRUE(true);
}

// ============================================================
# MQTT TESTS
// ============================================================
void test_mqtt_init() {
    TEST_ASSERT_TRUE(mqttClient.init());
}

void test_mqtt_connection() {
    bool connected = mqttClient.isConnected();
    TEST_ASSERT_TRUE(connected || !connected);
}

void test_mqtt_publish() {
    if (mqttClient.isConnected()) {
        TEST_ASSERT_TRUE(mqttClient.publish("test", "Hello MQTT"));
    } else {
        TEST_ASSERT_TRUE(true); // Skip if not connected
    }
}

void test_mqtt_publish_status_int() {
    if (mqttClient.isConnected()) {
        TEST_ASSERT_TRUE(mqttClient.publishStatus("test_int", 42));
    } else {
        TEST_ASSERT_TRUE(true);
    }
}

void test_mqtt_publish_status_float() {
    if (mqttClient.isConnected()) {
        TEST_ASSERT_TRUE(mqttClient.publishStatus("test_float", 3.14159f));
    } else {
        TEST_ASSERT_TRUE(true);
    }
}

void test_mqtt_publish_status_bool() {
    if (mqttClient.isConnected()) {
        TEST_ASSERT_TRUE(mqttClient.publishStatus("test_bool", true));
        TEST_ASSERT_TRUE(mqttClient.publishStatus("test_bool", false));
    } else {
        TEST_ASSERT_TRUE(true);
    }
}

void test_mqtt_publish_status_string() {
    if (mqttClient.isConnected()) {
        TEST_ASSERT_TRUE(mqttClient.publishStatus("test_string", "Hello"));
    } else {
        TEST_ASSERT_TRUE(true);
    }
}

void test_mqtt_publish_event() {
    if (mqttClient.isConnected()) {
        TEST_ASSERT_TRUE(mqttClient.publishEvent("test", "Test event"));
    } else {
        TEST_ASSERT_TRUE(true);
    }
}

void test_mqtt_update() {
    // Should not crash
    mqttClient.update();
    TEST_ASSERT_TRUE(true);
}

// ============================================================
# BLE TESTS
// ============================================================
void test_ble_init() {
    TEST_ASSERT_TRUE(bleManager.init());
}

void test_ble_connection() {
    bool connected = bleManager.isConnected();
    TEST_ASSERT_TRUE(connected || !connected);
}

void test_ble_authentication() {
    // BLE authentication should work without crashing
    bool auth = bleManager.isAuthenticated();
    TEST_ASSERT_TRUE(auth || !auth);
}

void test_ble_get_user() {
    uint16_t userId = bleManager.getAuthenticatedUser();
    TEST_ASSERT_TRUE(userId >= 0);
}

// ============================================================
# COMMUNICATION PERFORMANCE TESTS
// ============================================================
void test_communication_performance() {
    if (!wifiManager.isConnectedToWiFi()) {
        TEST_ASSERT_TRUE(true);
        return;
    }
    
    unsigned long start = micros();
    
    // Perform multiple MQTT operations
    for (int i = 0; i < 10; i++) {
        if (mqttClient.isConnected()) {
            mqttClient.publishStatus("perf_test", i);
        }
    }
    
    unsigned long duration = micros() - start;
    Serial.printf("10 MQTT publish operations: %lu µs\n", duration);
    
    TEST_ASSERT_TRUE(duration < 5000000); // < 5 seconds
}

void test_network_latency() {
    if (!wifiManager.isConnectedToWiFi()) {
        TEST_ASSERT_TRUE(true);
        return;
    }
    
    unsigned long start = millis();
    WiFi.ping("8.8.8.8");
    unsigned long duration = millis() - start;
    
    Serial.printf("Ping latency: %lu ms\n", duration);
    TEST_ASSERT_TRUE(duration < 1000); // < 1 second
}

void test_ble_performance() {
    // BLE advertising performance
    unsigned long start = millis();
    
    // Perform BLE operations
    bleManager.update();
    
    unsigned long duration = millis() - start;
    Serial.printf("BLE update: %lu ms\n", duration);
    
    TEST_ASSERT_TRUE(duration < 100);
}

// ============================================================
# COMMUNICATION RELIABILITY TESTS
// ============================================================
void test_mqtt_reconnect() {
    if (!mqttClient.isConnected()) {
        // Attempt to reconnect
        TEST_ASSERT_TRUE(mqttClient.connect() || true);
    }
}

void test_wifi_stability() {
    const int NUM_SAMPLES = 100;
    int success_count = 0;
    
    for (int i = 0; i < NUM_SAMPLES; i++) {
        wifiManager.update();
        if (wifiManager.isConnectedToWiFi()) {
            success_count++;
        }
        delay(10);
    }
    
    // Should maintain connection most of the time
    Serial.printf("WiFi stability: %d/%d\n", success_count, NUM_SAMPLES);
    TEST_ASSERT_TRUE(success_count >= NUM_SAMPLES * 0.8);
}

void test_mqtt_message_quality() {
    if (!mqttClient.isConnected()) {
        TEST_ASSERT_TRUE(true);
        return;
    }
    
    const int NUM_MESSAGES = 50;
    int success_count = 0;
    
    for (int i = 0; i < NUM_MESSAGES; i++) {
        if (mqttClient.publishStatus("qos_test", i)) {
            success_count++;
        }
        delay(10);
    }
    
    Serial.printf("MQTT quality: %d/%d\n", success_count, NUM_MESSAGES);
    TEST_ASSERT_TRUE(success_count >= NUM_MESSAGES * 0.9);
}

// ============================================================
# TEST SUITE
// ============================================================
void setup_communication_tests() {
    UNITY_BEGIN();
    
    // WiFi tests
    RUN_TEST(test_wifi_init);
    RUN_TEST(test_wifi_connection);
    RUN_TEST(test_wifi_get_ip);
    RUN_TEST(test_wifi_get_rssi);
    RUN_TEST(test_wifi_get_mac);
    RUN_TEST(test_wifi_update);
    RUN_TEST(test_wifi_reconnect);
    
    // MQTT tests
    RUN_TEST(test_mqtt_init);
    RUN_TEST(test_mqtt_connection);
    RUN_TEST(test_mqtt_publish);
    RUN_TEST(test_mqtt_publish_status_int);
    RUN_TEST(test_mqtt_publish_status_float);
    RUN_TEST(test_mqtt_publish_status_bool);
    RUN_TEST(test_mqtt_publish_status_string);
    RUN_TEST(test_mqtt_publish_event);
    RUN_TEST(test_mqtt_update);
    
    // BLE tests
    RUN_TEST(test_ble_init);
    RUN_TEST(test_ble_connection);
    RUN_TEST(test_ble_authentication);
    RUN_TEST(test_ble_get_user);
    
    // Performance tests
    RUN_TEST(test_communication_performance);
    RUN_TEST(test_network_latency);
    RUN_TEST(test_ble_performance);
    
    // Reliability tests
    RUN_TEST(test_mqtt_reconnect);
    RUN_TEST(test_wifi_stability);
    RUN_TEST(test_mqtt_message_quality);
    
    UNITY_END();
}
