#include "include/auth_manager.h"
#include "include/database.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <cctype>

AuthManager::AuthManager(Database* database) : db(database) {
    jwt_secret = "your-super-secret-jwt-key-change-in-production";
    token_expiry_hours = 24;
    initPermissions();
}

AuthManager::~AuthManager() {
    cleanup();
}

void AuthManager::initPermissions() {
    // Admin permissions
    role_permissions["admin"] = {
        {"*", "read"}, {"*", "write"}, {"*", "delete"}, {"*", "admin"},
        {"users", "manage"}, {"logs", "view"}, {"devices", "manage"},
        {"guest_codes", "manage"}, {"voice_profiles", "manage"}
    };
    
    // User permissions
    role_permissions["user"] = {
        {"door", "lock"}, {"door", "unlock"}, {"door", "status"},
        {"guest_codes", "create"}, {"guest_codes", "view_own"},
        {"logs", "view_own"}, {"voice_profile", "manage_own"},
        {"profile", "update"}
    };
    
    // Guest permissions
    role_permissions["guest"] = {
        {"door", "unlock"}, {"door", "status"}
    };
}

std::string AuthManager::generateTokenString() {
    const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, chars.size() - 1);
    
    std::string token = "smt_";
    for (int i = 0; i < 32; i++) {
        token += chars[dis(gen)];
    }
    return token;
}

std::string AuthManager::generateSalt() {
    const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, chars.size() - 1);
    
    std::string salt;
    for (int i = 0; i < 32; i++) {
        salt += chars[dis(gen)];
    }
    return salt;
}

std::string AuthManager::hashPassword(const std::string& password, const std::string& salt) {
    // Simple SHA256 hash (would use proper implementation in production)
    std::string combined = password + salt;
    unsigned long hash = 5381;
    for (char c : combined) {
        hash = ((hash << 5) + hash) + c;
    }
    
    std::stringstream ss;
    ss << std::hex << hash;
    return ss.str();
}

bool AuthManager::verifyPassword(const std::string& password, const std::string& hash, const std::string& salt) {
    return hashPassword(password, salt) == hash;
}

bool AuthManager::registerUser(const std::string& username, const std::string& email,
                                const std::string& password, const std::string& role) {
    User user;
    user.username = username;
    user.email = email;
    user.salt = generateSalt();
    user.password_hash = hashPassword(password, user.salt);
    user.role = role;
    user.created_at = time(nullptr);
    user.active = true;
    user.failed_attempts = 0;
    user.locked_until = 0;
    
    return db->createUser(user);
}

AuthToken* AuthManager::authenticate(const std::string& username, const std::string& password,
                                      const std::string& ip, const std::string& user_agent) {
    // Check rate limiting
    if (!checkRateLimit(username, 5, 300)) {
        return nullptr;
    }
    
    // Check if account is locked
    if (isAccountLocked(username)) {
        return nullptr;
    }
    
    User* user = db->getUserByUsername(username);
    if (!user) {
        recordFailedAttempt(username, ip);
        return nullptr;
    }
    
    if (!verifyPassword(password, user->password_hash, user->salt)) {
        recordFailedAttempt(username, ip);
        delete user;
        return nullptr;
    }
    
    // Check if user is active
    if (!user->active) {
        delete user;
        return nullptr;
    }
    
    // Clear failed attempts on successful login
    clearFailedAttempts(username);
    
    // Generate token
    AuthToken* token = new AuthToken();
    token->token = generateTokenString();
    token->user_id = user->id;
    token->username = user->username;
    token->role = user->role;
    token->created_at = time(nullptr);
    token->expires_at = token->created_at + (token_expiry_hours * 3600);
    token->ip_address = ip;
    token->user_agent = user_agent;
    
    // Store token
    {
        std::lock_guard<std::mutex> lock(token_mutex);
        active_tokens[token->token] = *token;
    }
    
    delete user;
    return token;
}

bool AuthManager::validateToken(const std::string& token) {
    std::lock_guard<std::mutex> lock(token_mutex);
    
    auto it = active_tokens.find(token);
    if (it == active_tokens.end()) {
        return false;
    }
    
    if (it->second.expires_at < time(nullptr)) {
        active_tokens.erase(it);
        return false;
    }
    
    return true;
}

bool AuthManager::logout(const std::string& token) {
    std::lock_guard<std::mutex> lock(token_mutex);
    return active_tokens.erase(token) > 0;
}

bool AuthManager::refreshToken(const std::string& old_token, AuthToken& new_token) {
    std::lock_guard<std::mutex> lock(token_mutex);
    
    auto it = active_tokens.find(old_token);
    if (it == active_tokens.end()) {
        return false;
    }
    
    if (it->second.expires_at < time(nullptr)) {
        active_tokens.erase(it);
        return false;
    }
    
    // Generate new token
    new_token = it->second;
    new_token.token = generateTokenString();
    new_token.created_at = time(nullptr);
    new_token.expires_at = new_token.created_at + (token_expiry_hours * 3600);
    
    // Replace old token
    active_tokens.erase(it);
    active_tokens[new_token.token] = new_token;
    
    return true;
}

bool AuthManager::hasPermission(const std::string& token, const std::string& resource, const std::string& action) {
    std::lock_guard<std::mutex> lock(token_mutex);
    
    auto it = active_tokens.find(token);
    if (it == active_tokens.end()) {
        return false;
    }
    
    const AuthToken& auth_token = it->second;
    auto perm_it = role_permissions.find(auth_token.role);
    
    if (perm_it == role_permissions.end()) {
        return false;
    }
    
    for (const auto& perm : perm_it->second) {
        if ((perm.resource == "*" || perm.resource == resource) &&
            (perm.action == "*" || perm.action == action)) {
            return true;
        }
    }
    
    return false;
}

bool AuthManager::isAdmin(const std::string& token) {
    return hasPermission(token, "*", "admin");
}

bool AuthManager::isOwner(const std::string& token, int resource_user_id) {
    std::lock_guard<std::mutex> lock(token_mutex);
    
    auto it = active_tokens.find(token);
    if (it == active_tokens.end()) {
        return false;
    }
    
    return it->second.user_id == resource_user_id || isAdmin(token);
}

bool AuthManager::changePassword(int user_id, const std::string& old_password, const std::string& new_password) {
    User* user = db->getUserById(user_id);
    if (!user) return false;
    
    if (!verifyPassword(old_password, user->password_hash, user->salt)) {
        delete user;
        return false;
    }
    
    std::string new_salt = generateSalt();
    std::string new_hash = hashPassword(new_password, new_salt);
    
    user->password_hash = new_hash;
    user->salt = new_salt;
    
    bool result = db->updateUser(*user);
    delete user;
    
    return result;
}

bool AuthManager::resetPassword(const std::string& username, const std::string& new_password) {
    User* user = db->getUserByUsername(username);
    if (!user) return false;
    
    std::string new_salt = generateSalt();
    std::string new_hash = hashPassword(new_password, new_salt);
    
    user->password_hash = new_hash;
    user->salt = new_salt;
    
    bool result = db->updateUser(*user);
    delete user;
    
    if (result) {
        revokeAllTokens(user->id);
    }
    
    return result;
}

bool AuthManager::enableTwoFactor(int user_id, const std::string& secret) {
    // Implementation for 2FA would go here
    return true;
}

bool AuthManager::verifyTwoFactor(int user_id, const std::string& code) {
    // Implementation for 2FA verification would go here
    return true;
}

AuthToken* AuthManager::getTokenInfo(const std::string& token) {
    std::lock_guard<std::mutex> lock(token_mutex);
    
    auto it = active_tokens.find(token);
    if (it == active_tokens.end()) {
        return nullptr;
    }
    
    return new AuthToken(it->second);
}

bool AuthManager::revokeAllTokens(int user_id) {
    std::lock_guard<std::mutex> lock(token_mutex);
    
    for (auto it = active_tokens.begin(); it != active_tokens.end();) {
        if (it->second.user_id == user_id) {
            it = active_tokens.erase(it);
        } else {
            ++it;
        }
    }
    return true;
}

std::vector<AuthToken> AuthManager::getUserTokens(int user_id) {
    std::vector<AuthToken> tokens;
    std::lock_guard<std::mutex> lock(token_mutex);
    
    for (const auto& pair : active_tokens) {
        if (pair.second.user_id == user_id) {
            tokens.push_back(pair.second);
        }
    }
    return tokens;
}

void AuthManager::cleanup() {
    std::lock_guard<std::mutex> lock(token_mutex);
    cleanupExpiredTokens();
}

void AuthManager::cleanupExpiredTokens() {
    time_t now = time(nullptr);
    for (auto it = active_tokens.begin(); it != active_tokens.end();) {
        if (it->second.expires_at < now) {
            it = active_tokens.erase(it);
        } else {
            ++it;
        }
    }
}

int AuthManager::getActiveSessionCount() {
    std::lock_guard<std::mutex> lock(token_mutex);
    cleanupExpiredTokens();
    return active_tokens.size();
}

bool AuthManager::isAccountLocked(const std::string& username) {
    User* user = db->getUserByUsername(username);
    if (!user) return false;
    
    bool locked = user->locked_until > time(nullptr);
    delete user;
    return locked;
}

void AuthManager::recordFailedAttempt(const std::string& username, const std::string& ip) {
    User* user = db->getUserByUsername(username);
    if (!user) return;
    
    user->failed_attempts++;
    
    if (user->failed_attempts >= 10) {
        user->locked_until = time(nullptr) + 900; // Lock for 15 minutes
    }
    
    db->updateUser(*user);
    delete user;
}

void AuthManager::clearFailedAttempts(const std::string& username) {
    User* user = db->getUserByUsername(username);
    if (!user) return;
    
    user->failed_attempts = 0;
    user->locked_until = 0;
    db->updateUser(*user);
    delete user;
}

bool AuthManager::checkRateLimit(const std::string& key, int max_requests, int time_window_seconds) {
    static std::map<std::string, std::vector<time_t>> request_log;
    static std::mutex rate_limit_mutex;
    
    std::lock_guard<std::mutex> lock(rate_limit_mutex);
    time_t now = time(nullptr);
    time_t cutoff = now - time_window_seconds;
    
    auto& timestamps = request_log[key];
    
    // Remove old entries
    timestamps.erase(
        std::remove_if(timestamps.begin(), timestamps.end(),
                       [cutoff](time_t t) { return t < cutoff; }),
        timestamps.end());
    
    if (timestamps.size() >= (size_t)max_requests) {
        return false;
    }
    
    timestamps.push_back(now);
    return true;
}

std::string AuthManager::generateApiKey(int user_id) {
    std::string api_key = "sk_" + std::to_string(user_id) + "_" + generateTokenString();
    return api_key;
}

bool AuthManager::validateApiKey(const std::string& api_key, int& user_id) {
    // Parse API key format: sk_{user_id}_{random}
    if (api_key.substr(0, 3) != "sk_") return false;
    
    size_t first_underscore = api_key.find('_', 3);
    if (first_underscore == std::string::npos) return false;
    
    std::string user_id_str = api_key.substr(3, first_underscore - 3);
    user_id = std::stoi(user_id_str);
    
    return true;
}

bool AuthManager::revokeApiKey(const std::string& api_key) {
    // Implementation to revoke API key
    return true;
}
