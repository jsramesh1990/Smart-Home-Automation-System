// wifi_manager.cpp - WiFi Manager Implementation
#include "wifi_manager.h"
#include "constants.h"
#include "logger.h"
#include <WiFiManager.h>
#include <ESPmDNS.h>

extern Logger logger;

WiFiManagerClass::WiFiManagerClass() {
    isConnected = false;
    retryCount = 0;
}

bool WiFiManagerClass::init() {
    logger.log(LOG_LEVEL_INFO, "WiFi", "Initializing...");
    
    // Set WiFi to station mode
    WiFi.mode(WIFI_STA);
    
    // Try to connect to saved WiFi
    if (connect()) {
        return true;
    }
    
    // If no saved WiFi, start AP mode for configuration
    startConfigPortal();
    return true;
}

bool WiFiManagerClass::connect() {
    logger.log(LOG_LEVEL_INFO, "WiFi", "Connecting to %s...", WIFI_SSID);
    
    // Set hostname
    WiFi.setHostname(WIFI_HOSTNAME);
    
    // Connect
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < WIFI_CONNECTION_TIMEOUT) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        isConnected = true;
        retryCount = 0;
        
        logger.log(LOG_LEVEL_INFO, "WiFi", "Connected!");
        logger.log(LOG_LEVEL_INFO, "WiFi", "IP: %s", WiFi.localIP().toString().c_str());
        logger.log(LOG_LEVEL_INFO, "WiFi", "RSSI: %d dBm", WiFi.RSSI());
        
        // Setup mDNS
        if (MDNS.begin(WIFI_HOSTNAME)) {
            logger.log(LOG_LEVEL_INFO, "mDNS", "Started: %s.local", WIFI_HOSTNAME);
            MDNS.addService("http", "tcp", 80);
        }
        
        return true;
    }
    
    logger.log(LOG_LEVEL_ERROR, "WiFi", "Connection failed");
    isConnected = false;
    return false;
}

void WiFiManagerClass::startConfigPortal() {
    logger.log(LOG_LEVEL_INFO, "WiFi", "Starting configuration portal...");
    
    WiFiManager wifiManager;
    
    // Set custom parameters
    WiFiManagerParameter custom_mqtt_broker("mqtt", "MQTT Broker", MQTT_BROKER, 40);
    WiFiManagerParameter custom_mqtt_port("mqttport", "MQTT Port", "1883", 6);
    wifiManager.addParameter(&custom_mqtt_broker);
    wifiManager.addParameter(&custom_mqtt_port);
    
    // Custom AP name
    wifiManager.setAPCallback([this](WiFiManager* wm) {
        logger.log(LOG_LEVEL_INFO, "WiFi", "AP Mode started: SmartLock-AP");
    });
    
    // Connect or start portal
    if (!wifiManager.autoConnect("SmartLock-AP", "smartlock123")) {
        logger.log(LOG_LEVEL_ERROR, "WiFi", "Configuration failed!");
        ESP.restart();
    }
    
    // Save credentials to SPIFFS
    saveCredentials(WIFI_SSID, WIFI_PASSWORD);
    
    // Update MQTT settings from parameters
    strcpy(mqttBroker, custom_mqtt_broker.getValue());
    mqttPort = atoi(custom_mqtt_port.getValue());
    
    // Restart to apply new settings
    ESP.restart();
}

bool WiFiManagerClass::update() {
    if (WiFi.status() != WL_CONNECTED) {
        isConnected = false;
        
        if (retryCount < WIFI_MAX_RETRIES) {
            retryCount++;
            logger.log(LOG_LEVEL_WARNING, "WiFi", "Reconnecting (attempt %d/%d)...", 
                       retryCount, WIFI_MAX_RETRIES);
            
            WiFi.reconnect();
            delay(1000);
            
            if (WiFi.status() == WL_CONNECTED) {
                isConnected = true;
                retryCount = 0;
                logger.log(LOG_LEVEL_INFO, "WiFi", "Reconnected!");
                return true;
            }
        } else {
            logger.log(LOG_LEVEL_ERROR, "WiFi", "Max retries exceeded");
            // Try to restart WiFi
            WiFi.disconnect();
            delay(1000);
            WiFi.begin();
            retryCount = 0;
        }
    } else {
        isConnected = true;
        retryCount = 0;
    }
    
    return isConnected;
}

void WiFiManagerClass::saveCredentials(const char* ssid, const char* password) {
    // Save to SPIFFS or EEPROM
    logger.log(LOG_LEVEL_INFO, "WiFi", "Saving credentials...");
    // Implementation for saving credentials
}

String WiFiManagerClass::getIP() {
    return WiFi.localIP().toString();
}

int WiFiManagerClass::getRSSI() {
    return WiFi.RSSI();
}

String WiFiManagerClass::getMAC() {
    return WiFi.macAddress();
}

bool WiFiManagerClass::isConnectedToWiFi() {
    return isConnected;
}
