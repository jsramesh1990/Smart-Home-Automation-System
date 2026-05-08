#ifndef AUTH_MANAGER_H
#define AUTH_MANAGER_H

#include <string>
#include <map>
#include <ctime>
#include <vector>
#include <mutex>

class Database;

struct AuthToken {
    std::string token;
    int user_id;
    std::string username;
    std::string role;
    time_t created_at;
    time_t expires_at;
    std::string ip_address;
    std::string user_agent;
};

struct Permission {
    std::string resource;
    std::string action;  // read, write, delete, admin
};

class AuthManager {
private:
    Database* db;
    std::map<std::string, AuthToken> active_tokens;
    std::mutex token_mutex;
    std::string jwt_secret;
    int token_expiry_hours;
    
    std::map<std::string, std::vector<Permission>> role_permissions;
    
    std::string generateTokenString();
    std::string hashPassword(const std::string& password, const std::string& salt);
    std::string generateSalt();
    bool verifyPassword(const std::string& password, const std::string& hash, const std::string& salt);
    
    void initPermissions();
    void cleanupExpiredTokens();
    
public:
    AuthManager(Database* database);
    ~AuthManager();
    
    // Authentication
    bool registerUser(const std::string& username, const std::string& email, 
                      const std::string& password, const std::string& role = "user");
    AuthToken* authenticate(const std::string& username, const std::string& password,
                            const std::string& ip = "", const std::string& user_agent = "");
    bool validateToken(const std::string& token);
    bool logout(const std::string& token);
    bool refreshToken(const std::string& old_token, AuthToken& new_token);
    
    // Authorization
    bool hasPermission(const std::string& token, const std::string& resource, const std::string& action);
    bool isAdmin(const std::string& token);
    bool isOwner(const std::string& token, int resource_user_id);
    
    // User management
    bool changePassword(int user_id, const std::string& old_password, const std::string& new_password);
    bool resetPassword(const std::string& username, const std::string& new_password);
    bool enableTwoFactor(int user_id, const std::string& secret);
    bool verifyTwoFactor(int user_id, const std::string& code);
    
    // Token management
    AuthToken* getTokenInfo(const std::string& token);
    bool revokeAllTokens(int user_id);
    std::vector<AuthToken> getUserTokens(int user_id);
    
    // Session management
    void cleanup();
    int getActiveSessionCount();
    
    // Security
    bool isAccountLocked(const std::string& username);
    void recordFailedAttempt(const std::string& username, const std::string& ip);
    void clearFailedAttempts(const std::string& username);
    
    // Rate limiting
    bool checkRateLimit(const std::string& key, int max_requests, int time_window_seconds);
    
    // API Key management
    std::string generateApiKey(int user_id);
    bool validateApiKey(const std::string& api_key, int& user_id);
    bool revokeApiKey(const std::string& api_key);
};

#endif
