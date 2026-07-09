// wifi_manager.h - WiFi Manager Header
#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>

class WiFiManagerClass {
private:
    bool isConnected;
    int retryCount;
    char mqttBroker[64];
    int mqttPort;
    
    bool connect();
    void startConfigPortal();
    void saveCredentials(const char* ssid, const char* password);
    
public:
    WiFiManagerClass();
    bool init();
    bool update();
    bool isConnectedToWiFi();
    String getIP();
    int getRSSI();
    String getMAC();
};

#endif // WIFI_MANAGER_H
