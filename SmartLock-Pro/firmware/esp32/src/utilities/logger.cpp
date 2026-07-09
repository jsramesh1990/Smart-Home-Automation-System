// logger.cpp - Logger Implementation
#include "logger.h"
#include "config.h"

Logger::Logger() {
    logLevel = LOG_LEVEL_INFO;
    initialized = false;
}

void Logger::init(int level) {
    logLevel = level;
    initialized = true;
    log(LOG_LEVEL_INFO, "Logger", "Logger initialized with level %d", level);
}

void Logger::log(int level, const char* source, const char* format, ...) {
    if (level > logLevel || !initialized) return;
    
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    // Get timestamp
    unsigned long timestamp = millis();
    unsigned long seconds = timestamp / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    
    // Format: [HH:MM:SS.mmm] [LEVEL] [SOURCE] Message
    Serial.printf("[%02lu:%02lu:%02lu.%03lu] [%s] [%s] %s\n",
        hours % 24,
        minutes % 60,
        seconds % 60,
        timestamp % 1000,
        getLevelString(level),
        source,
        buffer
    );
}

const char* Logger::getLevelString(int level) {
    switch(level) {
        case LOG_LEVEL_ERROR:   return "ERROR";
        case LOG_LEVEL_INFO:    return "INFO ";
        case LOG_LEVEL_DEBUG:   return "DEBUG";
        default:                return "UNKNOWN";
    }
}

void Logger::error(const char* source, const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    log(LOG_LEVEL_ERROR, source, buffer);
}

void Logger::info(const char* source, const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    log(LOG_LEVEL_INFO, source, buffer);
}

void Logger::debug(const char* source, const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    log(LOG_LEVEL_DEBUG, source, buffer);
}
