#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

class NetworkManager {
private:
    WebServer server;
    char ssid[64];
    char password[64];
    IPAddress localIP;
    bool apMode;
    
    void handleRoot();
    void handleApiStatus();
    void handleApiLock();
    void handleApiUnlock();
    void handleApiConfig();
    void handleNotFound();
    String generateJsonResponse(bool success, const char* message);
    
public:
    NetworkManager();
    void begin(const char* wifiSSID, const char* wifiPassword);
    bool connectToWiFi(int timeoutSeconds = 30);
    void startWebServer();
    void startAccessPoint(const char* apSSID, const char* apPassword);
    void handleClient();
    String getLocalIP();
    bool isConnected();
    int getRSSI();
    String wifiStatusToString();
    void scanNetworks(String* networks, int* count);
};

#endif
