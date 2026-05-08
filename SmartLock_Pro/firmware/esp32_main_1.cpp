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
unsigned long lastAutoLockCheck = 0;
unsigned long lastHeartbeat = 0;

// Forward declarations
void onMqttMessage(const char* topic, const char* payload);
void voiceCommandLock();
void voiceCommandUnlock();
void voiceCommandStatus();
void handleAutoLock();
void sendHeartbeat();
void processCommands();

void setup() {
    Serial.begin(115200);
    Serial.println("\n\nSmartLock Pro Starting...");
    
    // Initialize hardware
    hardware.begin();
    
    // Initialize voice recognition
    voice.begin();
    voice.registerCommand("lock", voiceCommandLock);
    voice.registerCommand("unlock", voiceCommandUnlock);
    voice.registerCommand("status", voiceCommandStatus);
    
    // Enroll default user (in production, do this securely)
    voice.enrollVoice("admin", "hey smartlock");
    
    // Setup network
    network.begin(WIFI_SSID, WIFI_PASSWORD);
    
    if (network.connectToWiFi(30)) {
        network.startWebServer();
        
        // Setup MQTT
        mqtt.begin(MQTT_BROKER, MQTT_PORT, MQTT_CLIENT_ID);
        mqtt.setCredentials("", "");  // Add credentials if needed
        mqtt.setCallback(onMqttMessage);
        mqtt.connect();
        mqtt.subscribe("smartlock/+/command");
    } else {
        // Start AP mode for configuration
        network.startAccessPoint("SmartLock_Config", "12345678");
    }
    
    Serial.println("System ready");
    hardware.indicateSuccess();
}

void loop() {
    // Handle network clients
    network.handleClient();
    
    // Handle MQTT
    mqtt.loop();
    
    // Process voice recognition
    // In production, read from I2S microphone
    static int sample = 0;
    voice.processAudioSample(sample++);
    
    // Auto-lock check
    handleAutoLock();
    
    // Send heartbeat every 30 seconds
    if (millis() - lastHeartbeat > 30000) {
        sendHeartbeat();
        lastHeartbeat = millis();
    }
    
    // Check for tamper
    if (hardware.checkTamper()) {
        Serial.println("TAMPER DETECTED!");
        mqtt.publish("smartlock/alerts", "{\"type\":\"tamper\",\"timestamp\":" + String(millis()) + "}");
    }
    
    delay(10);
}

void onMqttMessage(const char* topic, const char* payload) {
    Serial.print("MQTT message on ");
    Serial.print(topic);
    Serial.print(": ");
    Serial.println(payload);
    
    // Parse and handle command
    if (strstr(payload, "\"command\":\"lock\"")) {
        hardware.lockDoor();
        mqtt.publish("smartlock/status", "{\"state\":\"locked\"}");
    } else if (strstr(payload, "\"command\":\"unlock\"")) {
        hardware.unlockDoor();
        mqtt.publish("smartlock/status", "{\"state\":\"unlocked\"}");
    }
}

void voiceCommandLock() {
    Serial.println("Voice: Lock command received");
    if (hardware.lockDoor()) {
        mqtt.publish("smartlock/voice", "{\"command\":\"lock\",\"status\":\"success\"}");
    }
}

void voiceCommandUnlock() {
    Serial.println("Voice: Unlock command received");
    if (hardware.unlockDoor()) {
        mqtt.publish("smartlock/voice", "{\"command\":\"unlock\",\"status\":\"success\"}");
    } else {
        hardware.incrementAttempts();
    }
}

void voiceCommandStatus() {
    Serial.println("Voice: Status command received");
    // Speak status (in production, use text-to-speech)
}

void handleAutoLock() {
    if (hardware.isAutoLockNeeded()) {
        hardware.lockDoor();
        Serial.println("Auto-lock triggered");
        mqtt.publish("smartlock/status", "{\"state\":\"locked\",\"reason\":\"auto\"}");
    }
}

void sendHeartbeat() {
    StaticJsonDocument<256> heartbeat;
    heartbeat["device_id"] = MQTT_CLIENT_ID;
    heartbeat["timestamp"] = millis();
    heartbeat["battery"] = hardware.getBatteryPercentage();
    heartbeat["state"] = hardware.getState() == LOCKED ? "locked" : "unlocked";
    heartbeat["rssi"] = network.getRSSI();
    
    String output;
    serializeJson(heartbeat, output);
    mqtt.publish("smartlock/heartbeat", output.c_str());
}

void processCommands() {
    // Process serial commands for testing
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        
        if (cmd == "lock") {
            hardware.lockDoor();
        } else if (cmd == "unlock") {
            hardware.unlockDoor();
        } else if (cmd == "status") {
            Serial.print("State: ");
            Serial.println(hardware.getState() == LOCKED ? "Locked" : "Unlocked");
            Serial.print("Battery: ");
            Serial.print(hardware.getBatteryPercentage());
            Serial.println("%");
        }
    }
}
