// sensor_manager.cpp - Sensor Manager Implementation
#include "sensor_manager.h"
#include "pins.h"
#include "constants.h"
#include "logger.h"
#include <DHT.h>

extern Logger logger;

SensorManager::SensorManager() {
    dht = nullptr;
    temperature = 0.0f;
    humidity = 0.0f;
    initialized = false;
}

bool SensorManager::init() {
    logger.log(LOG_LEVEL_INFO, "Sensors", "Initializing...");
    
    // Initialize DHT sensor
    dht = new DHT(PIN_TEMP_SENSOR, DHT22);
    if (dht) {
        dht->begin();
        delay(100);
        initialized = true;
        logger.log(LOG_LEVEL_INFO, "Sensors", "DHT22 initialized");
        return true;
    }
    
    logger.log(LOG_LEVEL_ERROR, "Sensors", "Failed to initialize DHT22");
    return false;
}

bool SensorManager::readTemperature(float* temp) {
    if (!initialized || !dht) {
        return false;
    }
    
    float t = dht->readTemperature();
    if (isnan(t)) {
        logger.log(LOG_LEVEL_WARNING, "Sensors", "Failed to read temperature");
        return false;
    }
    
    temperature = t;
    if (temp) *temp = t;
    return true;
}

bool SensorManager::readHumidity(float* hum) {
    if (!initialized || !dht) {
        return false;
    }
    
    float h = dht->readHumidity();
    if (isnan(h)) {
        logger.log(LOG_LEVEL_WARNING, "Sensors", "Failed to read humidity");
        return false;
    }
    
    humidity = h;
    if (hum) *hum = h;
    return true;
}

bool SensorManager::isDoorOpen() {
    return digitalRead(PIN_DOOR_SENSOR) == LOW;
}

bool SensorManager::isMotionDetected() {
    return digitalRead(PIN_PIR_SENSOR) == HIGH;
}

uint8_t SensorManager::getBatteryLevel() {
    // Read battery voltage on ADC pin
    int adcValue = analogRead(PIN_TEMP_SENSOR); // Using temp pin for battery
    
    // Convert ADC to voltage (assuming 3.3V reference)
    float voltage = (adcValue / 4096.0f) * 3.3f;
    
    // Convert to battery percentage (3.0V = 0%, 4.2V = 100%)
    float percentage = ((voltage - 3.0f) / 1.2f) * 100.0f;
    if (percentage > 100) percentage = 100;
    if (percentage < 0) percentage = 0;
    
    return (uint8_t)percentage;
}

float SensorManager::getTemperature() {
    float t;
    if (readTemperature(&t)) {
        return t;
    }
    return -999.0f;
}

float SensorManager::getHumidity() {
    float h;
    if (readHumidity(&h)) {
        return h;
    }
    return -999.0f;
}

void SensorManager::update() {
    readTemperature(nullptr);
    readHumidity(nullptr);
}
