#include "include/network_manager.h"
#include "include/config.h"

NetworkManager::NetworkManager() : server(LOCAL_PORT) {
    apMode = false;
}

void NetworkManager::begin(const char* wifiSSID, const char* wifiPassword) {
    strncpy(ssid, wifiSSID, 63);
    strncpy(password, wifiPassword, 63);
}

bool NetworkManager::connectToWiFi(int timeoutSeconds) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < timeoutSeconds) {
        delay(1000);
        attempts++;
        Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        localIP = WiFi.localIP();
        Serial.println("\nWiFi connected");
        Serial.print("IP address: ");
        Serial.println(localIP);
        return true;
    }
    
    Serial.println("\nWiFi connection failed");
    return false;
}

void NetworkManager::startWebServer() {
    server.on("/", std::bind(&NetworkManager::handleRoot, this));
    server.on("/api/status", HTTP_GET, std::bind(&NetworkManager::handleApiStatus, this));
    server.on("/api/lock", HTTP_POST, std::bind(&NetworkManager::handleApiLock, this));
    server.on("/api/unlock", HTTP_POST, std::bind(&NetworkManager::handleApiUnlock, this));
    server.on("/api/config", HTTP_POST, std::bind(&NetworkManager::handleApiConfig, this));
    server.onNotFound(std::bind(&NetworkManager::handleNotFound, this));
    
    server.begin();
    Serial.println("Web server started");
}

void NetworkManager::startAccessPoint(const char* apSSID, const char* apPassword) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID, apPassword);
    localIP = WiFi.softAPIP();
    apMode = true;
    startWebServer();
    Serial.println("Access Point started");
}

void NetworkManager::handleClient() {
    server.handleClient();
}

String NetworkManager::generateJsonResponse(bool success, const char* message) {
    StaticJsonDocument<200> doc;
    doc["success"] = success;
    doc["message"] = message;
    doc["timestamp"] = millis();
    
    String response;
    serializeJson(doc, response);
    return response;
}

void NetworkManager::handleRoot() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<title>SmartLock Pro</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>";
    html += "body{font-family:Arial;text-align:center;margin-top:50px;}";
    html += "button{padding:15px 30px;font-size:18px;margin:10px;}";
    html += ".lock{background-color:#ff4444;color:white;}";
    html += ".unlock{background-color:#44ff44;color:black;}";
    html += ".status{font-size:24px;margin:20px;}";
    html += "</style></head><body>";
    html += "<h1>SmartLock Pro</h1>";
    html += "<div class='status'>Status: <span id='status'>Unknown</span></div>";
    html += "<button class='lock' onclick='lockDoor()'>🔒 LOCK</button>";
    html += "<button class='unlock' onclick='unlockDoor()'>🔓 UNLOCK</button>";
    html += "<script>";
    html += "async function lockDoor(){";
    html += "let r=await fetch('/api/lock',{method:'POST'});";
    html += "let d=await r.json();alert(d.message);updateStatus();}";
    html += "async function unlockDoor(){";
    html += "let r=await fetch('/api/unlock',{method:'POST'});";
    html += "let d=await r.json();alert(d.message);updateStatus();}";
    html += "async function updateStatus(){";
    html += "let r=await fetch('/api/status');";
    html += "let d=await r.json();";
    html += "document.getElementById('status').innerText=d.state;}";
    html += "setInterval(updateStatus,5000);updateStatus();";
    html += "</script></body></html>";
    
    server.send(200, "text/html", html);
}

void NetworkManager::handleApiStatus() {
    StaticJsonDocument<256> doc;
    doc["state"] = "locked";  // Get actual state from hardware
    doc["battery"] = 85;
    doc["wifi_rssi"] = WiFi.RSSI();
    doc["timestamp"] = millis();
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void NetworkManager::handleApiLock() {
    // Lock door logic here
    server.send(200, "application/json", generateJsonResponse(true, "Door locked"));
}

void NetworkManager::handleApiUnlock() {
    // Unlock door logic here
    server.send(200, "application/json", generateJsonResponse(true, "Door unlocked"));
}

void NetworkManager::handleApiConfig() {
    // Handle configuration update
    server.send(200, "application/json", generateJsonResponse(true, "Config updated"));
}

void NetworkManager::handleNotFound() {
    server.send(404, "application/json", generateJsonResponse(false, "Endpoint not found"));
}

String NetworkManager::getLocalIP() {
    return localIP.toString();
}

bool NetworkManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

int NetworkManager::getRSSI() {
    return WiFi.RSSI();
}

String NetworkManager::wifiStatusToString() {
    switch(WiFi.status()) {
        case WL_CONNECTED: return "Connected";
        case WL_CONNECT_FAILED: return "Connection failed";
        case WL_DISCONNECTED: return "Disconnected";
        default: return "Unknown";
    }
}

void NetworkManager::scanNetworks(String* networks, int* count) {
    int n = WiFi.scanNetworks();
    *count = n > *count ? *count : n;
    
    for (int i = 0; i < *count; i++) {
        networks[i] = WiFi.SSID(i);
    }
}
