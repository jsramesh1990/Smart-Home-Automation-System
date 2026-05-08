#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>

class MqttManager {
private:
    WiFiClient wifiClient;
    PubSubClient mqttClient;
    
    char server[64];
    int port;
    char clientId[32];
    char username[32];
    char password[32];
    bool hasCredentials;
    
    bool connected;
    int reconnectAttempts;
    unsigned long lastReconnectAttempt;
    
    MqttCallback customCallback;
    
    void callback(char* topic, byte* payload, unsigned int length);
    bool reconnect();
    
public:
    MqttManager();
    
    void begin(const char* mqttServer, int mqttPort, const char* id);
    void setCredentials(const char* user, const char* pass);
    bool connect();
    void loop();
    void disconnect();
    
    bool publish(const char* topic, const char* payload, bool retained = false, int qos = 0);
    bool publishJson(const char* topic, JsonDocument& doc, bool retained = false);
    bool subscribe(const char* topic, int qos = 0);
    
    void setCallback(MqttCallback cb);
    bool isConnected();
    
    void publishLockState(bool locked);
    void publishHeartbeat();
    void publishLog(const char* action, const char* method, bool success);
    
    int getState() { return mqttClient.state(); }
};

typedef void (*MqttCallback)(const char* topic, const char* payload);

#endif
