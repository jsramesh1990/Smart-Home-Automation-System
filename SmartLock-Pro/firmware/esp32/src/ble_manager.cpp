// ble_manager.cpp - BLE Manager Implementation
#include "ble_manager.h"
#include "config.h"
#include "constants.h"
#include "logger.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

extern Logger logger;

// UUIDs for BLE Service
#define SERVICE_UUID           "0000180F-0000-1000-8000-00805F9B34FB"
#define CHARACTERISTIC_UUID    "00002A19-0000-1000-8000-00805F9B34FB"

BLEManager::BLEManager() {
    server = nullptr;
    characteristic = nullptr;
    connected = false;
    authenticated = false;
    authenticatedUser = 0;
}

bool BLEManager::init() {
    logger.log(LOG_LEVEL_INFO, "BLE", "Initializing...");
    
    BLEDevice::init(BLE_DEVICE_NAME);
    server = BLEDevice::createServer();
    server->setCallbacks(this);
    
    BLEService* service = server->createService(SERVICE_UUID);
    characteristic = service->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    
    characteristic->addDescriptor(new BLE2902());
    characteristic->setCallbacks(this);
    
    service->start();
    
    BLEAdvertising* advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(SERVICE_UUID);
    advertising->setScanResponse(true);
    advertising->setMinPreferred(0x06);
    advertising->setMaxPreferred(0x12);
    BLEDevice::startAdvertising();
    
    logger.log(LOG_LEVEL_INFO, "BLE", "Advertising started");
    return true;
}

void BLEManager::update() {
    // Nothing to do in update
}

bool BLEManager::isConnected() {
    return connected;
}

bool BLEManager::isAuthenticated() {
    return authenticated;
}

uint16_t BLEManager::getAuthenticatedUser() {
    return authenticatedUser;
}

void BLEManager::onConnect(BLEServer* pServer) {
    connected = true;
    logger.log(LOG_LEVEL_INFO, "BLE", "Device connected");
}

void BLEManager::onDisconnect(BLEServer* pServer) {
    connected = false;
    authenticated = false;
    authenticatedUser = 0;
    logger.log(LOG_LEVEL_INFO, "BLE", "Device disconnected");
    BLEDevice::startAdvertising();
}

void BLEManager::onWrite(BLECharacteristic* pCharacteristic) {
    String value = pCharacteristic->getValue().c_str();
    logger.log(LOG_LEVEL_DEBUG, "BLE", "Write: %s", value.c_str());
    
    // Parse command
    if (value.startsWith("AUTH:")) {
        // Authentication command
        String token = value.substring(5);
        // Validate token
        if (token == "SECRET_TOKEN") {
            authenticated = true;
            authenticatedUser = 1;
            pCharacteristic->setValue("OK");
            pCharacteristic->notify();
            logger.log(LOG_LEVEL_INFO, "BLE", "Authentication successful");
        } else {
            pCharacteristic->setValue("ERR");
            pCharacteristic->notify();
            logger.log(LOG_LEVEL_WARNING, "BLE", "Authentication failed");
        }
    } else if (value == "LOCK") {
        // Lock command
        // lockController.lock();
        pCharacteristic->setValue("LOCKED");
        pCharacteristic->notify();
    } else if (value == "UNLOCK") {
        // Unlock command
        if (authenticated) {
            // lockController.unlock();
            pCharacteristic->setValue("UNLOCKED");
            pCharacteristic->notify();
        } else {
            pCharacteristic->setValue("AUTH_REQUIRED");
            pCharacteristic->notify();
        }
    } else if (value == "STATUS") {
        // Status command
        pCharacteristic->setValue("LOCKED|CLOSED|100|85");
        pCharacteristic->notify();
    }
}
