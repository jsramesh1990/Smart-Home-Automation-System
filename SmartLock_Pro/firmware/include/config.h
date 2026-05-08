#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// WiFi Configuration
#define WIFI_SSID "SmartLock_Network"
#define WIFI_PASSWORD "SecurePass123!"

// Backup AP Configuration
#define AP_SSID "SmartLock_Config"
#define AP_PASSWORD "Admin123"

// MQTT Configuration
#define MQTT_BROKER "192.168.1.100"
#define MQTT_PORT 1883
#define MQTT_USER ""
#define MQTT_PASSWORD ""
#define MQTT_CLIENT_ID "smartlock_001"

// Pin Configuration
#define RELAY_LOCK_PIN    16
#define RELAY_UNLOCK_PIN  17
#define RGB_LED_PIN       2
#define BUZZER_PIN        15
#define TAMPER_PIN        4
#define BATTERY_PIN       34
#define DOOR_SENSOR_PIN   35
#define MIC_INPUT_PIN     36

// Timing Configuration (milliseconds)
#define AUTO_LOCK_DELAY_MS     30000
#define MAX_UNLOCK_ATTEMPTS    5
#define LOCK_COOLDOWN_MS       10000
#define HEARTBEAT_INTERVAL_MS  30000
#define LED_BLINK_INTERVAL_MS  500

// Voice Recognition
#define VOICE_SAMPLE_RATE      16000
#define VOICE_BUFFER_SIZE      2048
#define VOICE_FRAME_SIZE       512
#define VOICE_HOP_SIZE         256
#define MFCC_COUNT             13
#define VOICE_FEATURE_SIZE     128

// Security
#define JWT_SECRET       "smartlock-jwt-secret-key-2024"
#define API_KEY          "smartlock-pro-api-key"
#define ENCRYPTION_KEY   {0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF,0xFE,0xDC,0xBA,0x98,0x76,0x54,0x32,0x10}

// Network
#define LOCAL_HTTP_PORT     80
#define WEBSOCKET_PORT      81
#define OTA_PORT            8266

// Debug
#define SERIAL_BAUD_RATE    115200
#define DEBUG_ENABLED       1

#if DEBUG_ENABLED
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

#endif
