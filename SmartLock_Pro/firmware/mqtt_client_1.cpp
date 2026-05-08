#include "include/mqtt_client.h"
#include "include/config.h"

MqttManager::MqttManager() : mqttClient(wifiClient) {
    customCallback = nullptr;
}

void MqttManager::begin(const char* mqttServer, int mqttPort, const char* id) {
    strncpy(server, mqttServer, 63);
    port = mqttPort;
    strncpy(clientId, id, 31);
    
    mqttClient.setServer(server, port);
    mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) {
        this->callback(topic, payload, length);
    });
}

void MqttManager::setCredentials(const char* user, const char* pass) {
    strncpy(username, user, 31);
    strncpy(password, pass, 31);
}

bool MqttManager::connect() {
    if (strlen(username) > 0) {
        return mqttClient.connect(clientId, username, password);
    } else {
        return mqttClient.connect(clientId);
    }
}

bool MqttManager::reconnect() {
    if (mqttClient.connected()) {
        return true;
    }
    
    for (int i = 0; i < 5; i++) {
        if (connect()) {
            Serial.println("MQTT connected");
            // Resubscribe to topics
            subscribe("smartlock/+/command");
            subscribe("smartlock/+/config");
            return true;
        }
        delay(2000);
    }
    return false;
}

void MqttManager::loop() {
    if (!mqttClient.connected()) {
        reconnect();
    }
    mqttClient.loop();
}

void MqttManager::callback(char* topic, byte* payload, unsigned int length) {
    char message[256];
    if (length < sizeof(message)) {
        memcpy(message, payload, length);
        message[length] = '\0';
        
        if (customCallback) {
            customCallback(topic, message);
        }
    }
}

bool MqttManager::publish(const char* topic, const char* payload, bool retained) {
    return mqttClient.publish(topic, payload, retained);
}

bool MqttManager::subscribe(const char* topic, int qos) {
    return mqttClient.subscribe(topic, qos);
}

void MqttManager::setCallback(MqttCallback cb) {
    customCallback = cb;
}

bool MqttManager::isConnected() {
    return mqttClient.connected();
}

void MqttManager::disconnect() {
    mqttClient.disconnect();
}
