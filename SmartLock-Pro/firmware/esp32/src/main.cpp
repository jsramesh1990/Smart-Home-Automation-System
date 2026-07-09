// main.cpp - SmartLock Pro Main Entry Point
#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include <ESPmDNS.h>
#include <Update.h>

#include "pins.h"
#include "constants.h"
#include "error_codes.h"
#include "config.h"
#include "wifi_manager.h"
#include "ble_manager.h"
#include "lock_controller.h"
#include "sensor_manager.h"
#include "mqtt_client.h"
#include "authentication/fingerprint.h"
#include "authentication/rfid.h"
#include "authentication/keypad.h"
#include "security/encryption.h"
#include "security/otp_generator.h"
#include "utilities/logger.h"

// ============================================================
// GLOBAL OBJECTS
// ============================================================
WiFiManagerClass wifiManager;
BLEManager bleManager;
LockController lockController;
SensorManager sensorManager;
MQTTClient mqttClient;
FingerprintAuth fingerprintAuth;
RFIDAuth rfidAuth;
KeypadAuth keypadAuth;
EncryptionManager encryption;
OTPGenerator otpGen;
Logger logger;

// System variables
SystemStatus systemStatus;
uint32_t lastHeartbeat = 0;
uint32_t lastSensorRead = 0;
bool systemInitialized = false;
ErrorHandler errorHandler;

// ============================================================
// FORWARD DECLARATIONS
// ============================================================
void initSystem();
void processAuthRequest(AuthMethod method, void* data);
void handleAccessGranted(uint16_t userId);
void handleAccessDenied(AuthMethod method, const char* reason);
void updateSystemStatus();
void publishStatus();
void systemLoop();

// ============================================================
// SYSTEM INITIALIZATION
// ============================================================
void initSystem() {
    Serial.begin(115200);
    Serial.println("\n\n╔══════════════════════════════════════════════════╗");
    Serial.println("║         SMARTLOCK PRO SYSTEM v" FIRMWARE_VERSION "      ║");
    Serial.println("║    Built: " BUILD_DATE "                           ║");
    Serial.println("╚══════════════════════════════════════════════════╝\n");
    
    // Initialize SPI
    SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI);
    
    // Initialize I2C
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    
    // Initialize peripherals
    pinMode(PIN_LED_RED, OUTPUT);
    pinMode(PIN_LED_GREEN, OUTPUT);
    pinMode(PIN_LED_BLUE, OUTPUT);
    pinMode(PIN_BUZZER, OUTPUT);
    pinMode(PIN_DOOR_SENSOR, INPUT_PULLUP);
    pinMode(PIN_PIR_SENSOR, INPUT);
    pinMode(PIN_BOOT_BTN, INPUT_PULLUP);
    
    // Set initial LED states
    digitalWrite(PIN_LED_RED, LOW);
    digitalWrite(PIN_LED_GREEN, LOW);
    digitalWrite(PIN_LED_BLUE, LOW);
    
    // Initialize subsystems
    logger.init(LOG_LEVEL_INFO);
    logger.log(LOG_LEVEL_INFO, "System", "Initializing SmartLock Pro v%s", FIRMWARE_VERSION);
    
    // WiFi
    if (!wifiManager.init()) {
        errorHandler.logError(ERR_NETWORK_WIFI, __FILE__, __LINE__);
        logger.log(LOG_LEVEL_ERROR, "WiFi", "Initialization failed");
    }
    
    // BLE
    if (!bleManager.init()) {
        errorHandler.logError(ERR_HARDWARE_INIT, __FILE__, __LINE__);
        logger.log(LOG_LEVEL_ERROR, "BLE", "Initialization failed");
    }
    
    // MQTT
    if (!mqttClient.init()) {
        errorHandler.logError(ERR_NETWORK_MQTT, __FILE__, __LINE__);
        logger.log(LOG_LEVEL_ERROR, "MQTT", "Initialization failed");
    }
    
    // Sensors
    if (!sensorManager.init()) {
        errorHandler.logError(ERR_HARDWARE_SENSOR, __FILE__, __LINE__);
        logger.log(LOG_LEVEL_ERROR, "Sensors", "Initialization failed");
    }
    
    // Authentication subsystems
    if (!fingerprintAuth.init()) {
        errorHandler.logError(ERR_HARDWARE_FINGERPRINT, __FILE__, __LINE__);
        logger.log(LOG_LEVEL_ERROR, "Fingerprint", "Initialization failed");
    }
    
    if (!rfidAuth.init()) {
        errorHandler.logError(ERR_HARDWARE_RFID, __FILE__, __LINE__);
        logger.log(LOG_LEVEL_ERROR, "RFID", "Initialization failed");
    }
    
    if (!keypadAuth.init()) {
        errorHandler.logError(ERR_HARDWARE_KEYPAD, __FILE__, __LINE__);
        logger.log(LOG_LEVEL_ERROR, "Keypad", "Initialization failed");
    }
    
    // Security
    if (!encryption.init()) {
        errorHandler.logError(ERR_SEC_ENCRYPT, __FILE__, __LINE__);
        logger.log(LOG_LEVEL_ERROR, "Encryption", "Initialization failed");
    }
    
    if (!otpGen.init()) {
        errorHandler.logError(ERR_AUTH_OTP, __FILE__, __LINE__);
        logger.log(LOG_LEVEL_ERROR, "OTP", "Initialization failed");
    }
    
    // Lock controller
    if (!lockController.init()) {
        errorHandler.logError(ERR_HARDWARE_SERVO, __FILE__, __LINE__);
        logger.log(LOG_LEVEL_ERROR, "Lock", "Initialization failed");
    }
    
    // System ready
    systemInitialized = true;
    systemStatus.lock_state = LOCK_LOCKED;
    systemStatus.door_open = false;
    systemStatus.battery_level = 100;
    systemStatus.wifi_rssi = 0;
    systemStatus.mqtt_connected = false;
    systemStatus.uptime = 0;
    systemStatus.free_heap = ESP.getFreeHeap();
    
    logger.log(LOG_LEVEL_INFO, "System", "Initialization complete");
    logger.log(LOG_LEVEL_INFO, "System", "Free Heap: %u bytes", systemStatus.free_heap);
    
    // Blink LEDs to indicate ready
    digitalWrite(PIN_LED_GREEN, HIGH);
    delay(100);
    digitalWrite(PIN_LED_GREEN, LOW);
    delay(100);
    digitalWrite(PIN_LED_GREEN, HIGH);
    delay(100);
    digitalWrite(PIN_LED_GREEN, LOW);
}

// ============================================================
# AUTHENTICATION HANDLING
// ============================================================
void processAuthRequest(AuthMethod method, void* data) {
    if (!systemInitialized) {
        logger.log(LOG_LEVEL_ERROR, "Auth", "System not initialized");
        return;
    }
    
    logger.log(LOG_LEVEL_DEBUG, "Auth", "Processing authentication request: %d", method);
    
    bool granted = false;
    uint16_t userId = 0;
    char reason[64] = "Unknown error";
    
    switch (method) {
        case AUTH_FINGERPRINT: {
            int fingerId = fingerprintAuth.authenticate();
            if (fingerId > 0) {
                granted = true;
                userId = fingerId;
                snprintf(reason, sizeof(reason), "Fingerprint match: ID %d", fingerId);
            } else {
                snprintf(reason, sizeof(reason), "Fingerprint not recognized");
            }
            break;
        }
        
        case AUTH_RFID: {
            uint32_t uid = rfidAuth.authenticate();
            if (uid > 0) {
                granted = true;
                userId = rfidAuth.getUserByUID(uid);
                snprintf(reason, sizeof(reason), "RFID match: UID %08X", uid);
            } else {
                snprintf(reason, sizeof(reason), "RFID not recognized");
            }
            break;
        }
        
        case AUTH_KEYPAD: {
            char* pin = (char*)data;
            if (keypadAuth.authenticate(pin)) {
                granted = true;
                userId = keypadAuth.getUserByPIN(pin);
                snprintf(reason, sizeof(reason), "PIN accepted");
            } else {
                snprintf(reason, sizeof(reason), "Invalid PIN");
            }
            break;
        }
        
        case AUTH_BLE: {
            // BLE authentication handled in BLE manager
            granted = bleManager.isAuthenticated();
            if (granted) {
                userId = bleManager.getAuthenticatedUser();
                snprintf(reason, sizeof(reason), "BLE authentication successful");
            } else {
                snprintf(reason, sizeof(reason), "BLE authentication failed");
            }
            break;
        }
        
        case AUTH_REMOTE: {
            // Remote authentication via MQTT/HTTP
            if (mqttClient.isAuthenticated()) {
                granted = true;
                userId = 1; // Remote admin
                snprintf(reason, sizeof(reason), "Remote authentication successful");
            } else {
                snprintf(reason, sizeof(reason), "Remote authentication failed");
            }
            break;
        }
        
        default:
            snprintf(reason, sizeof(reason), "Unknown authentication method");
            break;
    }
    
    if (granted && userId > 0) {
        handleAccessGranted(userId);
    } else {
        handleAccessDenied(method, reason);
    }
    
    // Log the attempt
    logger.log(LOG_LEVEL_INFO, "Auth", "User %d - %s: %s", 
               userId, granted ? "GRANTED" : "DENIED", reason);
}

void handleAccessGranted(uint16_t userId) {
    // Check if user is enabled
    User* user = getUserById(userId);
    if (user && !user->enabled) {
        handleAccessDenied(AUTH_UNKNOWN, "User disabled");
        return;
    }
    
    // Unlock the door
    if (lockController.unlock()) {
        systemStatus.lock_state = LOCK_UNLOCKED;
        digitalWrite(PIN_LED_GREEN, HIGH);
        digitalWrite(PIN_BUZZER, HIGH);
        delay(200);
        digitalWrite(PIN_BUZZER, LOW);
        delay(100);
        digitalWrite(PIN_BUZZER, HIGH);
        delay(200);
        digitalWrite(PIN_BUZZER, LOW);
        digitalWrite(PIN_LED_GREEN, LOW);
        
        // Update user last access
        if (user) {
            user->last_access = millis();
            user->failed_attempts = 0;
            saveUserData(user);
        }
        
        // Publish event
        publishAccessEvent(userId, true, "Access granted");
        
        // Schedule auto-lock
        setTimeout([]() {
            if (systemStatus.lock_state == LOCK_UNLOCKED) {
                lockController.lock();
                systemStatus.lock_state = LOCK_LOCKED;
                publishStatus();
                logger.log(LOG_LEVEL_INFO, "Lock", "Auto-lock triggered");
            }
        }, AUTO_LOCK_TIMEOUT * 1000);
        
    } else {
        logger.log(LOG_LEVEL_ERROR, "Lock", "Failed to unlock door");
    }
}

void handleAccessDenied(AuthMethod method, const char* reason) {
    // Flash red LED
    for (int i = 0; i < 3; i++) {
        digitalWrite(PIN_LED_RED, HIGH);
        delay(100);
        digitalWrite(PIN_LED_RED, LOW);
        delay(100);
    }
    
    // Buzzer
    digitalWrite(PIN_BUZZER, HIGH);
    delay(300);
    digitalWrite(PIN_BUZZER, LOW);
    
    // Log event
    publishAccessEvent(0, false, reason);
    logger.log(LOG_LEVEL_WARNING, "Auth", "Access denied: %s", reason);
    
    // Track failed attempts
    // If too many, lockout
}

// ============================================================
# STATUS MANAGEMENT
// ============================================================
void updateSystemStatus() {
    systemStatus.lock_state = lockController.getState();
    systemStatus.door_open = digitalRead(PIN_DOOR_SENSOR) == LOW;
    systemStatus.battery_level = sensorManager.getBatteryLevel();
    systemStatus.wifi_rssi = WiFi.RSSI();
    systemStatus.mqtt_connected = mqttClient.isConnected();
    systemStatus.uptime = millis() / 1000;
    systemStatus.free_heap = ESP.getFreeHeap();
    
    // Temperature & humidity
    if (millis() - lastSensorRead > TEMP_UPDATE_INTERVAL * 1000) {
        sensorManager.readTemperature(&systemStatus.temperature);
        sensorManager.readHumidity(&systemStatus.humidity);
        lastSensorRead = millis();
    }
}

void publishStatus() {
    if (!mqttClient.isConnected()) {
        return;
    }
    
    updateSystemStatus();
    
    // Publish all status topics
    mqttClient.publishStatus("lock", systemStatus.lock_state);
    mqttClient.publishStatus("door", systemStatus.door_open);
    mqttClient.publishStatus("battery", systemStatus.battery_level);
    mqttClient.publishStatus("wifi", systemStatus.wifi_rssi);
    mqttClient.publishStatus("uptime", systemStatus.uptime);
    
    if (systemStatus.temperature != 0) {
        mqttClient.publishStatus("temperature", systemStatus.temperature);
        mqttClient.publishStatus("humidity", systemStatus.humidity);
    }
}

void publishAccessEvent(uint16_t userId, bool granted, const char* reason) {
    if (!mqttClient.isConnected()) {
        return;
    }
    
    StaticJsonDocument<256> doc;
    doc["user_id"] = userId;
    doc["granted"] = granted;
    doc["reason"] = reason;
    doc["timestamp"] = time(nullptr);
    doc["device_id"] = DEVICE_ID;
    
    String payload;
    serializeJson(doc, payload);
    mqttClient.publish("events/access", payload);
}

// ============================================================
# MAIN SYSTEM LOOP
// ============================================================
void systemLoop() {
    // Update WiFi
    wifiManager.update();
    
    // Update MQTT
    mqttClient.update();
    
    // Update BLE
    bleManager.update();
    
    // Read keypad
    String pin = keypadAuth.readInput();
    if (pin.length() > 0) {
        processAuthRequest(AUTH_KEYPAD, (void*)pin.c_str());
    }
    
    // Handle fingerprint auto-detection
    if (fingerprintAuth.isFingerDetected()) {
        processAuthRequest(AUTH_FINGERPRINT, nullptr);
    }
    
    // Handle RFID auto-detection
    if (rfidAuth.isCardDetected()) {
        processAuthRequest(AUTH_RFID, nullptr);
    }
    
    // Check door state change
    static bool lastDoorState = false;
    bool currentDoorState = digitalRead(PIN_DOOR_SENSOR) == LOW;
    if (currentDoorState != lastDoorState) {
        logger.log(LOG_LEVEL_INFO, "Sensor", "Door %s", 
                   currentDoorState ? "opened" : "closed");
        mqttClient.publishStatus("door", currentDoorState);
        lastDoorState = currentDoorState;
    }
    
    // Check motion
    if (digitalRead(PIN_PIR_SENSOR) == HIGH) {
        mqttClient.publishEvent("motion", "detected");
    }
    
    // Periodic status update
    if (millis() - lastHeartbeat > 30000) { // Every 30 seconds
        publishStatus();
        lastHeartbeat = millis();
        
        // Check for OTA updates
        // checkForOTA();
        
        // Memory check
        if (ESP.getFreeHeap() < 10000) {
            logger.log(LOG_LEVEL_WARNING, "System", "Low memory: %u bytes", 
                       ESP.getFreeHeap());
        }
    }
    
    // Handle watchdog
    esp_task_wdt_reset();
}

// ============================================================
# ARDUINO ENTRY POINTS
// ============================================================
void setup() {
    initSystem();
}

void loop() {
    systemLoop();
    
    // Allow other tasks to run
    delay(10);
}

// ============================================================
# INTERRUPT HANDLERS
// ============================================================
void IRAM_ATTR doorSensorISR() {
    // Handle door state change in ISR
    // Note: Keep this minimal, defer processing to loop
    static unsigned long lastInterrupt = 0;
    if (millis() - lastInterrupt > 50) { // Debounce
        lastInterrupt = millis();
        // Set flag for loop processing
        // doorStateChanged = true;
    }
}

void IRAM_ATTR tamperISR() {
    // Handle tamper detection
    errorHandler.logError(ERR_SEC_TAMPER, "ISR", __LINE__);
    // Trigger alarm
}
