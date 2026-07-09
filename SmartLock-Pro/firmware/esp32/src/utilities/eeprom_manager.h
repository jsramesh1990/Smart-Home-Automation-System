// eeprom_manager.h - EEPROM/Storage Manager Header
#ifndef EEPROM_MANAGER_H
#define EEPROM_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include <vector>
#include <map>

// ============================================================
// STORAGE KEYS
// ============================================================
#define STORAGE_NAMESPACE "smartlock"

// System keys
#define KEY_DEVICE_ID        "device_id"
#define KEY_DEVICE_NAME      "device_name"
#define KEY_FIRMWARE_VERSION "firmware_ver"
#define KEY_BOOT_COUNT       "boot_count"
#define KEY_SYSTEM_CONFIG    "system_cfg"

// WiFi keys
#define KEY_WIFI_SSID        "wifi_ssid"
#define KEY_WIFI_PASSWORD    "wifi_pass"
#define KEY_WIFI_HOSTNAME    "wifi_host"
#define KEY_WIFI_CONFIG      "wifi_cfg"

// MQTT keys
#define KEY_MQTT_BROKER      "mqtt_broker"
#define KEY_MQTT_PORT        "mqtt_port"
#define KEY_MQTT_USERNAME    "mqtt_user"
#define KEY_MQTT_PASSWORD    "mqtt_pass"
#define KEY_MQTT_CONFIG      "mqtt_cfg"

// User keys
#define KEY_USER_COUNT       "user_count"
#define KEY_USER_PREFIX      "user_"
#define KEY_USER_DATA        "user_data"

// Authentication keys
#define KEY_PIN_PREFIX       "pin_"
#define KEY_RFID_PREFIX      "rfid_"
#define KEY_FINGERPRINT_PREFIX "fp_"

// Security keys
#define KEY_ENCRYPTION_KEY   "encrypt_key"
#define KEY_OTP_SECRET       "otp_secret"
#define KEY_SECURITY_CONFIG  "security_cfg"

// Device configuration
#define KEY_LOCK_CONFIG      "lock_cfg"
#define KEY_SENSOR_CONFIG    "sensor_cfg"
#define KEY_AUTO_LOCK_TIMEOUT "auto_lock"

// Logging
#define KEY_LOG_PREFIX       "log_"
#define KEY_EVENT_LOG        "event_log"

// ============================================================
// DATA STRUCTURES
// ============================================================

struct WiFiCredentials {
    char ssid[64];
    char password[64];
    char hostname[32];
    bool dhcp_enabled;
    uint32_t static_ip;
    uint32_t gateway;
    uint32_t subnet;
    uint32_t dns;
};

struct MQTTConfig {
    char broker[64];
    uint16_t port;
    char username[32];
    char password[32];
    bool tls_enabled;
    char ca_cert[2048];
    char client_cert[2048];
    char client_key[2048];
};

struct UserData {
    uint16_t id;
    char username[32];
    uint8_t access_level;
    bool enabled;
    uint32_t created_at;
    uint32_t last_access;
    uint8_t failed_attempts;
    uint32_t lockout_until;
};

struct LockConfig {
    uint16_t servo_min_pulse;
    uint16_t servo_max_pulse;
    uint8_t lock_angle;
    uint8_t unlock_angle;
    uint16_t auto_lock_timeout;
    uint8_t retry_attempts;
    bool buzzer_enabled;
    bool led_enabled;
};

struct SensorConfig {
    uint16_t temp_update_interval;
    uint16_t motion_timeout;
    bool door_sensor_enabled;
    bool pir_enabled;
    bool temp_humidity_enabled;
    uint8_t battery_check_interval;
};

// ============================================================
// EEPROM MANAGER CLASS
// ============================================================

class EEPROMManager {
private:
    Preferences preferences;
    bool initialized;
    std::map<String, String> cache;
    std::vector<String> dirty_keys;
    
    // Private methods
    void loadCache();
    void saveCache();
    String getKey(const char* key, const char* subkey = nullptr);
    bool isDirty(const String& key);
    void markDirty(const String& key);
    
public:
    EEPROMManager();
    ~EEPROMManager();
    
    // Initialization
    bool init();
    void deinit();
    bool isInitialized();
    void clear();
    
    // Basic operations
    bool putString(const char* key, const char* value);
    bool putString(const char* key, const String& value);
    String getString(const char* key, const char* defaultValue = "");
    bool getString(const char* key, char* buffer, size_t bufferSize);
    
    bool putInt(const char* key, int value);
    int getInt(const char* key, int defaultValue = 0);
    
    bool putUInt(const char* key, uint32_t value);
    uint32_t getUInt(const char* key, uint32_t defaultValue = 0);
    
    bool putFloat(const char* key, float value);
    float getFloat(const char* key, float defaultValue = 0.0f);
    
    bool putBool(const char* key, bool value);
    bool getBool(const char* key, bool defaultValue = false);
    
    bool putBytes(const char* key, const uint8_t* data, size_t length);
    bool getBytes(const char* key, uint8_t* data, size_t maxLength, size_t* actualLength);
    size_t getBytesLength(const char* key);
    
    bool remove(const char* key);
    bool exists(const char* key);
    
    // Batch operations
    void beginTransaction();
    void commitTransaction();
    void rollbackTransaction();
    
    // Structured data operations
    bool saveWiFiCredentials(const WiFiCredentials& creds);
    bool loadWiFiCredentials(WiFiCredentials& creds);
    
    bool saveMQTTConfig(const MQTTConfig& config);
    bool loadMQTTConfig(MQTTConfig& config);
    
    bool saveUser(uint16_t userId, const UserData& user);
    bool loadUser(uint16_t userId, UserData& user);
    bool deleteUser(uint16_t userId);
    std::vector<uint16_t> getAllUserIds();
    
    bool saveLockConfig(const LockConfig& config);
    bool loadLockConfig(LockConfig& config);
    
    bool saveSensorConfig(const SensorConfig& config);
    bool loadSensorConfig(SensorConfig& config);
    
    // Authentication data
    bool savePIN(uint16_t userId, const char* pin);
    String getPIN(uint16_t userId);
    bool deletePIN(uint16_t userId);
    
    bool saveRFID(uint16_t userId, uint32_t uid);
    uint32_t getRFID(uint16_t userId);
    bool deleteRFID(uint16_t userId);
    uint16_t findUserByRFID(uint32_t uid);
    
    bool saveFingerprintID(uint16_t userId, uint16_t fingerprintId);
    uint16_t getFingerprintID(uint16_t userId);
    bool deleteFingerprintID(uint16_t userId);
    uint16_t findUserByFingerprint(uint16_t fingerprintId);
    
    // Logging
    bool appendLog(const char* logData);
    bool getLogs(char* buffer, size_t maxSize);
    bool clearLogs();
    size_t getLogSize();
    
    // Security
    bool saveEncryptionKey(const uint8_t* key, size_t length);
    bool loadEncryptionKey(uint8_t* key, size_t maxLength);
    bool saveOTPSecret(const char* secret);
    String getOTPSecret();
    
    // System
    bool saveSystemConfig(const String& config);
    String getSystemConfig();
    
    // Statistics
    size_t getStorageUsage();
    size_t getFreeSpace();
    void printStats();
};

// ============================================================
// GLOBAL INSTANCE
// ============================================================

extern EEPROMManager Storage;

#endif // EEPROM_MANAGER_H
