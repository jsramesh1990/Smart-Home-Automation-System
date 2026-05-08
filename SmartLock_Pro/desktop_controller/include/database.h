#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <string>
#include <vector>
#include <ctime>
#include <mutex>

struct User {
    int id;
    std::string username;
    std::string email;
    std::string password_hash;
    std::string salt;
    std::string role;  // admin, user, guest
    time_t created_at;
    time_t last_login;
    bool active;
    int failed_attempts;
    time_t locked_until;
};

struct AccessLog {
    int id;
    int user_id;
    std::string username;
    std::string action;  // lock, unlock, create_code, etc.
    std::string method;  // app, voice, code, mqtt, api
    std::string details;
    time_t timestamp;
    bool success;
    std::string ip_address;
    std::string user_agent;
};

struct GuestCode {
    int id;
    std::string code;
    int created_by;
    std::string created_by_name;
    time_t created_at;
    time_t expires_at;
    int max_uses;
    int uses_remaining;
    bool active;
    std::string notes;
};

struct VoiceProfile {
    int id;
    int user_id;
    std::vector<float> features;
    float threshold;
    time_t enrolled_at;
    time_t last_used;
    int verification_count;
    int success_count;
};

struct Device {
    int id;
    std::string device_id;
    std::string name;
    std::string ip_address;
    std::string mac_address;
    std::string firmware_version;
    time_t last_seen;
    bool active;
    int battery_level;
};

class Database {
private:
    sqlite3* db;
    std::string db_path;
    std::mutex db_mutex;
    
    bool executeQuery(const std::string& sql);
    bool tableExists(const std::string& table_name);
    void createTables();
    void createIndexes();
    void backup();
    
public:
    Database(const std::string& path = "smartlock.db");
    ~Database();
    
    bool initialize();
    bool close();
    
    // User operations
    bool createUser(const User& user);
    User* getUserById(int id);
    User* getUserByUsername(const std::string& username);
    User* getUserByEmail(const std::string& email);
    bool updateUser(const User& user);
    bool deleteUser(int id);
    bool authenticateUser(const std::string& username, const std::string& password);
    bool updateLoginAttempts(int user_id, bool success);
    std::vector<User> getAllUsers();
    int getUserCount();
    
    // Access log operations
    bool logAccess(const AccessLog& log);
    std::vector<AccessLog> getAccessLogs(time_t from, time_t to, int limit = 100, int offset = 0);
    std::vector<AccessLog> getUserLogs(int user_id, int limit = 50);
    std::vector<AccessLog> searchLogs(const std::string& search, time_t from, time_t to);
    int getLogCount(time_t from, time_t to);
    bool clearOldLogs(int days_to_keep);
    
    // Guest code operations
    bool createGuestCode(const GuestCode& code);
    GuestCode* getGuestCode(const std::string& code);
    bool useGuestCode(const std::string& code);
    bool revokeGuestCode(int id);
    bool updateGuestCode(int id, const GuestCode& code);
    std::vector<GuestCode> getActiveGuestCodes();
    std::vector<GuestCode> getGuestCodesByUser(int user_id);
    bool cleanupExpiredCodes();
    
    // Voice profile operations
    bool saveVoiceProfile(const VoiceProfile& profile);
    VoiceProfile* getVoiceProfile(int user_id);
    bool deleteVoiceProfile(int user_id);
    bool updateVoiceProfile(const VoiceProfile& profile);
    std::vector<VoiceProfile> getAllVoiceProfiles();
    bool incrementVoiceUsage(int user_id, bool success);
    
    // Device operations
    bool registerDevice(const Device& device);
    bool updateDevice(const Device& device);
    Device* getDevice(const std::string& device_id);
    std::vector<Device> getAllDevices();
    bool updateDeviceLastSeen(const std::string& device_id);
    bool deleteDevice(const std::string& device_id);
    
    // Statistics
    struct Statistics {
        int total_users;
        int active_devices;
        int total_accesses_today;
        int failed_accesses_today;
        int active_guest_codes;
        int voice_verifications_today;
        double success_rate;
    };
    Statistics getStatistics();
    
    // Backup/Restore
    bool exportToJson(const std::string& filename);
    bool importFromJson(const std::string& filename);
    
    // Maintenance
    bool vacuum();
    bool integrityCheck();
};

#endif
