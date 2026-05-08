#ifndef ACCESS_LOGGER_H
#define ACCESS_LOGGER_H

#include <string>
#include <vector>
#include <ctime>
#include <fstream>
#include <mutex>

class Database;

struct LogEntry {
    int id;
    int user_id;
    std::string username;
    std::string action;
    std::string method;
    std::string details;
    time_t timestamp;
    bool success;
    std::string ip_address;
    std::string user_agent;
    std::string device_id;
};

struct LogFilter {
    time_t from_time;
    time_t to_time;
    std::string user;
    std::string action;
    std::string method;
    bool success_only;
    int limit;
    int offset;
};

class AccessLogger {
private:
    Database* db;
    std::ofstream log_file;
    std::mutex log_mutex;
    std::string log_dir;
    
    bool file_logging_enabled;
    bool db_logging_enabled;
    
    void writeToFile(const LogEntry& entry);
    void rotateLogFile();
    std::string formatTimestamp(time_t timestamp);
    std::string escapeCSV(const std::string& str);
    
public:
    AccessLogger(Database* database);
    ~AccessLogger();
    
    bool initialize(const std::string& log_directory = "/var/log/smartlock/");
    void enableFileLogging(bool enable);
    void enableDatabaseLogging(bool enable);
    
    // Logging methods
    bool log(const LogEntry& entry);
    bool logAccess(int user_id, const std::string& username, const std::string& action,
                   const std::string& method, const std::string& details, bool success,
                   const std::string& ip = "", const std::string& user_agent = "",
                   const std::string& device_id = "");
    
    // Convenience methods
    bool logLock(int user_id, const std::string& username, const std::string& method, bool success);
    bool logUnlock(int user_id, const std::string& username, const std::string& method, bool success);
    bool logGuestCodeUse(const std::string& code, const std::string& method, bool success);
    bool logVoiceCommand(int user_id, const std::string& username, const std::string& command, bool success);
    bool logFailedAttempt(const std::string& username, const std::string& method, const std::string& ip);
    
    // Query methods
    std::vector<LogEntry> getLogs(const LogFilter& filter);
    std::vector<LogEntry> getUserLogs(int user_id, int limit = 100);
    std::vector<LogEntry> getRecentLogs(int count = 100);
    std::vector<LogEntry> getLogsByAction(const std::string& action, int limit = 100);
    std::vector<LogEntry> getLogsByMethod(const std::string& method, int limit = 100);
    
    // Statistics
    int getLogCount(const LogFilter& filter);
    int getSuccessfulAttempts(time_t from, time_t to);
    int getFailedAttempts(time_t from, time_t to);
    double getSuccessRate(time_t from, time_t to);
    std::map<std::string, int> getActionBreakdown(time_t from, time_t to);
    std::map<std::string, int> getMethodBreakdown(time_t from, time_t to);
    
    // Export methods
    bool exportToCSV(const LogFilter& filter, const std::string& filename);
    bool exportToJSON(const LogFilter& filter, const std::string& filename);
    std::string generateReport(time_t from, time_t to);
    
    // Maintenance
    bool cleanupOldLogs(int days_to_keep);
    bool archiveLogs(time_t older_than);
    size_t getLogFileSize();
};

#endif
