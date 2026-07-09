// eeprom_manager.cpp - EEPROM/Storage Manager Implementation
#include "eeprom_manager.h"
#include "logger.h"
#include <cstring>
#include <CRC32.h>

extern Logger logger;

// Global instance
EEPROMManager Storage;

// ============================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================

EEPROMManager::EEPROMManager() {
    initialized = false;
}

EEPROMManager::~EEPROMManager() {
    deinit();
}

// ============================================================
# INITIALIZATION
// ============================================================

bool EEPROMManager::init() {
    if (initialized) {
        return true;
    }
    
    logger.log(LOG_LEVEL_INFO, "Storage", "Initializing...");
    
    // Initialize Preferences
    preferences.begin(STORAGE_NAMESPACE, false);
    
    // Load cache
    loadCache();
    
    initialized = true;
    logger.log(LOG_LEVEL_INFO, "Storage", "Initialized successfully");
    logger.log(LOG_LEVEL_INFO, "Storage", "Usage: %u bytes", getStorageUsage());
    logger.log(LOG_LEVEL_INFO, "Storage", "Free: %u bytes", getFreeSpace());
    
    return true;
}

void EEPROMManager::deinit() {
    if (!initialized) {
        return;
    }
    
    // Save cache
    saveCache();
    
    // Close preferences
    preferences.end();
    
    initialized = false;
    logger.log(LOG_LEVEL_INFO, "Storage", "Deinitialized");
}

bool EEPROMManager::isInitialized() {
    return initialized;
}

void EEPROMManager::clear() {
    if (!initialized) {
        return;
    }
    
    logger.log(LOG_LEVEL_WARNING, "Storage", "Clearing all storage...");
    
    // Clear all keys
    preferences.clear();
    
    // Clear cache
    cache.clear();
    dirty_keys.clear();
    
    logger.log(LOG_LEVEL_INFO, "Storage", "Storage cleared");
}

// ============================================================
# CACHE MANAGEMENT
// ============================================================

void EEPROMManager::loadCache() {
    // Load all keys into cache
    // This is a simplified implementation
    // In production, you'd iterate through all keys
    
    // We'll lazily load keys on demand
}

void EEPROMManager::saveCache() {
    // Save dirty keys
    for (const String& key : dirty_keys) {
        if (cache.find(key) != cache.end()) {
            preferences.putString(key.c_str(), cache[key]);
        }
    }
    dirty_keys.clear();
}

String EEPROMManager::getKey(const char* key, const char* subkey) {
    if (subkey) {
        return String(key) + "_" + String(subkey);
    }
    return String(key);
}

bool EEPROMManager::isDirty(const String& key) {
    for (const String& dirty_key : dirty_keys) {
        if (dirty_key == key) {
            return true;
        }
    }
    return false;
}

void EEPROMManager::markDirty(const String& key) {
    if (!isDirty(key)) {
        dirty_keys.push_back(key);
    }
}

// ============================================================
# BASIC OPERATIONS - STRING
// ============================================================

bool EEPROMManager::putString(const char* key, const char* value) {
    if (!initialized) {
        return false;
    }
    
    String keyStr = key;
    cache[keyStr] = String(value);
    markDirty(keyStr);
    
    // Immediate write if not in transaction
    if (dirty_keys.size() > 10) {
        saveCache();
    }
    
    return true;
}

bool EEPROMManager::putString(const char* key, const String& value) {
    return putString(key, value.c_str());
}

String EEPROMManager::getString(const char* key, const char* defaultValue) {
    if (!initialized) {
        return String(defaultValue);
    }
    
    String keyStr = key;
    
    // Check cache first
    if (cache.find(keyStr) != cache.end()) {
        return cache[keyStr];
    }
    
    // Load from preferences
    String value = preferences.getString(key, defaultValue);
    cache[keyStr] = value;
    
    return value;
}

bool EEPROMManager::getString(const char* key, char* buffer, size_t bufferSize) {
    String value = getString(key);
    strncpy(buffer, value.c_str(), bufferSize - 1);
    buffer[bufferSize - 1] = '\0';
    return value.length() > 0;
}

// ============================================================
# BASIC OPERATIONS - NUMERIC
// ============================================================

bool EEPROMManager::putInt(const char* key, int value) {
    String keyStr = key;
    cache[keyStr] = String(value);
    markDirty(keyStr);
    return true;
}

int EEPROMManager::getInt(const char* key, int defaultValue) {
    String keyStr = key;
    
    if (cache.find(keyStr) != cache.end()) {
        return cache[keyStr].toInt();
    }
    
    int value = preferences.getInt(key, defaultValue);
    cache[keyStr] = String(value);
    return value;
}

bool EEPROMManager::putUInt(const char* key, uint32_t value) {
    String keyStr = key;
    cache[keyStr] = String(value);
    markDirty(keyStr);
    return true;
}

uint32_t EEPROMManager::getUInt(const char* key, uint32_t defaultValue) {
    String keyStr = key;
    
    if (cache.find(keyStr) != cache.end()) {
        return (uint32_t)cache[keyStr].toInt();
    }
    
    uint32_t value = preferences.getUInt(key, defaultValue);
    cache[keyStr] = String(value);
    return value;
}

bool EEPROMManager::putFloat(const char* key, float value) {
    String keyStr = key;
    cache[keyStr] = String(value, 6);
    markDirty(keyStr);
    return true;
}

float EEPROMManager::getFloat(const char* key, float defaultValue) {
    String keyStr = key;
    
    if (cache.find(keyStr) != cache.end()) {
        return cache[keyStr].toFloat();
    }
    
    float value = preferences.getFloat(key, defaultValue);
    cache[keyStr] = String(value, 6);
    return value;
}

bool EEPROMManager::putBool(const char* key, bool value) {
    String keyStr = key;
    cache[keyStr] = value ? "true" : "false";
    markDirty(keyStr);
    return true;
}

bool EEPROMManager::getBool(const char* key, bool defaultValue) {
    String keyStr = key;
    
    if (cache.find(keyStr) != cache.end()) {
        return cache[keyStr] == "true";
    }
    
    bool value = preferences.getBool(key, defaultValue);
    cache[keyStr] = value ? "true" : "false";
    return value;
}

// ============================================================
# BASIC OPERATIONS - BYTES
// ============================================================

bool EEPROMManager::putBytes(const char* key, const uint8_t* data, size_t length) {
    if (!initialized) {
        return false;
    }
    
    String keyStr = key;
    
    // Convert to base64 for storage
    String encoded = base64::encode(data, length);
    cache[keyStr] = encoded;
    markDirty(keyStr);
    
    return true;
}

bool EEPROMManager::getBytes(const char* key, uint8_t* data, size_t maxLength, size_t* actualLength) {
    String keyStr = key;
    String encoded;
    
    if (cache.find(keyStr) != cache.end()) {
        encoded = cache[keyStr];
    } else {
        encoded = preferences.getString(key, "");
        if (encoded.length() > 0) {
            cache[keyStr] = encoded;
        }
    }
    
    if (encoded.length() == 0) {
        if (actualLength) *actualLength = 0;
        return false;
    }
    
    // Decode from base64
    size_t decodedLength = base64::decode(encoded, data, maxLength);
    if (actualLength) *actualLength = decodedLength;
    
    return decodedLength > 0;
}

size_t EEPROMManager::getBytesLength(const char* key) {
    String keyStr = key;
    String encoded;
    
    if (cache.find(keyStr) != cache.end()) {
        encoded = cache[keyStr];
    } else {
        encoded = preferences.getString(key, "");
        if (encoded.length() > 0) {
            cache[keyStr] = encoded;
        }
    }
    
    return base64::decoded_length(encoded);
}

// ============================================================
# BASIC OPERATIONS - MISC
// ============================================================

bool EEPROMManager::remove(const char* key) {
    if (!initialized) {
        return false;
    }
    
    String keyStr = key;
    cache.erase(keyStr);
    preferences.remove(key);
    
    return true;
}

bool EEPROMManager::exists(const char* key) {
    if (!initialized) {
        return false;
    }
    
    String keyStr = key;
    
    if (cache.find(keyStr) != cache.end()) {
        return true;
    }
    
    return preferences.isKey(key);
}

// ============================================================
# TRANSACTIONS
// ============================================================

void EEPROMManager::beginTransaction() {
    // No-op for now - we use lazy saving
}

void EEPROMManager::commitTransaction() {
    saveCache();
}

void EEPROMManager::rollbackTransaction() {
    // Clear dirty keys
    dirty_keys.clear();
    
    // Reload cache from preferences
    cache.clear();
    loadCache();
}

// ============================================================
# STRUCTURED DATA - WIFI
// ============================================================

bool EEPROMManager::saveWiFiCredentials(const WiFiCredentials& creds) {
    bool success = true;
    
    success &= putString(KEY_WIFI_SSID, creds.ssid);
    success &= putString(KEY_WIFI_PASSWORD, creds.password);
    success &= putString(KEY_WIFI_HOSTNAME, creds.hostname);
    success &= putBool("wifi_dhcp", creds.dhcp_enabled);
    
    if (!creds.dhcp_enabled) {
        success &= putUInt("wifi_ip", creds.static_ip);
        success &= putUInt("wifi_gateway", creds.gateway);
        success &= putUInt("wifi_subnet", creds.subnet);
        success &= putUInt("wifi_dns", creds.dns);
    }
    
    return success;
}

bool EEPROMManager::loadWiFiCredentials(WiFiCredentials& creds) {
    bool success = true;
    
    success &= getString(KEY_WIFI_SSID, creds.ssid, sizeof(creds.ssid));
    success &= getString(KEY_WIFI_PASSWORD, creds.password, sizeof(creds.password));
    success &= getString(KEY_WIFI_HOSTNAME, creds.hostname, sizeof(creds.hostname));
    creds.dhcp_enabled = getBool("wifi_dhcp", true);
    
    if (!creds.dhcp_enabled) {
        creds.static_ip = getUInt("wifi_ip", 0);
        creds.gateway = getUInt("wifi_gateway", 0);
        creds.subnet = getUInt("wifi_subnet", 0);
        creds.dns = getUInt("wifi_dns", 0);
    }
    
    return success;
}

// ============================================================
# STRUCTURED DATA - MQTT
// ============================================================

bool EEPROMManager::saveMQTTConfig(const MQTTConfig& config) {
    bool success = true;
    
    success &= putString(KEY_MQTT_BROKER, config.broker);
    success &= putUInt(KEY_MQTT_PORT, config.port);
    success &= putString(KEY_MQTT_USERNAME, config.username);
    success &= putString(KEY_MQTT_PASSWORD, config.password);
    success &= putBool("mqtt_tls", config.tls_enabled);
    
    if (config.tls_enabled) {
        success &= putString("mqtt_ca_cert", config.ca_cert);
        success &= putString("mqtt_client_cert", config.client_cert);
        success &= putString("mqtt_client_key", config.client_key);
    }
    
    return success;
}

bool EEPROMManager::loadMQTTConfig(MQTTConfig& config) {
    bool success = true;
    
    success &= getString(KEY_MQTT_BROKER, config.broker, sizeof(config.broker));
    config.port = getUInt(KEY_MQTT_PORT, 1883);
    success &= getString(KEY_MQTT_USERNAME, config.username, sizeof(config.username));
    success &= getString(KEY_MQTT_PASSWORD, config.password, sizeof(config.password));
    config.tls_enabled = getBool("mqtt_tls", false);
    
    if (config.tls_enabled) {
        success &= getString("mqtt_ca_cert", config.ca_cert, sizeof(config.ca_cert));
        success &= getString("mqtt_client_cert", config.client_cert, sizeof(config.client_cert));
        success &= getString("mqtt_client_key", config.client_key, sizeof(config.client_key));
    }
    
    return success;
}

// ============================================================
# STRUCTURED DATA - USERS
// ============================================================

bool EEPROMManager::saveUser(uint16_t userId, const UserData& user) {
    char key[32];
    snprintf(key, sizeof(key), "%s%04d", KEY_USER_PREFIX, userId);
    
    // Save user data as JSON or binary
    // For simplicity, we'll use a string format
    String userStr = String(user.id) + "|" +
                     String(user.username) + "|" +
                     String(user.access_level) + "|" +
                     String(user.enabled ? "1" : "0") + "|" +
                     String(user.created_at) + "|" +
                     String(user.last_access) + "|" +
                     String(user.failed_attempts) + "|" +
                     String(user.lockout_until);
    
    bool success = putString(key, userStr);
    
    if (success) {
        // Update user count
        uint16_t count = getUInt(KEY_USER_COUNT, 0);
        // Check if this is a new user
        if (!exists(key)) {
            count++;
            putUInt(KEY_USER_COUNT, count);
        }
    }
    
    return success;
}

bool EEPROMManager::loadUser(uint16_t userId, UserData& user) {
    char key[32];
    snprintf(key, sizeof(key), "%s%04d", KEY_USER_PREFIX, userId);
    
    String userStr = getString(key);
    if (userStr.length() == 0) {
        return false;
    }
    
    // Parse user data
    int pos = 0;
    int nextPos;
    
    // id
    nextPos = userStr.indexOf('|', pos);
    if (nextPos < 0) return false;
    user.id = userStr.substring(pos, nextPos).toInt();
    pos = nextPos + 1;
    
    // username
    nextPos = userStr.indexOf('|', pos);
    if (nextPos < 0) return false;
    strncpy(user.username, userStr.substring(pos, nextPos).c_str(), sizeof(user.username) - 1);
    pos = nextPos + 1;
    
    // access_level
    nextPos = userStr.indexOf('|', pos);
    if (nextPos < 0) return false;
    user.access_level = userStr.substring(pos, nextPos).toInt();
    pos = nextPos + 1;
    
    // enabled
    nextPos = userStr.indexOf('|', pos);
    if (nextPos < 0) return false;
    user.enabled = userStr.substring(pos, nextPos).toInt() == 1;
    pos = nextPos + 1;
    
    // created_at
    nextPos = userStr.indexOf('|', pos);
    if (nextPos < 0) return false;
    user.created_at = userStr.substring(pos, nextPos).toInt();
    pos = nextPos + 1;
    
    // last_access
    nextPos = userStr.indexOf('|', pos);
    if (nextPos < 0) return false;
    user.last_access = userStr.substring(pos, nextPos).toInt();
    pos = nextPos + 1;
    
    // failed_attempts
    nextPos = userStr.indexOf('|', pos);
    if (nextPos < 0) return false;
    user.failed_attempts = userStr.substring(pos, nextPos).toInt();
    pos = nextPos + 1;
    
    // lockout_until
    user.lockout_until = userStr.substring(pos).toInt();
    
    return true;
}

bool EEPROMManager::deleteUser(uint16_t userId) {
    char key[32];
    snprintf(key, sizeof(key), "%s%04d", KEY_USER_PREFIX, userId);
    
    if (remove(key)) {
        uint16_t count = getUInt(KEY_USER_COUNT, 0);
        if (count > 0) {
            count--;
            putUInt(KEY_USER_COUNT, count);
        }
        return true;
    }
    
    return false;
}

std::vector<uint16_t> EEPROMManager::getAllUserIds() {
    std::vector<uint16_t> userIds;
    uint16_t count = getUInt(KEY_USER_COUNT, 0);
    
    for (uint16_t i = 1; i <= count; i++) {
        char key[32];
        snprintf(key, sizeof(key), "%s%04d", KEY_USER_PREFIX, i);
        if (exists(key)) {
            userIds.push_back(i);
        }
    }
    
    return userIds;
}

// ============================================================
# STRUCTURED DATA - LOCK CONFIG
// ============================================================

bool EEPROMManager::saveLockConfig(const LockConfig& config) {
    bool success = true;
    
    success &= putUInt("lock_servo_min", config.servo_min_pulse);
    success &= putUInt("lock_servo_max", config.servo_max_pulse);
    success &= putUInt("lock_angle", config.lock_angle);
    success &= putUInt("unlock_angle", config.unlock_angle);
    success &= putUInt(KEY_AUTO_LOCK_TIMEOUT, config.auto_lock_timeout);
    success &= putUInt("lock_retry", config.retry_attempts);
    success &= putBool("lock_buzzer", config.buzzer_enabled);
    success &= putBool("lock_led", config.led_enabled);
    
    return success;
}

bool EEPROMManager::loadLockConfig(LockConfig& config) {
    config.servo_min_pulse = getUInt("lock_servo_min", 500);
    config.servo_max_pulse = getUInt("lock_servo_max", 2400);
    config.lock_angle = getUInt("lock_angle", 0);
    config.unlock_angle = getUInt("unlock_angle", 90);
    config.auto_lock_timeout = getUInt(KEY_AUTO_LOCK_TIMEOUT, 30);
    config.retry_attempts = getUInt("lock_retry", 3);
    config.buzzer_enabled = getBool("lock_buzzer", true);
    config.led_enabled = getBool("lock_led", true);
    
    return true;
}

// ============================================================
# STRUCTURED DATA - SENSOR CONFIG
// ============================================================

bool EEPROMManager::saveSensorConfig(const SensorConfig& config) {
    bool success = true;
    
    success &= putUInt("sensor_temp_interval", config.temp_update_interval);
    success &= putUInt("sensor_motion_timeout", config.motion_timeout);
    success &= putBool("sensor_door", config.door_sensor_enabled);
    success &= putBool("sensor_pir", config.pir_enabled);
    success &= putBool("sensor_dht", config.temp_humidity_enabled);
    success &= putUInt("sensor_battery_interval", config.battery_check_interval);
    
    return success;
}

bool EEPROMManager::loadSensorConfig(SensorConfig& config) {
    config.temp_update_interval = getUInt("sensor_temp_interval", 60);
    config.motion_timeout = getUInt("sensor_motion_timeout", 30);
    config.door_sensor_enabled = getBool("sensor_door", true);
    config.pir_enabled = getBool("sensor_pir", false);
    config.temp_humidity_enabled = getBool("sensor_dht", false);
    config.battery_check_interval = getUInt("sensor_battery_interval", 300);
    
    return true;
}

// ============================================================
# AUTHENTICATION DATA - PIN
// ============================================================

bool EEPROMManager::savePIN(uint16_t userId, const char* pin) {
    char key[32];
    snprintf(key, sizeof(key), "%s%04d", KEY_PIN_PREFIX, userId);
    return putString(key, pin);
}

String EEPROMManager::getPIN(uint16_t userId) {
    char key[32];
    snprintf(key, sizeof(key), "%s%04d", KEY_PIN_PREFIX, userId);
    return getString(key);
}

bool EEPROMManager::deletePIN(uint16_t userId) {
    char key[32];
    snprintf(key, sizeof(key), "%s%04d", KEY_PIN_PREFIX, userId);
    return remove(key);
}

// ============================================================
# AUTHENTICATION DATA - RFID
// ============================================================

bool EEPROMManager::saveRFID(uint16_t userId, uint32_t uid) {
    char key[32];
    snprintf(key, sizeof(key), "%s%04d", KEY_RFID_PREFIX, userId);
    return putUInt(key, uid);
}

uint32_t EEPROMManager::getRFID(uint16_t userId) {
    char key[32];
    snprintf(key, sizeof(key), "%s%04d", KEY_RFID_PREFIX, userId);
    return getUInt(key, 0);
}

bool EEPROMManager::deleteRFID(uint16_t userId) {
    char key[32];
    snprintf(key, sizeof(key), "%s%04d", KEY_RFID_PREFIX, userId);
    return remove(key);
}

uint16_t EEPROMManager::findUserByRFID(uint32_t uid) {
    uint16_t count = getUInt(KEY_USER_COUNT, 0);
    
    for (uint16_t i = 1; i <= count; i++) {
        uint32_t storedUid = getRFID(i);
        if (storedUid == uid) {
            return i;
        }
    }
    
    return 0;
}

// ============================================================
# AUTHENTICATION DATA - FINGERPRINT
// ============================================================

bool EEPROMManager::saveFingerprintID(uint16_t userId, uint16_t fingerprintId) {
    char key[32];
    snprintf(key, sizeof(key), "%s%04d", KEY_FINGERPRINT_PREFIX, userId);
    return putUInt(key, fingerprintId);
}

uint16_t EEPROMManager::getFingerprintID(uint16_t userId) {
    char key[32];
    snprintf(key, sizeof(key), "%s%04d", KEY_FINGERPRINT_PREFIX, userId);
    return getUInt(key, 0);
}

bool EEPROMManager::deleteFingerprintID(uint16_t userId) {
    char key[32];
    snprintf(key, sizeof(key), "%s%04d", KEY_FINGERPRINT_PREFIX, userId);
    return remove(key);
}

uint16_t EEPROMManager::findUserByFingerprint(uint16_t fingerprintId) {
    uint16_t count = getUInt(KEY_USER_COUNT, 0);
    
    for (uint16_t i = 1; i <= count; i++) {
        uint16_t storedId = getFingerprintID(i);
        if (storedId == fingerprintId) {
            return i;
        }
    }
    
    return 0;
}

// ============================================================
# LOGGING
// ============================================================

bool EEPROMManager::appendLog(const char* logData) {
    String currentLogs = getString(KEY_EVENT_LOG, "");
    currentLogs += String(logData) + "\n";
    
    // Keep only last 50KB
    if (currentLogs.length() > 50000) {
        int cutPos = currentLogs.indexOf('\n', 1000);
        if (cutPos > 0) {
            currentLogs = currentLogs.substring(cutPos + 1);
        }
    }
    
    return putString(KEY_EVENT_LOG, currentLogs);
}

bool EEPROMManager::getLogs(char* buffer, size_t maxSize) {
    String logs = getString(KEY_EVENT_LOG, "");
    strncpy(buffer, logs.c_str(), maxSize - 1);
    buffer[maxSize - 1] = '\0';
    return logs.length() > 0;
}

bool EEPROMManager::clearLogs() {
    return remove(KEY_EVENT_LOG);
}

size_t EEPROMManager::getLogSize() {
    String logs = getString(KEY_EVENT_LOG, "");
    return logs.length();
}

// ============================================================
# SECURITY
// ============================================================

bool EEPROMManager::saveEncryptionKey(const uint8_t* key, size_t length) {
    return putBytes(KEY_ENCRYPTION_KEY, key, length);
}

bool EEPROMManager::loadEncryptionKey(uint8_t* key, size_t maxLength) {
    size_t actualLength;
    return getBytes(KEY_ENCRYPTION_KEY, key, maxLength, &actualLength);
}

bool EEPROMManager::saveOTPSecret(const char* secret) {
    return putString(KEY_OTP_SECRET, secret);
}

String EEPROMManager::getOTPSecret() {
    return getString(KEY_OTP_SECRET, "");
}

// ============================================================
# SYSTEM
// ============================================================

bool EEPROMManager::saveSystemConfig(const String& config) {
    return putString(KEY_SYSTEM_CONFIG, config);
}

String EEPROMManager::getSystemConfig() {
    return getString(KEY_SYSTEM_CONFIG, "");
}

// ============================================================
# STATISTICS
// ============================================================

size_t EEPROMManager::getStorageUsage() {
    // Estimate storage usage
    size_t usage = 0;
    
    // Count total keys
    // This is a simplified estimate
    usage += preferences.freeEntries() * 32; // Approximate per-entry overhead
    
    // Add data size
    uint16_t userCount = getUInt(KEY_USER_COUNT, 0);
    usage += userCount * 128; // Approximate per-user data
    
    return usage;
}

size_t EEPROMManager::getFreeSpace() {
    // ESP32 Preferences uses NVS with ~20KB total space
    const size_t TOTAL_SPACE = 20000;
    size_t used = getStorageUsage();
    return used < TOTAL_SPACE ? TOTAL_SPACE - used : 0;
}

void EEPROMManager::printStats() {
    Serial.println("=== Storage Statistics ===");
    Serial.printf("Usage: %u bytes\n", getStorageUsage());
    Serial.printf("Free: %u bytes\n", getFreeSpace());
    Serial.printf("Cache entries: %d\n", cache.size());
    Serial.printf("Dirty keys: %d\n", dirty_keys.size());
    Serial.printf("User count: %d\n", getUInt(KEY_USER_COUNT, 0));
    Serial.printf("Log size: %d bytes\n", getLogSize());
    Serial.println("============================");
}

// ============================================================
# BASE64 HELPER FUNCTIONS
// ============================================================

namespace base64 {
    static const char* b64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    String encode(const uint8_t* data, size_t length) {
        String result;
        int i = 0;
        int j = 0;
        uint8_t char_array_3[3];
        uint8_t char_array_4[4];
        
        while (length--) {
            char_array_3[i++] = *(data++);
            if (i == 3) {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;
                
                for (i = 0; i < 4; i++) {
                    result += b64chars[char_array_4[i]];
                }
                i = 0;
            }
        }
        
        if (i) {
            for (j = i; j < 3; j++) {
                char_array_3[j] = '\0';
            }
            
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            
            for (j = 0; j < i + 1; j++) {
                result += b64chars[char_array_4[j]];
            }
            
            while (i++ < 3) {
                result += '=';
            }
        }
        
        return result;
    }
    
    size_t decode(const String& encoded, uint8_t* data, size_t maxLength) {
        size_t in_len = encoded.length();
        size_t i = 0;
        size_t j = 0;
        size_t in_ = 0;
        uint8_t char_array_4[4], char_array_3[3];
        size_t output_len = 0;
        
        while (in_len-- && encoded[in_] != '=') {
            char_array_4[i++] = encoded[in_];
            in_++;
            if (i == 4) {
                for (i = 0; i < 4; i++) {
                    char_array_4[i] = (uint8_t)strchr(b64chars, char_array_4[i]) - (uint8_t*)b64chars;
                }
                
                char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
                
                for (i = 0; i < 3; i++) {
                    if (output_len < maxLength) {
                        data[output_len++] = char_array_3[i];
                    }
                }
                i = 0;
            }
        }
        
        if (i) {
            for (j = i; j < 4; j++) {
                char_array_4[j] = 0;
            }
            for (j = 0; j < 4; j++) {
                char_array_4[j] = (uint8_t)strchr(b64chars, char_array_4[j]) - (uint8_t*)b64chars;
            }
            
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            
            for (j = 0; j < i - 1; j++) {
                if (output_len < maxLength) {
                    data[output_len++] = char_array_3[j];
                }
            }
        }
        
        return output_len;
    }
    
    size_t decoded_length(const String& encoded) {
        size_t len = encoded.length();
        if (len == 0) return 0;
        
        size_t padding = 0;
        if (encoded[len - 1] == '=') padding++;
        if (encoded[len - 2] == '=') padding++;
        
        return (len * 3 / 4) - padding;
    }
} // namespace base64
