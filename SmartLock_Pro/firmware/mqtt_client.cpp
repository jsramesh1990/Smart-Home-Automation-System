#include "include/mqtt_client.h"
#include "include/config.h"
#include <ArduinoJson.h>

MqttManager::MqttManager() : mqttClient(wifiClient) {
    customCallback = nullptr;
    connected = false;
    reconnectAttempts = 0;
    lastReconnectAttempt = 0;
}

void MqttManager::begin(const char* mqttServer, int mqttPort, const char* id) {
    strncpy(server, mqttServer, 63);
    port = mqttPort;
    strncpy(clientId, id, 31);
    
    mqttClient.setServer(server, port);
    mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) {
        this->callback(topic, payload, length);
    });
    
    DEBUG_PRINTF("MQTT configured: %s:%d\n", server, port);
}

void MqttManager::setCredentials(const char* user, const char* pass) {
    if (user && strlen(user) > 0) {
        strncpy(username, user, 31);
        if (pass) strncpy(password, pass, 31);
        hasCredentials = true;
    } else {
        hasCredentials = false;
    }
}

bool MqttManager::connect() {
    bool result;
    
    if (hasCredentials) {
        result = mqttClient.connect(clientId, username, password);
    } else {
        result = mqttClient.connect(clientId);
    }
    
    if (result) {
        connected = true;
        reconnectAttempts = 0;
        DEBUG_PRINTLN("MQTT connected");
        
        // Publish birth message
        StaticJsonDocument<128> birthMsg;
        birthMsg["event"] = "connected";
        birthMsg["device_id"] = clientId;
        birthMsg["timestamp"] = millis();
        
        String payload;
        serializeJson(birthMsg, payload);
        mqttClient.publish("smartlock/status", payload.c_str(), true);
        
    } else {
        DEBUG_PRINTF("MQTT connection failed, rc=%d\n", mqttClient.state());
    }
    
    return result;
}

bool MqttManager::reconnect() {
    if (connected) return true;
    
    unsigned long now = millis();
    if (now - lastReconnectAttempt < 5000) {
        return false;
    }
    lastReconnectAttempt = now;
    
    DEBUG_PRINTLN("Attempting MQTT reconnection...");
    return connect();
}

void MqttManager::loop() {
    if (!mqttClient.connected()) {
        reconnect();
    } else {
        mqttClient.loop();
    }
}

void MqttManager::callback(char* topic, byte* payload, unsigned int length) {
    char message[512];
    if (length < sizeof(message)) {
        memcpy(message, payload, length);
        message[length] = '\0';
        
        DEBUG_PRINTF("MQTT message [%s]: %s\n", topic, message);
        
        if (customCallback) {
            customCallback(topic, message);
        }
    }
}

bool MqttManager::publish(const char* topic, const char* payload, bool retained, int qos) {
    if (!connected && !mqttClient.connected()) {
        return false;
    }
    
    return mqttClient.publish(topic, payload, retained);
}

bool MqttManager::publishJson(const char* topic, JsonDocument& doc, bool retained) {
    String payload;
    serializeJson(doc, payload);
    return publish(topic, payload.c_str(), retained);
}

bool MqttManager::subscribe(const char* topic, int qos) {
    if (!connected && !mqttClient.connected()) {
        return false;
    }
    
    return mqttClient.subscribe(topic, qos);
}

void MqttManager::setCallback(MqttCallback cb) {
    customCallback = cb;
}

bool MqttManager::isConnected() {
    return mqttClient.connected();
}

void MqttManager::disconnect() {
    if (mqttClient.connected()) {
        // Publish death message
        StaticJsonDocument<128> deathMsg;
        deathMsg["event"] = "disconnected";
        deathMsg["device_id"] = clientId;
        deathMsg["timestamp"] = millis();
        
        String payload;
        serializeJson(deathMsg, payload);
        mqttClient.publish("smartlock/status", payload.c_str(), true);
        
        mqttClient.disconnect();
    }
    connected = false;
    DEBUG_PRINTLN("MQTT disconnected");
}

void MqttManager::publishLockState(bool locked) {
    StaticJsonDocument<256> stateMsg;
    stateMsg["device_id"] = clientId;
    stateMsg["state"] = locked ? "locked" : "unlocked";
    stateMsg["timestamp"] = millis();
    stateMsg["battery"] = 85; // Get from hardware
    
    publishJson("smartlock/state", stateMsg, true);
}

void MqttManager::publishHeartbeat() {
    StaticJsonDocument<256> heartbeat;
    heartbeat["device_id"] = clientId;
    heartbeat["timestamp"] = millis();
    heartbeat["uptime"] = millis() / 1000;
    heartbeat["rssi"] = WiFi.RSSI();
    heartbeat["free_heap"] = ESP.getFreeHeap();
    heartbeat["battery"] = 85;
    
    publishJson("smartlock/heartbeat", heartbeat, false);
}

void MqttManager::publishLog(const char* action, const char* method, bool success) {
    StaticJsonDocument<256> logMsg;
    logMsg["device_id"] = clientId;
    logMsg["action"] = action;
    logMsg["method"] = method;
    logMsg["success"] = success;
    logMsg["timestamp"] = millis();
    
    publishJson("smartlock/logs", logMsg, false);
}
