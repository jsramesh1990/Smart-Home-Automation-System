// rfid.cpp - RFID Authentication Implementation
#include "rfid.h"
#include "pins.h"
#include "constants.h"
#include "logger.h"
#include <MFRC522.h>

extern Logger logger;

RFIDAuth::RFIDAuth() {
    mfrc522 = nullptr;
    initialized = false;
}

bool RFIDAuth::init() {
    logger.log(LOG_LEVEL_INFO, "RFID", "Initializing...");
    
    // Create RFID object
    mfrc522 = new MFRC522(PIN_RFID_CS, PIN_RFID_RST);
    
    if (!mfrc522) {
        logger.log(LOG_LEVEL_ERROR, "RFID", "Failed to create RFID object");
        return false;
    }
    
    // Initialize
    SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_RFID_CS);
    mfrc522->PCD_Init();
    delay(100);
    
    // Verify module
    if (mfrc522->PCD_PerformSelfTest()) {
        initialized = true;
        logger.log(LOG_LEVEL_INFO, "RFID", "Module initialized");
        logger.log(LOG_LEVEL_INFO, "RFID", "Firmware version: 0x%X", 
                   mfrc522->PCD_ReadRegister(MFRC522::VersionReg));
        return true;
    }
    
    logger.log(LOG_LEVEL_ERROR, "RFID", "Self-test failed!");
    return false;
}

uint32_t RFIDAuth::authenticate() {
    if (!initialized || !mfrc522) {
        return 0;
    }
    
    // Check for new card
    if (!mfrc522->PICC_IsNewCardPresent()) {
        return 0;
    }
    
    // Select card
    if (!mfrc522->PICC_ReadCardSerial()) {
        return 0;
    }
    
    // Get UID
    uint32_t uid = 0;
    for (byte i = 0; i < mfrc522->uid.size; i++) {
        uid <<= 8;
        uid |= mfrc522->uid.uidByte[i];
    }
    
    // Halt PICC
    mfrc522->PICC_HaltA();
    
    // Return UID
    return uid;
}

bool RFIDAuth::isCardDetected() {
    if (!initialized || !mfrc522) {
        return false;
    }
    
    return mfrc522->PICC_IsNewCardPresent();
}

bool RFIDAuth::addCard(uint32_t uid, uint16_t userId) {
    // Store in database
    cardDatabase[uid] = userId;
    return true;
}

bool RFIDAuth::removeCard(uint32_t uid) {
    if (cardDatabase.find(uid) != cardDatabase.end()) {
        cardDatabase.erase(uid);
        return true;
    }
    return false;
}

uint16_t RFIDAuth::getUserByUID(uint32_t uid) {
    if (cardDatabase.find(uid) != cardDatabase.end()) {
        return cardDatabase[uid];
    }
    return 0;
}
