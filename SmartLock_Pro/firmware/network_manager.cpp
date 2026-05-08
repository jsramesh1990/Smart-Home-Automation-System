#include "include/network_manager.h"
#include "include/config.h"
#include <ArduinoJson.h>

NetworkManager::NetworkManager() {
    server = nullptr;
    dnsServer = nullptr;
    apMode = false;
    connected = false;
    reconnectAttempts = 0;
    lastReconnectAttempt = 0;
}

NetworkManager::~NetworkManager() {
    if (server) delete server;
    if (dnsServer) delete dnsServer;
}

void NetworkManager::begin(const char* wifiSSID, const char* wifiPassword) {
    strncpy(ssid, wifiSSID, 63);
    strncpy(password, wifiPassword, 63);
    
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
}

bool NetworkManager::connectToWiFi(int timeoutSeconds) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < timeoutSeconds) {
        delay(1000);
        attempts++;
        DEBUG_PRINT(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        localIP = WiFi.localIP();
        connected = true;
        reconnectAttempts = 0;
        DEBUG_PRINTLN("\nWiFi connected");
        DEBUG_PRINT("IP address: ");
        DEBUG_PRINTLN(localIP);
        DEBUG_PRINT("RSSI: ");
        DEBUG_PRINTLN(WiFi.RSSI());
        return true;
    }
    
    connected = false;
    DEBUG_PRINTLN("\nWiFi connection failed");
    return false;
}

void NetworkManager::startWebServer() {
    if (server) delete server;
    server = new WebServer(LOCAL_HTTP_PORT);
    
    server->on("/", std::bind(&NetworkManager::handleRoot, this));
    server->on("/api/status", HTTP_GET, std::bind(&NetworkManager::handleApiStatus, this));
    server->on("/api/lock", HTTP_POST, std::bind(&NetworkManager::handleApiLock, this));
    server->on("/api/unlock", HTTP_POST, std::bind(&NetworkManager::handleApiUnlock, this));
    server->on("/api/config", HTTP_POST, std::bind(&NetworkManager::handleApiConfig, this));
    server->on("/api/logs", HTTP_GET, std::bind(&NetworkManager::handleApiLogs, this));
    server->on("/api/voice/enroll", HTTP_POST, std::bind(&NetworkManager::handleApiVoice, this));
    server->on("/api/guest-code", HTTP_POST, std::bind(&NetworkManager::handleApiGuestCode, this));
    server->on("/update", HTTP_POST, std::bind(&NetworkManager::handleUpdate, this));
    server->on("/reset", HTTP_POST, std::bind(&NetworkManager::handleReset, this));
    server->onNotFound(std::bind(&NetworkManager::handleNotFound, this));
    
    server->begin();
    DEBUG_PRINTLN("Web server started");
}

void NetworkManager::startAccessPoint(const char* apSSID, const char* apPassword) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID, apPassword);
    localIP = WiFi.softAPIP();
    apMode = true;
    
    if (!dnsServer) {
        dnsServer = new DNSServer();
        dnsServer->start(53, "*", localIP);
    }
    
    startWebServer();
    DEBUG_PRINTLN("Access Point started");
    DEBUG_PRINT("AP IP: ");
    DEBUG_PRINTLN(localIP);
}

void NetworkManager::handleClient() {
    if (server) server->handleClient();
    if (dnsServer && apMode) dnsServer->processNextRequest();
}

String NetworkManager::generateJsonResponse(bool success, const char* message, const char* data) {
    StaticJsonDocument<512> doc;
    doc["success"] = success;
    doc["message"] = message;
    doc["timestamp"] = millis();
    
    if (data) {
        doc["data"] = data;
    }
    
    String response;
    serializeJson(doc, response);
    return response;
}

String NetworkManager::htmlEscape(const String& str) {
    String escaped = str;
    escaped.replace("&", "&amp;");
    escaped.replace("<", "&lt;");
    escaped.replace(">", "&gt;");
    escaped.replace("\"", "&quot;");
    return escaped;
}

void NetworkManager::handleRoot() {
    String html = R"raw(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>SmartLock Pro</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
        }
        .container {
            background: white;
            border-radius: 20px;
            padding: 40px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            max-width: 500px;
            width: 100%;
            text-align: center;
        }
        h1 {
            color: #333;
            margin-bottom: 10px;
        }
        .subtitle {
            color: #666;
            margin-bottom: 30px;
        }
        .status-card {
            background: #f5f5f5;
            border-radius: 15px;
            padding: 20px;
            margin-bottom: 30px;
        }
        .lock-status {
            font-size: 48px;
            margin-bottom: 10px;
        }
        .status-text {
            font-size: 24px;
            font-weight: bold;
        }
        .locked { color: #e74c3c; }
        .unlocked { color: #2ecc71; }
        .buttons {
            display: flex;
            gap: 15px;
            margin-bottom: 30px;
        }
        button {
            flex: 1;
            padding: 15px;
            font-size: 18px;
            font-weight: bold;
            border: none;
            border-radius: 10px;
            cursor: pointer;
            transition: transform 0.2s, opacity 0.2s;
        }
        button:active { transform: scale(0.98); }
        .lock-btn {
            background: #e74c3c;
            color: white;
        }
        .unlock-btn {
            background: #2ecc71;
            color: white;
        }
        .info {
            text-align: left;
            background: #f9f9f9;
            border-radius: 10px;
            padding: 15px;
            margin-top: 20px;
        }
        .info-item {
            display: flex;
            justify-content: space-between;
            padding: 8px 0;
            border-bottom: 1px solid #eee;
        }
        .info-label { font-weight: bold; color: #555; }
        .info-value { color: #333; }
        .battery-low { color: #e74c3c; }
        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.5; }
        }
        .loading { animation: pulse 1s infinite; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔐 SmartLock Pro</h1>
        <div class="subtitle">Secure Access Control System</div>
        
        <div class="status-card">
            <div class="lock-status" id="lockIcon">🔒</div>
            <div class="status-text" id="statusText">Loading...</div>
        </div>
        
        <div class="buttons">
            <button class="lock-btn" onclick="controlLock('lock')">🔒 LOCK</button>
            <button class="unlock-btn" onclick="controlLock('unlock')">🔓 UNLOCK</button>
        </div>
        
        <div class="info">
            <div class="info-item">
                <span class="info-label">🔋 Battery</span>
                <span class="info-value" id="battery">--%</span>
            </div>
            <div class="info-item">
                <span class="info-label">📡 WiFi Signal</span>
                <span class="info-value" id="wifi">-- dBm</span>
            </div>
            <div class="info-item">
                <span class="info-label">🕐 Last Activity</span>
                <span class="info-value" id="lastActivity">--</span>
            </div>
        </div>
    </div>
    
    <script>
        async function controlLock(action) {
            try {
                const response = await fetch(`/api/${action}`, { method: 'POST' });
                const data = await response.json();
                if (data.success) {
                    updateStatus();
                } else {
                    alert('Error: ' + data.message);
                }
            } catch (error) {
                alert('Network error: ' + error.message);
            }
        }
        
        async function updateStatus() {
            try {
                const response = await fetch('/api/status');
                const data = await response.json();
                
                const isLocked = data.state === 'locked';
                document.getElementById('lockIcon').innerHTML = isLocked ? '🔒' : '🔓';
                document.getElementById('statusText').innerHTML = isLocked ? 'LOCKED' : 'UNLOCKED';
                document.getElementById('statusText').className = 'status-text ' + (isLocked ? 'locked' : 'unlocked');
                
                document.getElementById('battery').innerHTML = data.battery + '%';
                if (data.battery < 20) {
                    document.getElementById('battery').className = 'battery-low';
                }
                
                document.getElementById('wifi').innerHTML = data.wifi_rssi + ' dBm';
                document.getElementById('lastActivity').innerHTML = new Date(data.last_activity).toLocaleTimeString();
            } catch (error) {
                console.error('Status update failed:', error);
            }
        }
        
        setInterval(updateStatus, 3000);
        updateStatus();
    </script>
</body>
</html>
)raw";
    
    server->send(200, "text/html", html);
}

void NetworkManager::handleApiStatus() {
    StaticJsonDocument<256> doc;
    doc["success"] = true;
    doc["state"] = "locked";  // Get from hardware controller
    doc["battery"] = 85;
    doc["wifi_rssi"] = WiFi.RSSI();
    doc["wifi_ssid"] = WiFi.SSID();
    doc["ip"] = WiFi.localIP().toString();
    doc["mac"] = WiFi.macAddress();
    doc["uptime"] = millis() / 1000;
    doc["last_activity"] = millis();
    
    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void NetworkManager::handleApiLock() {
    // Call hardware lock
    server->send(200, "application/json", generateJsonResponse(true, "Door locked"));
}

void NetworkManager::handleApiUnlock() {
    // Call hardware unlock
    server->send(200, "application/json", generateJsonResponse(true, "Door unlocked"));
}

void NetworkManager::handleApiConfig() {
    if (server->hasArg("plain")) {
        String body = server->arg("plain");
        DEBUG_PRINTLN("Config update: " + body);
        server->send(200, "application/json", generateJsonResponse(true, "Configuration updated"));
    } else {
        server->send(400, "application/json", generateJsonResponse(false, "No configuration data"));
    }
}

void NetworkManager::handleApiLogs() {
    server->send(200, "application/json", generateJsonResponse(true, "Logs retrieved"));
}

void NetworkManager::handleApiVoice() {
    server->send(200, "application/json", generateJsonResponse(true, "Voice enrolled"));
}

void NetworkManager::handleApiGuestCode() {
    server->send(200, "application/json", generateJsonResponse(true, "Guest code generated"));
}

void NetworkManager::handleNotFound() {
    server->send(404, "application/json", generateJsonResponse(false, "Endpoint not found"));
}

void NetworkManager::handleUpdate() {
    if (server->hasArg("plain")) {
        // Handle OTA update
        server->send(200, "application/json", generateJsonResponse(true, "Update started"));
    } else {
        server->send(400, "application/json", generateJsonResponse(false, "No firmware data"));
    }
}

void NetworkManager::handleReset() {
    server->send(200, "application/json", generateJsonResponse(true, "Resetting device..."));
    delay(100);
    ESP.restart();
}

bool NetworkManager::reconnect() {
    unsigned long now = millis();
    if (now - lastReconnectAttempt < 5000) {
        return false;
    }
    lastReconnectAttempt = now;
    
    if (!connected && !apMode) {
        DEBUG_PRINTLN("Attempting WiFi reconnection...");
        return connectToWiFi(20);
    }
    return connected;
}

String NetworkManager::getLocalIP() {
    return localIP.toString();
}

String NetworkManager::getMACAddress() {
    return WiFi.macAddress();
}

bool NetworkManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

int NetworkManager::getRSSI() {
    return WiFi.RSSI();
}

String NetworkManager::getSSID() {
    return WiFi.SSID();
}

String NetworkManager::wifiStatusToString() {
    switch(WiFi.status()) {
        case WL_CONNECTED: return "Connected";
        case WL_CONNECT_FAILED: return "Connection failed";
        case WL_DISCONNECTED: return "Disconnected";
        case WL_NO_SSID_AVAIL: return "SSID not found";
        default: return "Unknown";
    }
}

void NetworkManager::scanNetworks(String* networks, int* count) {
    int n = WiFi.scanNetworks();
    *count = n < *count ? n : *count;
    
    for (int i = 0; i < *count; i++) {
        networks[i] = WiFi.SSID(i);
    }
    WiFi.scanDelete();
}

bool NetworkManager::enableOTA(const char* password) {
    // OTA setup
    return true;
}

void NetworkManager::handleOTA() {
    // Handle OTA updates
}

void NetworkManager::setAPMode(bool enable) {
    apMode = enable;
}
