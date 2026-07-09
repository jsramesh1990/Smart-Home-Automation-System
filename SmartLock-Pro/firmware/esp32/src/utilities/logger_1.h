// logger.h - Logger Header
#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <stdarg.h>

// Log levels
#define LOG_LEVEL_ERROR 0
#define LOG_LEVEL_WARNING 1
#define LOG_LEVEL_INFO 2
#define LOG_LEVEL_DEBUG 3
#define LOG_LEVEL_TRACE 4

class Logger {
private:
    int logLevel;
    bool initialized;
    bool serialEnabled;
    bool sdCardEnabled;
    bool mqttEnabled;
    char buffer[512];
    
    const char* getLevelString(int level);
    void writeToSerial(int level, const char* source, const char* message);
    void writeToSDCard(int level, const char* source, const char* message);
    void writeToMQTT(int level, const char* source, const char* message);
    void writeToUART(int level, const char* source, const char* message);
    
public:
    Logger();
    void init(int level = LOG_LEVEL_INFO);
    void setLevel(int level);
    int getLevel();
    
    void log(int level, const char* source, const char* format, ...);
    void log(int level, const char* source, const char* format, va_list args);
    
    void error(const char* source, const char* format, ...);
    void warning(const char* source, const char* format, ...);
    void info(const char* source, const char* format, ...);
    void debug(const char* source, const char* format, ...);
    void trace(const char* source, const char* format, ...);
    
    void enableSerial(bool enable);
    void enableSDCard(bool enable);
    void enableMQTT(bool enable);
    void enableUART(bool enable);
    
    void setSDCardPath(const char* path);
    void setMQTTTopic(const char* topic);
    
    void flush();
    bool isInitialized();
};

#endif // LOGGER_H
