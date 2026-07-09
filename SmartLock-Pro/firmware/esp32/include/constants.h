// constants.h - System Constants and Configuration
#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <Arduino.h>

// ============================================================
// DEVICE IDENTIFICATION
// ============================================================
#define DEVICE_ID           "DEV_001"
#define DEVICE_NAME         "SmartLock Pro"
#define DEVICE_LOCATION     "Front Door"
#define FIRMWARE_VERSION    "v1.0.0"
#define BUILD_DATE          __DATE__ " " __TIME__

// ============================================================
// WIFI CONFIGURATION
// ============================================================
#define WIFI_CONNECTION_TIMEOUT     30      // Seconds
#define WIFI_RECONNECT_INTERVAL     10      // Seconds
#define WIFI_MAX_RETRIES            5
#define WIFI_HOSTNAME               "smartlock-001"

// ============================================================
// MQTT CONFIGURATION
// ============================================================
#define MQTT_PORT                   1883
#define MQTT_KEEP_ALIVE             60
#define MQTT_QOS                    1
#define MQTT_TOPIC_PREFIX           "/smartlock"

// ============================================================
// AUTHENTICATION SETTINGS
// ============================================================
#define MAX_USERS                   100
#define MAX_FINGERPRINT_TEMPLATES   100
#define MAX_RFID_CARDS              100
#define MAX_PIN_LENGTH              6
#define MIN_PIN_LENGTH              4
#define FAILED_ATTEMPTS_LIMIT       5
#define LOCKOUT_DURATION            300     // Seconds

// ============================================================
// LOCK SETTINGS
// ============================================================
#define SERVO_MIN_PULSE             500     // µs
#define SERVO_MAX_PULSE             2400    // µs
#define LOCK_ANGLE                  0       // Degrees
#define UNLOCK_ANGLE                90      // Degrees
#define AUTO_LOCK_TIMEOUT           30      // Seconds
#define LOCK_RETRY_ATTEMPTS         3

// ============================================================
// SENSOR SETTINGS
// ============================================================
#define TEMP_UPDATE_INTERVAL        60      // Seconds
#define MOTION_TIMEOUT              30      // Seconds
#define DOOR_OPEN_THRESHOLD         1       // 1 = Open, 0 = Closed

// ============================================================
// COMMUNICATION SETTINGS
// ============================================================
#define BLE_ADV_INTERVAL            100     // ms
#define BLE_CONNECTION_TIMEOUT      30      // Seconds
#define HTTP_PORT                   80
#define WEBSOCKET_ENABLED           true

// ============================================================
// SECURITY SETTINGS
// ============================================================
#define SECURE_BOOT_ENABLED         true
#define FLASH_ENCRYPTION_ENABLED    true
#define USE_TLS                     true
#define ENCRYPTION_KEY_LENGTH       32      // Bytes (256-bit)

// ============================================================
// POWER MANAGEMENT
// ============================================================
#define DEEP_SLEEP_TIMEOUT          300     // Seconds
#define CPU_FREQ_ACTIVE             240000000L
#define CPU_FREQ_SLEEP              80000000L
#define POWER_SAVE_MODE             true

// ============================================================
// OTA SETTINGS
// ============================================================
#define OTA_CHECK_INTERVAL          3600    // Seconds
#define OTA_BACKUP_ENABLED          true
#define OTA_BUFFER_SIZE             4096

// ============================================================
// LOGGING SETTINGS
// ============================================================
#define LOG_BUFFER_SIZE             4096
#define MAX_LOG_ENTRIES             10000
#define LOG_LEVEL_INFO              1
#define LOG_LEVEL_DEBUG             2
#define LOG_LEVEL_ERROR             0

// ============================================================
// TIMING CONSTANTS
// ============================================================
#define ONE_SECOND                  1000    // ms
#define ONE_MINUTE                  60000   // ms
#define ONE_HOUR                    3600000 // ms
#define ONE_DAY                     86400000 // ms

// ============================================================
// ERROR CODES
// ============================================================
enum ErrorCode {
    ERR_SUCCESS = 0,
    ERR_HARDWARE_INIT = 1,
    ERR_WIFI_CONNECT = 2,
    ERR_MQTT_CONNECT = 3,
    ERR_AUTH_FAILED = 4,
    ERR_FINGERPRINT_NOT_FOUND = 5,
    ERR_FINGERPRINT_NO_MATCH = 6,
    ERR_RFID_INVALID = 7,
    ERR_PIN_INVALID = 8,
    ERR_LOCK_JAMMED = 9,
    ERR_SENSOR_FAILED = 10,
    ERR_OTA_FAILED = 11,
    ERR_MEMORY_LOW = 12,
    ERR_TAMPER_DETECTED = 13,
    ERR_BATTERY_LOW = 14
};

// ============================================================
// AUTHENTICATION METHODS
// ============================================================
enum AuthMethod {
    AUTH_FINGERPRINT = 1,
    AUTH_RFID = 2,
    AUTH_KEYPAD = 3,
    AUTH_BLE = 4,
    AUTH_REMOTE = 5,
    AUTH_OTP = 6
};

// ============================================================
// LOCK STATES
// ============================================================
enum LockState {
    LOCK_LOCKED = 0,
    LOCK_UNLOCKED = 1,
    LOCK_JAMMED = 2,
    LOCK_TRANSITIONING = 3,
    LOCK_UNKNOWN = 4
};

// ============================================================
// SYSTEM STATES
// ============================================================
enum SystemState {
    SYS_INIT = 0,
    SYS_CONFIG = 1,
    SYS_READY = 2,
    SYS_ERROR = 3,
    SYS_MAINTENANCE = 4,
    SYS_DEEP_SLEEP = 5
};

// ============================================================
// DATA STRUCTURES
// ============================================================
struct User {
    uint16_t id;
    char username[32];
    char pin[8];
    uint16_t fingerprint_id;
    uint32_t rfid_uid;
    uint8_t access_level;
    bool enabled;
    uint32_t last_access;
    uint8_t failed_attempts;
};

struct AccessLog {
    uint32_t timestamp;
    uint16_t user_id;
    uint8_t method;
    bool granted;
    char reason[64];
};

struct SystemStatus {
    LockState lock_state;
    bool door_open;
    uint8_t battery_level;
    int16_t wifi_rssi;
    bool mqtt_connected;
    uint32_t uptime;
    float temperature;
    float humidity;
    uint16_t free_heap;
};

#endif // CONSTANTS_H
