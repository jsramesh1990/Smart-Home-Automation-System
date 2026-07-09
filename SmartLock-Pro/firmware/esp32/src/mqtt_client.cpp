// mqtt_client.cpp - MQTT Client Implementation
#include "mqtt_client.h"
#include "config.h"
#include "constants.h"
#include "logger.h"
#include <ArduinoJson.h>

extern Logger logger;

MQTTClient::MQTTClient() {
    client = nullptr;
    connected = false;
    lastReconnectAttempt = 0;
}

bool MQTTClient::init() {
    logger.log(LOG_LEVEL_INFO, "MQTT", "Initializing...");
    
    // Create MQTT client
    if (!client) {
        client = new PubSubClient(wifiClient);
    }
    
    if (client) {
        client->setServer(MQTT_BROKER, MQTT_PORT);
        client->setCallback([this](char* topic, byte* payload, unsigned int length) {
            this->callback(topic, payload, length);
        });
        
        // Connect to MQTT broker
        if (connect()) {
            logger.log(LOG_LEVEL_INFO, "MQTT", "Connected to broker");
            return true;
        }
    }
    
    logger.log(LOG_LEVEL_ERROR, "MQTT", "Initialization failed");
    return false;
}

bool MQTTClient::connect() {
    if (!client) return false;
    
    // Generate client ID
    String clientId = DEVICE_ID;
    clientId += "-";
    clientId += String(random(0xffff), HEX);
    
    // Connect with credentials
    if (client->connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
        connected = true;
        subscribeToTopics();
        return true;
    }
    
    connected = false;
    return false;
}

void MQTTClient::subscribeToTopics() {
    if (!client) return;
    
    // Construct topic paths
    String commandsTopic = String(MQTT_TOPIC_PREFIX) + "/" + DEVICE_ID + "/commands/#";
    String configTopic = String(MQTT_TOPIC_PREFIX) + "/" + DEVICE_ID + "/config";
    String systemTopic = String(MQTT_TOPIC_PREFIX) + "/system/#";
    
    // Subscribe
    client->subscribe(commandsTopic.c_str());
    client->subscribe(configTopic.c_str());
    client->subscribe(systemTopic.c_str());
    
    logger.log(LOG_LEVEL_DEBUG, "MQTT", "Subscribed to topics");
}

void MQTTClient::callback(char* topic, byte* payload, unsigned int length) {
    // Convert payload to string
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    logger.log(LOG_LEVEL_DEBUG, "MQTT", "Received: %s -> %s", topic, message.c_str());
    
    // Parse command
    if (String(topic).endsWith("/commands/lock")) {
        handleLockCommand(message);
    } else if (String(topic).endsWith("/commands/unlock")) {
        handleUnlockCommand(message);
    } else if (String(topic).endsWith("/commands/status")) {
        handleStatusCommand();
    } else if (String(topic).endsWith("/commands/config")) {
        handleConfigCommand(message);
    }
}

void MQTTClient::handleLockCommand(String message) {
    logger.log(LOG_LEVEL_INFO, "MQTT", "Lock command received");
    // Trigger lock action
    // lockController.lock();
}

void MQTTClient::handleUnlockCommand(String message) {
    logger.log(LOG_LEVEL_INFO, "MQTT", "Unlock command received");
    // Trigger unlock action
    // lockController.unlock();
}

void MQTTClient::handleStatusCommand() {
    logger.log(LOG_LEVEL_INFO, "MQTT", "Status request received");
    // Publish current status
    // publishStatus();
}

void MQTTClient::handleConfigCommand(String message) {
    logger.log(LOG_LEVEL_INFO, "MQTT", "Config command received");
    // Parse and apply configuration
}

bool MQTTClient::publish(const char* subTopic, const char* payload) {
    if (!client || !connected) {
        return false;
    }
    
    String topic = String(MQTT_TOPIC_PREFIX) + "/" + DEVICE_ID + "/" + subTopic;
    return client->publish(topic.c_str(), payload);
}

bool MQTTClient::publish(const char* subTopic, const String& payload) {
    return publish(subTopic, payload.c_str());
}

bool MQTTClient::publishStatus(const char* key, int value) {
    StaticJsonDocument<64> doc;
    doc["value"] = value;
    doc["timestamp"] = millis();
    
    String payload;
    serializeJson(doc, payload);
    return publish("status", payload);
}

bool MQTTClient::publishStatus(const char* key, float value) {
    StaticJsonDocument<64> doc;
    doc["value"] = value;
    doc["timestamp"] = millis();
    
    String payload;
    serializeJson(doc, payload);
    return publish("status", payload);
}

bool MQTTClient::publishStatus(const char* key, bool value) {
    StaticJsonDocument<64> doc;
    doc["value"] = value;
    doc["timestamp"] = millis();
    
    String payload;
    serializeJson(doc, payload);
    return publish("status", payload);
}

bool MQTTClient::publishStatus(const char* key, const char* value) {
    StaticJsonDocument<128> doc;
    doc["value"] = value;
    doc["timestamp"] = millis();
    
    String payload;
    serializeJson(doc, payload);
    return publish("status", payload);
}

bool MQTTClient::publishEvent(const char* type, const char* message) {
    StaticJsonDocument<256> doc;
    doc["type"] = type;
    doc["message"] = message;
    doc["timestamp"] = millis();
    doc["device_id"] = DEVICE_ID;
    
    String payload;
    serializeJson(doc, payload);
    return publish("events", payload);
}

void MQTTClient::update() {
    if (!client) return;
    
    // Check connection
    if (!client->connected()) {
        if (millis() - lastReconnectAttempt > 5000) {
            lastReconnectAttempt = millis();
            if (connect()) {
                logger.log(LOG_LEVEL_INFO, "MQTT", "Reconnected");
            }
        }
        return;
    }
    
    // Process MQTT messages
    client->loop();
}

bool MQTTClient::isConnected() {
    return connected && client && client->connected();
}
