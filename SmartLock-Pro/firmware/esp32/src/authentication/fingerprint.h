// fingerprint.h - Fingerprint Authentication Header
#ifndef FINGERPRINT_H
#define FINGERPRINT_H

#include <Arduino.h>
#include <Adafruit_Fingerprint.h>

class FingerprintAuth {
private:
    Adafruit_Fingerprint* sensor;
    bool initialized;
    
public:
    FingerprintAuth();
    bool init();
    int authenticate();
    bool enroll(int id);
    bool deleteTemplate(int id);
    bool clearAllTemplates();
    int getTemplateCount();
    bool isFingerDetected();
};

#endif // FINGERPRINT_H
