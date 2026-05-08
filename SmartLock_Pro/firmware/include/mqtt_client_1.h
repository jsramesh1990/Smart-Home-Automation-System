#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>

struct MqttMessage {
    char topic[64];
    char payload[256];
    int qos;
    bool retained;
};

typedef void (*MqttCallback)(const char* topic, const char* payload);

class MqttManager {
private:
    WiFiClient wifiClient;
    PubSubClient mqttClient;
    MqttCallback customCallback;
    
    char server[64];
    int port;
    char clientId[32];
    char username[32];
    char password[32];
    
    bool reconnect();
    void callback(char* topic, byte* payload, unsigned int length);
    
public:
    MqttManager();
    void begin(const char* mqttServer, int mqttPort, const char* id);
    void setCredentials(const char* user, const char* pass);
    bool connect();
    void loop();
    bool publish(const char* topic, const char* payload, bool retained = false);
    bool subscribe(const char* topic, int qos = 0);
    void setCallback(MqttCallback cb);
    bool isConnected();
    void disconnect();
};

#endif
