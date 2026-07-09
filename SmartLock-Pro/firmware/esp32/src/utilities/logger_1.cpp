// logger.cpp - Logger Implementation
#include "logger.h"
#include <time.h>
#include <SD.h>
#include <stdarg.h>

// Global instance
Logger logger;

Logger::Logger() {
    logLevel = LOG_LEVEL_INFO;
    initialized = false;
    serialEnabled = true;
    sdCardEnabled = false;
    mqttEnabled = false;
}

void Logger::init(int level) {
    logLevel = level;
    initialized = true;
    info("Logger", "Logger initialized with level %d", level);
}

void Logger::setLevel(int level) {
    logLevel = level;
}

int Logger::getLevel() {
    return logLevel;
}

const char* Logger::getLevelString(int level) {
    switch(level) {
        case LOG_LEVEL_ERROR:   return "ERROR";
        case LOG_LEVEL_WARNING: return "WARN ";
        case LOG_LEVEL_INFO:    return "INFO ";
        case LOG_LEVEL_DEBUG:   return "DEBUG";
        case LOG_LEVEL_TRACE:   return "TRACE";
        default:                return "UNK  ";
    }
}

void Logger::log(int level, const char* source, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log(level, source, format, args);
    va_end(args);
}

void Logger::log(int level, const char* source, const char* format, va_list args) {
    if (level > logLevel || !initialized) return;
    
    // Format message
    vsnprintf(buffer, sizeof(buffer), format, args);
    
    // Get timestamp
    time_t now = time(nullptr);
    struct tm* tm_info = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    // Format: [timestamp] [LEVEL] [SOURCE] message
    char formatted[512];
    snprintf(formatted, sizeof(formatted), "[%s] [%s] [%s] %s",
             timestamp, getLevelString(level), source, buffer);
    
    // Send to outputs
    if (serialEnabled) {
        writeToSerial(level, source, formatted);
    }
    
    if (sdCardEnabled) {
        writeToSDCard(level, source, formatted);
    }
    
    if (mqttEnabled) {
        writeToMQTT(level, source, formatted);
    }
}

void Logger::writeToSerial(int level, const char* source, const char* message) {
    Serial.println(message);
}

void Logger::writeToSDCard(int level, const char* source, const char* message) {
    // Implementation depends on SD card setup
}

void Logger::writeToMQTT(int level, const char* source, const char* message) {
    // Implementation depends on MQTT setup
}

void Logger::writeToUART(int level, const char* source, const char* message) {
    // Implementation depends on UART setup
}

void Logger::error(const char* source, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log(LOG_LEVEL_ERROR, source, format, args);
    va_end(args);
}

void Logger::warning(const char* source, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log(LOG_LEVEL_WARNING, source, format, args);
    va_end(args);
}

void Logger::info(const char* source, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log(LOG_LEVEL_INFO, source, format, args);
    va_end(args);
}

void Logger::debug(const char* source, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log(LOG_LEVEL_DEBUG, source, format, args);
    va_end(args);
}

void Logger::trace(const char* source, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log(LOG_LEVEL_TRACE, source, format, args);
    va_end(args);
}

void Logger::enableSerial(bool enable) {
    serialEnabled = enable;
}

void Logger::enableSDCard(bool enable) {
    sdCardEnabled = enable;
}

void Logger::enableMQTT(bool enable) {
    mqttEnabled = enable;
}

void Logger::enableUART(bool enable) {
    // Implementation depends on UART setup
}

void Logger::setSDCardPath(const char* path) {
    // Implementation depends on SD card setup
}

void Logger::setMQTTTopic(const char* topic) {
    // Implementation depends on MQTT setup
}

void Logger::flush() {
    Serial.flush();
}

bool Logger::isInitialized() {
    return initialized;
}
