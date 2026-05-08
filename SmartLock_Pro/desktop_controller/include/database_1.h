#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <string>
#include <vector>
#include <ctime>

struct User {
    int id;
    std::string username;
    std::string email;
    std::string passwordHash;
    std::string salt;
    std::string role;
    time_t createdAt;
    bool active;
};

struct AccessLog {
    int id;
    int userId;
    std::string action;
    std::string method;  // voice, app, code, etc.
    std::string details;
    time_t timestamp;
    bool success;
};

struct GuestCode {
    int id;
    std::string code;
    std::string userId;
    time_t expiresAt;
    int maxUses;
    int usesRemaining;
    bool active;
};

struct VoiceProfile {
    int id;
    int userId;
    std::vector<float> features;
    float threshold;
    time_t enrolledAt;
};

class Database {
private:
    sqlite3* db;
    std::string dbPath;
    
    bool executeQuery(const std::string& sql);
    bool tableExists(const std::string& tableName);
    void createTables();
    
public:
    Database(const std::string& path);
    ~Database();
    
    bool initialize();
    
    // User operations
    bool createUser(const User& user);
    User* getUserById(int id);
    User* getUserByUsername(const std::string& username);
    bool updateUser(const User& user);
    bool deleteUser(int id);
    std::vector<User> getAllUsers();
    
    // Access log operations
    bool logAccess(const AccessLog& log);
    std::vector<AccessLog> getAccessLogs(time_t from, time_t to, int limit = 100);
    std::vector<AccessLog> getUserLogs(int userId, int limit = 50);
    bool clearOldLogs(int daysToKeep);
    
    // Guest code operations
    bool createGuestCode(const GuestCode& code);
    GuestCode* getGuestCode(const std::string& code);
    bool useGuestCode(const std::string& code);
    bool revokeGuestCode(int id);
    std::vector<GuestCode> getActiveGuestCodes();
    bool cleanupExpiredCodes();
    
    // Voice profile operations
    bool saveVoiceProfile(const VoiceProfile& profile);
    VoiceProfile* getVoiceProfile(int userId);
    bool deleteVoiceProfile(int userId);
    std::vector<VoiceProfile> getAllVoiceProfiles();
    
    // Statistics
    int getTotalAccessCount();
    int getFailedAccessCount();
    double getSuccessRate();
};

#endif
