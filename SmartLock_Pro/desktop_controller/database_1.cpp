#include "include/database.h"
#include <iostream>
#include <sstream>

Database::Database(const std::string& path) : dbPath(path), db(nullptr) {}

Database::~Database() {
    if (db) {
        sqlite3_close(db);
    }
}

bool Database::initialize() {
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    
    createTables();
    return true;
}

void Database::createTables() {
    // Users table
    const char* usersSQL = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            email TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            salt TEXT NOT NULL,
            role TEXT DEFAULT 'user',
            created_at INTEGER NOT NULL,
            active INTEGER DEFAULT 1
        );
    )";
    
    // Access logs table
    const char* logsSQL = R"(
        CREATE TABLE IF NOT EXISTS access_logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            action TEXT NOT NULL,
            method TEXT NOT NULL,
            details TEXT,
            timestamp INTEGER NOT NULL,
            success INTEGER DEFAULT 1,
            FOREIGN KEY(user_id) REFERENCES users(id)
        );
    )";
    
    // Guest codes table
    const char* guestSQL = R"(
        CREATE TABLE IF NOT EXISTS guest_codes (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            code TEXT UNIQUE NOT NULL,
            user_id TEXT NOT NULL,
            expires_at INTEGER NOT NULL,
            max_uses INTEGER DEFAULT 1,
            uses_remaining INTEGER DEFAULT 1,
            active INTEGER DEFAULT 1
        );
    )";
    
    // Voice profiles table
    const char* voiceSQL = R"(
        CREATE TABLE IF NOT EXISTS voice_profiles (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER UNIQUE NOT NULL,
            features BLOB NOT NULL,
            threshold REAL DEFAULT 0.75,
            enrolled_at INTEGER NOT NULL,
            FOREIGN KEY(user_id) REFERENCES users(id)
        );
    )";
    
    executeQuery(usersSQL);
    executeQuery(logsSQL);
    executeQuery(guestSQL);
    executeQuery(voiceSQL);
    
    // Create indexes
    executeQuery("CREATE INDEX IF NOT EXISTS idx_logs_timestamp ON access_logs(timestamp);");
    executeQuery("CREATE INDEX IF NOT EXISTS idx_logs_user ON access_logs(user_id);");
    executeQuery("CREATE INDEX IF NOT EXISTS idx_guest_code ON guest_codes(code);");
    executeQuery("CREATE INDEX IF NOT EXISTS idx_guest_expires ON guest_codes(expires_at);");
}

bool Database::executeQuery(const std::string& sql) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
    
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool Database::createUser(const User& user) {
    std::stringstream sql;
    sql << "INSERT INTO users (username, email, password_hash, salt, role, created_at, active) "
        << "VALUES ('" << user.username << "', '" << user.email << "', '"
        << user.passwordHash << "', '" << user.salt << "', '" << user.role << "', "
        << user.createdAt << ", " << user.active << ");";
    
    return executeQuery(sql.str());
}

User* Database::getUserByUsername(const std::string& username) {
    std::stringstream sql;
    sql << "SELECT id, username, email, password_hash, salt, role, created_at, active "
        << "FROM users WHERE username = '" << username << "';";
    
    sqlite3_stmt* stmt;
    User* user = nullptr;
    
    int rc = sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, nullptr);
    if (rc == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW) {
        user = new User();
        user->id = sqlite3_column_int(stmt, 0);
        user->username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        user->email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        user->passwordHash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        user->salt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        user->role = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        user->createdAt = sqlite3_column_int64(stmt, 6);
        user->active = sqlite3_column_int(stmt, 7);
    }
    
    sqlite3_finalize(stmt);
    return user;
}

bool Database::logAccess(const AccessLog& log) {
    std::stringstream sql;
    sql << "INSERT INTO access_logs (user_id, action, method, details, timestamp, success) "
        << "VALUES (" << log.userId << ", '" << log.action << "', '" << log.method << "', '"
        << log.details << "', " << log.timestamp << ", " << log.success << ");";
    
    return executeQuery(sql.str());
}

std::vector<AccessLog> Database::getAccessLogs(time_t from, time_t to, int limit) {
    std::vector<AccessLog> logs;
    std::stringstream sql;
    sql << "SELECT id, user_id, action, method, details, timestamp, success "
        << "FROM access_logs WHERE timestamp BETWEEN " << from << " AND " << to
        << " ORDER BY timestamp DESC LIMIT " << limit << ";";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, nullptr);
    
    if (rc == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            AccessLog log;
            log.id = sqlite3_column_int(stmt, 0);
            log.userId = sqlite3_column_int(stmt, 1);
            log.action = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            log.method = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            log.details = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            log.timestamp = sqlite3_column_int64(stmt, 5);
            log.success = sqlite3_column_int(stmt, 6);
            logs.push_back(log);
        }
    }
    
    sqlite3_finalize(stmt);
    return logs;
}

bool Database::createGuestCode(const GuestCode& code) {
    std::stringstream sql;
    sql << "INSERT INTO guest_codes (code, user_id, expires_at, max_uses, uses_remaining, active) "
        << "VALUES ('" << code.code << "', '" << code.userId << "', " << code.expiresAt
        << ", " << code.maxUses << ", " << code.usesRemaining << ", " << code.active << ");";
    
    return executeQuery(sql.str());
}

GuestCode* Database::getGuestCode(const std::string& code) {
    std::stringstream sql;
    sql << "SELECT id, code, user_id, expires_at, max_uses, uses_remaining, active "
        << "FROM guest_codes WHERE code = '" << code << "';";
    
    sqlite3_stmt* stmt;
    GuestCode* guestCode = nullptr;
    
    int rc = sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, nullptr);
    if (rc == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW) {
        guestCode = new GuestCode();
        guestCode->id = sqlite3_column_int(stmt, 0);
        guestCode->code = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        guestCode->userId = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        guestCode->expiresAt = sqlite3_column_int64(stmt, 3);
        guestCode->maxUses = sqlite3_column_int(stmt, 4);
        guestCode->usesRemaining = sqlite3_column_int(stmt, 5);
        guestCode->active = sqlite3_column_int(stmt, 6);
    }
    
    sqlite3_finalize(stmt);
    return guestCode;
}

bool Database::useGuestCode(const std::string& code) {
    GuestCode* gc = getGuestCode(code);
    if (!gc || !gc->active || gc->expiresAt < time(nullptr) || gc->usesRemaining <= 0) {
        delete gc;
        return false;
    }
    
    std::stringstream sql;
    sql << "UPDATE guest_codes SET uses_remaining = " << (gc->usesRemaining - 1)
        << " WHERE code = '" << code << "';";
    
    bool result = executeQuery(sql.str());
    delete gc;
    return result;
}

bool Database::cleanupExpiredCodes() {
    std::stringstream sql;
    sql << "UPDATE guest_codes SET active = 0 WHERE expires_at < " << time(nullptr) << ";";
    return executeQuery(sql.str());
}

bool Database::saveVoiceProfile(const VoiceProfile& profile) {
    std::stringstream sql;
    sql << "INSERT OR REPLACE INTO voice_profiles (user_id, features, threshold, enrolled_at) "
        << "VALUES (" << profile.userId << ", ?" << ", " << profile.threshold << ", "
        << profile.enrolledAt << ");";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, nullptr);
    
    if (rc == SQLITE_OK) {
        // Bind blob data
        sqlite3_bind_blob(stmt, 1, profile.features.data(), 
                         profile.features.size() * sizeof(float), SQLITE_STATIC);
        rc = sqlite3_step(stmt);
    }
    
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}
