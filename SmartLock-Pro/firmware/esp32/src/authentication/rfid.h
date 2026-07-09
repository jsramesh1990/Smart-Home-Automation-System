// rfid.h - RFID Authentication Header
#ifndef RFID_H
#define RFID_H

#include <Arduino.h>
#include <MFRC522.h>
#include <map>

class RFIDAuth {
private:
    MFRC522* mfrc522;
    bool initialized;
    std::map<uint32_t, uint16_t> cardDatabase;
    
public:
    RFIDAuth();
    bool init();
    uint32_t authenticate();
    bool isCardDetected();
    bool addCard(uint32_t uid, uint16_t userId);
    bool removeCard(uint32_t uid);
    uint16_t getUserByUID(uint32_t uid);
};

#endif // RFID_H
