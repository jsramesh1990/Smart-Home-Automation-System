// logger.h - Logger Header
#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <stdarg.h>

// Log levels
#define LOG_LEVEL_ERROR 0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_DEBUG 2

class Logger {
private:
    int logLevel;
    bool initialized;
    const char* getLevelString(int level);
    
public:
    Logger();
    void init(int level);
    void log(int level, const char* source, const char* format, ...);
    void error(const char* source, const char* format, ...);
    void info(const char* source, const char* format, ...);
    void debug(const char* source, const char* format, ...);
};

#endif // LOGGER_H
