// test_performance.cpp - Performance Integration Tests
#include <Arduino.h>
#include <unity.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>

// ============================================================
# PERFORMANCE METRICS
// ============================================================
struct PerformanceMetrics {
    unsigned long min_time;
    unsigned long max_time;
    unsigned long avg_time;
    unsigned long total_time;
    int count;
};

PerformanceMetrics metrics_auth;
PerformanceMetrics metrics_lock;
PerformanceMetrics metrics_sensor;
PerformanceMetrics metrics_network;

void resetMetrics(PerformanceMetrics* m) {
    m->min_time = ULONG_MAX;
    m->max_time = 0;
    m->avg_time = 0;
    m->total_time = 0;
    m->count = 0;
}

void recordMetric(PerformanceMetrics* m, unsigned long time) {
    if (time < m->min_time) m->min_time = time;
    if (time > m->max_time) m->max_time = time;
    m->total_time += time;
    m->count++;
    m->avg_time = m->total_time / m->count;
}

void printMetrics(const char* name, PerformanceMetrics* m) {
    Serial.printf("%s - Min: %lu, Max: %lu, Avg: %lu, Count: %d\n",
                  name, m->min_time, m->max_time, m->avg_time, m->count);
}

// ============================================================
# AUTHENTICATION PERFORMANCE TESTS
// ============================================================
void test_auth_speed() {
    resetMetrics(&metrics_auth);
    
    for (int i = 0; i < 100; i++) {
        unsigned long start = micros();
        keypadAuth.authenticate("1234");
        unsigned long duration = micros() - start;
        recordMetric(&metrics_auth, duration);
    }
    
    printMetrics("Authentication", &metrics_auth);
    TEST_ASSERT_TRUE(metrics_auth.avg_time < 1000); // < 1ms average
}

void test_fingerprint_speed() {
    resetMetrics(&metrics_auth);
    
    for (int i = 0; i < 10; i++) {
        unsigned long start = micros();
        fingerprintAuth.authenticate();
        unsigned long duration = micros() - start;
        recordMetric(&metrics_auth, duration);
    }
    
    printMetrics("Fingerprint", &metrics_auth);
    TEST_ASSERT_TRUE(metrics_auth.avg_time < 500000); // < 500ms average
}

void test_rfid_speed() {
    resetMetrics(&metrics_auth);
    
    for (int i = 0; i < 100; i++) {
        unsigned long start = micros();
        rfidAuth.authenticate();
        unsigned long duration = micros() - start;
        recordMetric(&metrics_auth, duration);
    }
    
    printMetrics("RFID", &metrics_auth);
    TEST_ASSERT_TRUE(metrics_auth.avg_time < 10000); // < 10ms average
}

void test_otp_speed() {
    resetMetrics(&metrics_auth);
    
    for (int i = 0; i < 100; i++) {
        unsigned long start = micros();
        otpGen.generateOTP();
        unsigned long duration = micros() - start;
        recordMetric(&metrics_auth, duration);
    }
    
    printMetrics("OTP", &metrics_auth);
    TEST_ASSERT_TRUE(metrics_auth.avg_time < 1000); // < 1ms average
}

// ============================================================
# LOCK PERFORMANCE TESTS
// ============================================================
void test_lock_speed() {
    resetMetrics(&metrics_lock);
    
    for (int i = 0; i < 20; i++) {
        unsigned long start = micros();
        lockController.unlock();
        unsigned long duration = micros() - start;
        recordMetric(&metrics_lock, duration);
        
        lockController.lock();
        delay(10);
    }
    
    printMetrics("Lock", &metrics_lock);
    TEST_ASSERT_TRUE(metrics_lock.avg_time < 500000); // < 500ms average
}

void test_lock_consistency() {
    int consistent_count = 0;
    
    for (int i = 0; i < 50; i++) {
        lockController.unlock();
        int pos1 = testServo.read();
        lockController.lock();
        int pos2 = testServo.read();
        
        if (pos1 == UNLOCK_ANGLE && pos2 == LOCK_ANGLE) {
            consistent_count++;
        }
        delay(10);
    }
    
    Serial.printf("Lock consistency: %d/50\n", consistent_count);
    TEST_ASSERT_TRUE(consistent_count >= 45);
}

// ============================================================
# SENSOR PERFORMANCE TESTS
// ============================================================
void test_sensor_read_speed() {
    resetMetrics(&metrics_sensor);
    
    for (int i = 0; i < 100; i++) {
        unsigned long start = micros();
        sensorManager.getTemperature();
        sensorManager.getHumidity();
        unsigned long duration = micros() - start;
        recordMetric(&metrics_sensor, duration);
    }
    
    printMetrics("Sensor Read", &metrics_sensor);
    TEST_ASSERT_TRUE(metrics_sensor.avg_time < 10000); // < 10ms average
}

void test_battery_read_speed() {
    resetMetrics(&metrics_sensor);
    
    for (int i = 0; i < 100; i++) {
        unsigned long start = micros();
        sensorManager.getBatteryLevel();
        unsigned long duration = micros() - start;
        recordMetric(&metrics_sensor, duration);
    }
    
    printMetrics("Battery Read", &metrics_sensor);
    TEST_ASSERT_TRUE(metrics_sensor.avg_time < 1000); // < 1ms average
}

// ============================================================
# NETWORK PERFORMANCE TESTS
// ============================================================
void test_wifi_connection_speed() {
    if (!wifiManager.isConnectedToWiFi()) {
        TEST_ASSERT_TRUE(true);
        return;
    }
    
    resetMetrics(&metrics_network);
    
    // Measure connection speed
    unsigned long start = micros();
    wifiManager.update();
    unsigned long duration = micros() - start;
    recordMetric(&metrics_network, duration);
    
    printMetrics("WiFi Connection", &metrics_network);
    TEST_ASSERT_TRUE(metrics_network.avg_time < 10000);
}

void test_mqtt_publish_speed() {
    if (!mqttClient.isConnected()) {
        TEST_ASSERT_TRUE(true);
        return;
    }
    
    resetMetrics(&metrics_network);
    
    for (int i = 0; i < 50; i++) {
        unsigned long start = micros();
        mqttClient.publishStatus("perf_test", i);
        unsigned long duration = micros() - start;
        recordMetric(&metrics_network, duration);
    }
    
    printMetrics("MQTT Publish", &metrics_network);
    TEST_ASSERT_TRUE(metrics_network.avg_time < 10000); // < 10ms average
}

// ============================================================
# MEMORY PERFORMANCE TESTS
// ============================================================
void test_memory_usage() {
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t minFreeHeap = ESP.getMinFreeHeap();
    uint32_t maxAlloc = ESP.getMaxAllocHeap();
    
    Serial.printf("Free Heap: %u bytes\n", freeHeap);
    Serial.printf("Min Free Heap: %u bytes\n", minFreeHeap);
    Serial.printf("Max Alloc: %u bytes\n", maxAlloc);
    
    // Should have at least 10KB free
    TEST_ASSERT_TRUE(freeHeap > 10240);
    TEST_ASSERT_TRUE(maxAlloc > 4096);
}

void test_memory_stability() {
    uint32_t heap1 = ESP.getFreeHeap();
    
    // Perform operations
    for (int i = 0; i < 100; i++) {
        keypadAuth.authenticate("1234");
        lockController.unlock();
        lockController.lock();
        sensorManager.getTemperature();
        wifiManager.update();
    }
    
    uint32_t heap2 = ESP.getFreeHeap();
    int diff = (int)heap2 - (int)heap1;
    
    Serial.printf("Memory change: %d bytes\n", diff);
    TEST_ASSERT_TRUE(abs(diff) < 2048); // Should not leak more than 2KB
}

// ============================================================
# CPU PERFORMANCE TESTS
// ============================================================
void test_cpu_usage() {
    unsigned long start = millis();
    unsigned long busy_time = 0;
    
    // Run for 1 second measuring CPU time
    while (millis() - start < 1000) {
        unsigned long cycle_start = micros();
        
        // Simulate work
        keypadAuth.authenticate("1234");
        lockController.unlock();
        lockController.lock();
        
        busy_time += micros() - cycle_start;
        delay(1);
    }
    
    float cpu_percent = (busy_time / 1000.0f) * 100.0f;
    Serial.printf("CPU Usage: %.1f%%\n", cpu_percent);
    
    // Should be below 70% average
    TEST_ASSERT_TRUE(cpu_percent < 70);
}

// ============================================================
# TEST SUITE
// ============================================================
void setup_performance_tests() {
    UNITY_BEGIN();
    
    // Authentication performance
    RUN_TEST(test_auth_speed);
    RUN_TEST(test_fingerprint_speed);
    RUN_TEST(test_rfid_speed);
    RUN_TEST(test_otp_speed);
    
    // Lock performance
    RUN_TEST(test_lock_speed);
    RUN_TEST(test_lock_consistency);
    
    // Sensor performance
    RUN_TEST(test_sensor_read_speed);
    RUN_TEST(test_battery_read_speed);
    
    // Network performance
    RUN_TEST(test_wifi_connection_speed);
    RUN_TEST(test_mqtt_publish_speed);
    
    // Memory performance
    RUN_TEST(test_memory_usage);
    RUN_TEST(test_memory_stability);
    
    // CPU performance
    RUN_TEST(test_cpu_usage);
    
    UNITY_END();
}
