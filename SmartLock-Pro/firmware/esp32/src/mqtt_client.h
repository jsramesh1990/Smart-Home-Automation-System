// mqtt_client.h - MQTT Client Header
#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>

class MQTTClient {
private:
    WiFiClient wifiClient;
    PubSubClient* client;
    bool connected;
    unsigned long lastReconnectAttempt;
    
    void subscribeToTopics();
    void handleLockCommand(String message);
    void handleUnlockCommand(String message);
    void handleStatusCommand();
    void handleConfigCommand(String message);
    
public:
    MQTTClient();
    bool init();
    bool connect();
    void callback(char* topic, byte* payload, unsigned int length);
    bool publish(const char* subTopic, const char* payload);
    bool publish(const char* subTopic, const String& payload);
    bool publishStatus(const char* key, int value);
    bool publishStatus(const char* key, float value);
    bool publishStatus(const char* key, bool value);
    bool publishStatus(const char* key, const char* value);
    bool publishEvent(const char* type, const char* message);
    void update();
    bool isConnected();
};

#endif // MQTT_CLIENT_H
