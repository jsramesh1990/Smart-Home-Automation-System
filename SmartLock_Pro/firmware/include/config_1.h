#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// WiFi Configuration
const char* WIFI_SSID = "YourWiFiSSID";
const char* WIFI_PASSWORD = "YourWiFiPassword";

// MQTT Configuration
const char* MQTT_BROKER = "192.168.1.100";
const int MQTT_PORT = 1883;
const char* MQTT_CLIENT_ID = "smartlock_esp32_001";

// Pin Configuration
#define RELAY_LOCK_PIN   16
#define RELAY_UNLOCK_PIN 17
#define RGB_LED_PIN      2
#define BUZZER_PIN       15
#define TAMPER_PIN       4
#define BATTERY_PIN      34

// Lock Configuration
#define AUTO_LOCK_DELAY_MS 30000  // 30 seconds
#define MAX_UNLOCK_ATTEMPTS 5
#define LOCK_COOLDOWN_MS 10000

// Voice Recognition
#define VOICE_SENSOR_PIN   35
#define VOICE_SAMPLE_RATE  16000
#define VOICE_BUFFER_SIZE  1024

// Security
#define JWT_SECRET "your-jwt-secret-key"
#define API_KEY "smartlock-api-key-2024"

// Network
#define LOCAL_PORT 80
#define WEBSOCKET_PORT 81

#endif
