// fingerprint.cpp - Fingerprint Authentication Implementation
#include "fingerprint.h"
#include "pins.h"
#include "constants.h"
#include "logger.h"
#include <Adafruit_Fingerprint.h>

extern Logger logger;

FingerprintAuth::FingerprintAuth() {
    sensor = nullptr;
    initialized = false;
}

bool FingerprintAuth::init() {
    logger.log(LOG_LEVEL_INFO, "Fingerprint", "Initializing...");
    
    // Create fingerprint sensor object on UART2
    sensor = new Adafruit_Fingerprint(&Serial2);
    
    if (!sensor) {
        logger.log(LOG_LEVEL_ERROR, "Fingerprint", "Failed to create sensor object");
        return false;
    }
    
    // Set baud rate
    sensor->begin(57600);
    delay(100);
    
    // Verify sensor is present
    if (sensor->verifyPassword()) {
        initialized = true;
        logger.log(LOG_LEVEL_INFO, "Fingerprint", "Sensor found!");
        logger.log(LOG_LEVEL_INFO, "Fingerprint", "Capacity: %d fingerprints", 
                   sensor->capacity);
        return true;
    }
    
    logger.log(LOG_LEVEL_ERROR, "Fingerprint", "Sensor not found!");
    return false;
}

int FingerprintAuth::authenticate() {
    if (!initialized || !sensor) {
        return -1;
    }
    
    // Check if finger is present
    if (sensor->getImage() != FINGERPRINT_OK) {
        return -1;
    }
    
    // Convert image to feature set
    if (sensor->image2Tz() != FINGERPRINT_OK) {
        return -2;
    }
    
    // Search for match
    int result = sensor->fingerFastSearch();
    if (result != FINGERPRINT_OK) {
        return -3;
    }
    
    // Return match ID
    return sensor->fingerID;
}

bool FingerprintAuth::enroll(int id) {
    if (!initialized || !sensor) {
        return false;
    }
    
    logger.log(LOG_LEVEL_INFO, "Fingerprint", "Enrolling ID %d...", id);
    
    // Check if ID already exists
    if (sensor->loadModel(id) == FINGERPRINT_OK) {
        logger.log(LOG_LEVEL_WARNING, "Fingerprint", "ID %d already exists", id);
        return false;
    }
    
    // Enroll new fingerprint
    int result = sensor->enrollFingerprint(id);
    if (result == FINGERPRINT_OK) {
        logger.log(LOG_LEVEL_INFO, "Fingerprint", "Enrollment successful!");
        return true;
    }
    
    logger.log(LOG_LEVEL_ERROR, "Fingerprint", "Enrollment failed: %d", result);
    return false;
}

bool FingerprintAuth::deleteTemplate(int id) {
    if (!initialized || !sensor) {
        return false;
    }
    
    if (sensor->deleteModel(id) == FINGERPRINT_OK) {
        logger.log(LOG_LEVEL_INFO, "Fingerprint", "Deleted template ID %d", id);
        return true;
    }
    
    return false;
}

bool FingerprintAuth::clearAllTemplates() {
    if (!initialized || !sensor) {
        return false;
    }
    
    if (sensor->emptyDatabase() == FINGERPRINT_OK) {
        logger.log(LOG_LEVEL_INFO, "Fingerprint", "Cleared all templates");
        return true;
    }
    
    return false;
}

int FingerprintAuth::getTemplateCount() {
    if (!initialized || !sensor) {
        return -1;
    }
    
    return sensor->getTemplateCount();
}

bool FingerprintAuth::isFingerDetected() {
    if (!initialized || !sensor) {
        return false;
    }
    
    return sensor->getImage() == FINGERPRINT_OK;
}
