// error_codes.h - Error Code Definitions
#ifndef ERROR_CODES_H
#define ERROR_CODES_H

#include <Arduino.h>

// ============================================================
// ERROR CODE DEFINITIONS
// ============================================================
#define ERR_NONE                    0
#define ERR_UNKNOWN                 -1

// Hardware Errors (100-199)
#define ERR_HARDWARE_INIT           100
#define ERR_HARDWARE_SENSOR         101
#define ERR_HARDWARE_SERVO          102
#define ERR_HARDWARE_FINGERPRINT    103
#define ERR_HARDWARE_RFID           104
#define ERR_HARDWARE_KEYPAD         105
#define ERR_HARDWARE_POWER          106
#define ERR_HARDWARE_MEMORY         107

// Network Errors (200-299)
#define ERR_NETWORK_WIFI            200
#define ERR_NETWORK_MQTT            201
#define ERR_NETWORK_HTTP            202
#define ERR_NETWORK_DNS             203
#define ERR_NETWORK_TIMEOUT         204
#define ERR_NETWORK_SSL             205
#define ERR_NETWORK_OTA             206

// Authentication Errors (300-399)
#define ERR_AUTH_FINGERPRINT        300
#define ERR_AUTH_RFID               301
#define ERR_AUTH_PIN                302
#define ERR_AUTH_BLE                303
#define ERR_AUTH_OTP                304
#define ERR_AUTH_EXPIRED            305
#define ERR_AUTH_LOCKOUT            306
#define ERR_AUTH_INVALID            307
#define ERR_AUTH_USER_DISABLED      308

// Security Errors (400-499)
#define ERR_SEC_TAMPER              400
#define ERR_SEC_BOOT                401
#define ERR_SEC_ENCRYPT             402
#define ERR_SEC_AUTH                403
#define ERR_SEC_FLASH               404

// System Errors (500-599)
#define ERR_SYS_INIT                500
#define ERR_SYS_OTA                 501
#define ERR_SYS_WATCHDOG            502
#define ERR_SYS_STORAGE             503
#define ERR_SYS_TIMER               504

// ============================================================
// ERROR CODE DESCRIPTIONS
// ============================================================
static const char* ERROR_DESCRIPTIONS[] = {
    [ERR_NONE] = "No error",
    [ERR_UNKNOWN] = "Unknown error",
    [ERR_HARDWARE_INIT] = "Hardware initialization failed",
    [ERR_HARDWARE_SENSOR] = "Sensor read failed",
    [ERR_HARDWARE_SERVO] = "Servo control failed",
    [ERR_HARDWARE_FINGERPRINT] = "Fingerprint sensor error",
    [ERR_HARDWARE_RFID] = "RFID reader error",
    [ERR_HARDWARE_KEYPAD] = "Keypad input error",
    [ERR_HARDWARE_POWER] = "Power supply error",
    [ERR_HARDWARE_MEMORY] = "Memory allocation failed",
    [ERR_NETWORK_WIFI] = "WiFi connection failed",
    [ERR_NETWORK_MQTT] = "MQTT connection failed",
    [ERR_NETWORK_HTTP] = "HTTP request failed",
    [ERR_NETWORK_DNS] = "DNS resolution failed",
    [ERR_NETWORK_TIMEOUT] = "Network timeout",
    [ERR_NETWORK_SSL] = "SSL/TLS handshake failed",
    [ERR_NETWORK_OTA] = "OTA update failed",
    [ERR_AUTH_FINGERPRINT] = "Fingerprint authentication failed",
    [ERR_AUTH_RFID] = "RFID authentication failed",
    [ERR_AUTH_PIN] = "PIN authentication failed",
    [ERR_AUTH_BLE] = "BLE authentication failed",
    [ERR_AUTH_OTP] = "OTP validation failed",
    [ERR_AUTH_EXPIRED] = "Authentication expired",
    [ERR_AUTH_LOCKOUT] = "Account locked due to failed attempts",
    [ERR_AUTH_INVALID] = "Invalid credentials",
    [ERR_AUTH_USER_DISABLED] = "User account disabled",
    [ERR_SEC_TAMPER] = "Tamper detected",
    [ERR_SEC_BOOT] = "Secure boot verification failed",
    [ERR_SEC_ENCRYPT] = "Encryption/Decryption failed",
    [ERR_SEC_AUTH] = "Security authentication failed",
    [ERR_SEC_FLASH] = "Flash encryption error",
    [ERR_SYS_INIT] = "System initialization failed",
    [ERR_SYS_OTA] = "OTA update system error",
    [ERR_SYS_WATCHDOG] = "Watchdog timer triggered",
    [ERR_SYS_STORAGE] = "Storage access error",
    [ERR_SYS_TIMER] = "Timer error"
};

// ============================================================
// ERROR HANDLING FUNCTIONS
// ============================================================
inline const char* getErrorDescription(int errorCode) {
    if (errorCode >= 0 && errorCode < sizeof(ERROR_DESCRIPTIONS) / sizeof(ERROR_DESCRIPTIONS[0])) {
        return ERROR_DESCRIPTIONS[errorCode];
    }
    return "Unknown error code";
}

inline bool isCriticalError(int errorCode) {
    return (errorCode >= ERR_HARDWARE_INIT && errorCode < ERR_HARDWARE_MEMORY) ||
           (errorCode >= ERR_SEC_TAMPER && errorCode <= ERR_SEC_FLASH);
}

inline bool isAuthenticationError(int errorCode) {
    return errorCode >= ERR_AUTH_FINGERPRINT && errorCode <= ERR_AUTH_USER_DISABLED;
}

inline bool isNetworkError(int errorCode) {
    return errorCode >= ERR_NETWORK_WIFI && errorCode <= ERR_NETWORK_OTA;
}

// ============================================================
// ERROR LOGGING STRUCTURE
// ============================================================
struct ErrorRecord {
    uint32_t timestamp;
    int code;
    char message[128];
    char source[32];
    uint32_t line;
};

class ErrorHandler {
private:
    static const int MAX_ERRORS = 50;
    ErrorRecord errors[MAX_ERRORS];
    int errorCount = 0;
    
public:
    void logError(int code, const char* source, uint32_t line, const char* message = "") {
        if (errorCount < MAX_ERRORS) {
            errors[errorCount].timestamp = millis();
            errors[errorCount].code = code;
            errors[errorCount].line = line;
            strncpy(errors[errorCount].source, source, 31);
            if (message) {
                strncpy(errors[errorCount].message, message, 127);
            } else {
                strncpy(errors[errorCount].message, getErrorDescription(code), 127);
            }
            errorCount++;
        }
    }
    
    void clearErrors() {
        errorCount = 0;
    }
    
    ErrorRecord* getErrors(int* count) {
        *count = errorCount;
        return errors;
    }
    
    bool hasErrors() {
        return errorCount > 0;
    }
    
    void printErrors() {
        for (int i = 0; i < errorCount; i++) {
            Serial.printf("[ERROR] %s:%u - %s (Code: %d)\n",
                errors[i].source,
                errors[i].line,
                errors[i].message,
                errors[i].code
            );
        }
    }
};

#endif // ERROR_CODES_H
