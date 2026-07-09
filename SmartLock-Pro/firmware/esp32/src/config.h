// config.h - User Configuration File
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================
// WIFI CONFIGURATION
// ============================================================
// Set your WiFi credentials here, or use WiFiManager
#define WIFI_SSID           "YourNetworkName"      // Change this
#define WIFI_PASSWORD       "YourPassword"         // Change this
#define WIFI_HOSTNAME       "smartlock-001"

// ============================================================
// MQTT CONFIGURATION
// ============================================================
#define MQTT_BROKER         "192.168.1.100"        // Your MQTT broker IP
#define MQTT_PORT           1883                   // 8883 for TLS
#define MQTT_USERNAME       "smartlock"
#define MQTT_PASSWORD       "secure_password"

// ============================================================
# AUTHENTICATION SETTINGS
// ============================================================
#define ADMIN_PIN           "123456"              // Admin PIN
#define DEFAULT_PIN         "1234"                // Default user PIN

// ============================================================
# DEBUG SETTINGS
// ============================================================
#define DEBUG_ENABLED       1                     // 1=Enable, 0=Disable
#define DEBUG_LEVEL         2                     // 0=Error, 1=Info, 2=Debug

// ============================================================
# OTA SETTINGS
// ============================================================
#define OTA_URL             "http://192.168.1.100:8000/firmware.bin"
#define OTA_CHECK_INTERVAL  3600                  // Seconds

// ============================================================
# ENCRYPTION SETTINGS
// ============================================================
// 32-byte (256-bit) AES key - CHANGE THIS!
#define ENCRYPTION_KEY      "0123456789abcdef0123456789abcdef"

// ============================================================
# OTP SETTINGS
// ============================================================
#define OTP_SECRET          "S3cr3tK3yF0r0TPG3n3r4t10n"
#define OTP_PERIOD          30                    // Seconds

// ============================================================
# POWER MANAGEMENT
// ============================================================
#define BATTERY_ENABLED     0                     // 0=No battery, 1=Battery
#define BATTERY_PIN         34                    // ADC pin for battery

// ============================================================
# SYSTEM CONFIGURATION
// ============================================================
#define TIMEZONE            "UTC+5:30"
#define NTP_SERVER          "pool.ntp.org"

#endif // CONFIG_H
