// ble_manager.h - BLE Manager Header
#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

class BLEManager : public BLEServerCallbacks, public BLECharacteristicCallbacks {
private:
    BLEServer* server;
    BLECharacteristic* characteristic;
    bool connected;
    bool authenticated;
    uint16_t authenticatedUser;
    
public:
    BLEManager();
    bool init();
    void update();
    bool isConnected();
    bool isAuthenticated();
    uint16_t getAuthenticatedUser();
    
    // BLE Callbacks
    void onConnect(BLEServer* pServer) override;
    void onDisconnect(BLEServer* pServer) override;
    void onWrite(BLECharacteristic* pCharacteristic) override;
};

#endif // BLE_MANAGER_H
