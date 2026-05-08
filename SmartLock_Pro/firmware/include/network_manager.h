#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Update.h>

class NetworkManager {
private:
    WebServer* server;
    DNSServer* dnsServer;
    
    char ssid[64];
    char password[64];
    IPAddress localIP;
    bool apMode;
    bool connected;
    int reconnectAttempts;
    unsigned long lastReconnectAttempt;
    
    // Handler functions
    void handleRoot();
    void handleApiStatus();
    void handleApiLock();
    void handleApiUnlock();
    void handleApiConfig();
    void handleApiLogs();
    void handleApiVoice();
    void handleApiGuestCode();
    void handleNotFound();
    void handleUpdate();
    void handleReset();
    
    String generateJsonResponse(bool success, const char* message, const char* data = nullptr);
    String htmlEscape(const String& str);
    
public:
    NetworkManager();
    ~NetworkManager();
    
    void begin(const char* wifiSSID, const char* wifiPassword);
    bool connectToWiFi(int timeoutSeconds = 30);
    void startWebServer();
    void startAccessPoint(const char* apSSID, const char* apPassword);
    void handleClient();
    bool reconnect();
    
    String getLocalIP();
    String getMACAddress();
    bool isConnected();
    int getRSSI();
    String getSSID();
    String wifiStatusToString();
    void scanNetworks(String* networks, int* count);
    bool enableOTA(const char* password);
    void handleOTA();
    
    void setAPMode(bool enable);
    bool isAPMode() { return apMode; }
};

#endif
