// sensor_manager.h - Sensor Manager Header
#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <DHT.h>

class SensorManager {
private:
    DHT* dht;
    float temperature;
    float humidity;
    bool initialized;
    
public:
    SensorManager();
    bool init();
    bool readTemperature(float* temp);
    bool readHumidity(float* hum);
    bool isDoorOpen();
    bool isMotionDetected();
    uint8_t getBatteryLevel();
    float getTemperature();
    float getHumidity();
    void update();
};

#endif // SENSOR_MANAGER_H
