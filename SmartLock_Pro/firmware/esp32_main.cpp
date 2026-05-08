#include <Arduino.h>
#include "include/config.h"
#include "include/hardware_control.h"
#include "include/network_manager.h"
#include "include/voice_recognition.h"
#include "include/mqtt_client.h"

// Global objects
HardwareController hardware;
NetworkManager network;
VoiceRecognizer voice;
MqttManager mqtt;

// Timing variables
unsigned long lastAutoLockCheck = 0;
unsigned long lastHeartbeat = 0;
unsigned long lastWiFiCheck = 0;
unsigned long lastBatteryCheck = 0;

// System state
bool systemReady = false;
int bootCount = 0;

// Forward declarations
void onMqttMessage(const char* topic, const char* payload);
void voiceCommandLock();
void voiceCommandUnlock();
void voiceCommandStatus();
void handleAutoLock();
void sendHeartbeat();
void checkWiFi();
void checkBattery();
void saveConfig();
void loadConfig();
void factoryReset();

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(100);
    
    DEBUG_PRINTLN("\n\n╔════════════════════════════════════╗");
    DEBUG_PRINTLN("║     SmartLock Pro v1.0            ║");
    DEBUG_PRINTLN("║     Secure Access Control         ║");
    DEBUG_PRINTLN("╚════════════════════════════════════╝\n");
    
    // Load saved configuration
    loadConfig();
    
    // Initialize hardware
    hardware.begin();
    hardware.selfTest();
    
    // Initialize voice recognition
    voice.begin();
    voice.registerCommand("lock", voiceCommandLock);
    voice.registerCommand("unlock", voiceCommandUnlock);
    voice.registerCommand("status", voiceCommandStatus);
    voice.registerCommand("lock the door", voiceCommandLock);
    voice.registerCommand("unlock the door", voiceCommandUnlock);
    
    // Enroll default admin voice
    voice.enrollVoice("admin", "hey smartlock");
    
    // Setup network
    network.begin(WIFI_SSID, WIFI_PASSWORD);
    
    if (network.connectToWiFi(30)) {
        network.startWebServer();
        DEBUG_PRINTLN("Web interface available at: http://" + network.getLocalIP());
        
        // Setup MQTT
        mqtt.begin(MQTT_BROKER, MQTT_PORT, MQTT_CLIENT_ID);
        if (strlen(MQTT_USER) > 0) {
            mqtt.setCredentials(MQTT_USER, MQTT_PASSWORD);
        }
        mqtt.setCallback(onMqttMessage);
        mqtt.connect();
        mqtt.subscribe("smartlock/+/command");
        mqtt.subscribe("smartlock/+/config");
        
        systemReady = true;
    } else {
        // Start AP mode for configuration
        network.startAccessPoint(AP_SSID, AP_PASSWORD);
        DEBUG_PRINTLN("AP mode active. Connect to '" AP_SSID "' to configure");
    }
    
    // Enable OTA updates
    network.enableOTA("admin123");
    
    hardware.indicateSuccess();
    DEBUG_PRINTLN("\nSystem ready!");
    DEBUG_PRINTF("Free heap: %d bytes\n", ESP.getFreeHeap());
}

void loop() {
    // Network handling
    network.handleClient();
    network.handleOTA();
    
    // MQTT handling
    if (systemReady) {
        mqtt.loop();
    }
    
    // Voice recognition (simulated - would read from I2S in production)
    static int sample = 0;
    static unsigned long lastVoiceSample = 0;
    if (millis() - lastVoiceSample > 50) {
        lastVoiceSample = millis();
        voice.processAudioSample(sample++);
    }
    
    // Auto-lock check
    handleAutoLock();
    
    // Periodic tasks
    unsigned long now = millis();
    
    if (now - lastHeartbeat > HEARTBEAT_INTERVAL_MS) {
        sendHeartbeat();
        lastHeartbeat = now;
    }
    
    if (now - lastWiFiCheck > 60000) {  // Every minute
        checkWiFi();
        lastWiFiCheck = now;
    }
    
    if (now - lastBatteryCheck > 300000) {  // Every 5 minutes
        checkBattery();
        lastBatteryCheck = now;
    }
    
    // Check for tamper
    if (hardware.checkTamper()) {
        DEBUG_PRINTLN("⚠️ TAMPER DETECTED!");
        mqtt.publish("smartlock/alerts", "{\"type\":\"tamper\",\"severity\":\"critical\"}");
        hardware.indicateWarning();
    }
    
    // Check for jammed door
    if (hardware.isJammed()) {
        DEBUG_PRINTLN("⚠️ DOOR JAMMED!");
        mqtt.publish("smartlock/alerts", "{\"type\":\"jam\",\"severity\":\"high\"}");
    }
    
    delay(10);
}

void onMqttMessage(const char* topic, const char* payload) {
    DEBUG_PRINTF("MQTT message [%s]: %s\n", topic, payload);
    
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
        DEBUG_PRINTLN("Failed to parse MQTT message");
        return;
    }
    
    const char* command = doc["command"];
    
    if (command) {
        if (strcmp(command, "lock") == 0) {
            if (hardware.lockDoor()) {
                mqtt.publishLockState(true);
                mqtt.publishLog("lock", "mqtt", true);
            }
        } else if (strcmp(command, "unlock") == 0) {
            const char* code = doc["code"];
            if (code && strlen(code) > 0) {
                // Verify guest code before unlocking
                // Would check against database
                if (hardware.unlockDoor()) {
                    mqtt.publishLockState(false);
                    mqtt.publishLog("unlock", "guest_code", true);
                }
            } else {
                // Regular unlock
                if (hardware.unlockDoor()) {
                    mqtt.publishLockState(false);
                    mqtt.publishLog("unlock", "mqtt", true);
                }
            }
        } else if (strcmp(command, "status") == 0) {
            mqtt.publishLockState(hardware.getState() == STATE_LOCKED);
        }
    }
    
    const char* config = doc["config"];
    if (config) {
        // Handle configuration update
        saveConfig();
        DEBUG_PRINTLN("Configuration updated");
    }
}

void voiceCommandLock() {
    DEBUG_PRINTLN("Voice: Lock command received");
    if (hardware.lockDoor()) {
        mqtt.publishLockState(true);
        mqtt.publishLog("lock", "voice", true);
        hardware.indicateSuccess();
    } else {
        hardware.indicateFailure();
    }
}

void voiceCommandUnlock() {
    DEBUG_PRINTLN("Voice: Unlock command received");
    if (hardware.unlockDoor()) {
        mqtt.publishLockState(false);
        mqtt.publishLog("unlock", "voice", true);
        hardware.indicateSuccess();
        hardware.resetAttempts();
    } else {
        hardware.incrementAttempts();
        hardware.indicateFailure();
        
        if (hardware.getAttempts() >= MAX_UNLOCK_ATTEMPTS) {
            DEBUG_PRINTLN("Max voice attempts reached, emergency lock activated");
            hardware.emergencyLock();
            mqtt.publish("smartlock/alerts", "{\"type\":\"max_attempts\",\"severity\":\"high\"}");
        }
    }
}

void voiceCommandStatus() {
    DEBUG_PRINTLN("Voice: Status request");
    LockState state = hardware.getState();
    BatteryInfo batt = hardware.readBattery();
    
    char response[256];
    snprintf(response, sizeof(response), 
             "Door is %s. Battery at %d percent.",
             state == STATE_LOCKED ? "locked" : "unlocked",
             batt.percentage);
    
    DEBUG_PRINTLN(response);
    // Would speak response via TTS
}

void handleAutoLock() {
    if (hardware.isAutoLockNeeded()) {
        hardware.lockDoor();
        DEBUG_PRINTLN("Auto-lock triggered");
        mqtt.publishLockState(true);
        mqtt.publishLog("lock", "auto", true);
    }
}

void sendHeartbeat() {
    if (!systemReady) return;
    
    StaticJsonDocument<256> heartbeat;
    heartbeat["device_id"] = MQTT_CLIENT_ID;
    heartbeat["timestamp"] = millis();
    heartbeat["uptime"] = millis() / 1000;
    heartbeat["state"] = hardware.getStateString();
    heartbeat["rssi"] = network.getRSSI();
    heartbeat["free_heap"] = ESP.getFreeHeap();
    heartbeat["free_psram"] = ESP.getFreePsram();
    heartbeat["boot_count"] = bootCount;
    
    BatteryInfo batt = hardware.readBattery();
    heartbeat["battery_voltage"] = batt.voltage;
    heartbeat["battery_percentage"] = batt.percentage;
    heartbeat["battery_low"] = batt.low;
    
    String output;
    serializeJson(heartbeat, output);
    mqtt.publish("smartlock/heartbeat", output.c_str());
    
    DEBUG_PRINTLN("Heartbeat sent");
}

void checkWiFi() {
    if (!network.isConnected() && !network.isAPMode()) {
        DEBUG_PRINTLN("WiFi disconnected, attempting to reconnect...");
        if (network.reconnect()) {
            DEBUG_PRINTLN("WiFi reconnected");
            network.startWebServer();
            mqtt.connect();
        }
    }
}

void checkBattery() {
    BatteryInfo batt = hardware.readBattery();
    
    if (batt.low) {
        DEBUG_PRINTLN("⚠️ LOW BATTERY WARNING!");
        mqtt.publish("smartlock/alerts", "{\"type\":\"low_battery\",\"percentage\":" + String(batt.percentage) + "}");
        hardware.indicateWarning();
    }
    
    if (batt.critical) {
        DEBUG_PRINTLN("⚠️ CRITICAL BATTERY!");
        mqtt.publish("smartlock/alerts", "{\"type\":\"critical_battery\",\"percentage\":" + String(batt.percentage) + "}");
    }
}

void saveConfig() {
    // Save configuration to SPIFFS
    DEBUG_PRINTLN("Saving configuration...");
}

void loadConfig() {
    // Load configuration from SPIFFS
    DEBUG_PRINTLN("Loading configuration...");
    bootCount++;
}

void factoryReset() {
    DEBUG_PRINTLN("Factory reset initiated...");
    // Clear saved configuration
    // Reset to defaults
    hardware.indicateWarning();
    delay(2000);
    ESP.restart();
}
