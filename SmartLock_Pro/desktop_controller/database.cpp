#include "include/database.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

Database::Database(const std::string& path) : db_path(path), db(nullptr) {}

Database::~Database() {
    close();
}

bool Database::initialize() {
    int rc = sqlite3_open(db_path.c_str(), &db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    
    createTables();
    createIndexes();
    backup();
    
    std::cout << "Database initialized successfully" << std::endl;
    return true;
}

bool Database::close() {
    if (db) {
        int rc = sqlite3_close(db);
        if (rc == SQLITE_OK) {
            db = nullptr;
            return true;
        }
    }
    return false;
}

bool Database::executeQuery(const std::string& sql) {
    std::lock_guard<std::mutex> lock(db_mutex);
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
    
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool Database::tableExists(const std::string& table_name) {
    std::string sql = "SELECT name FROM sqlite_master WHERE type='table' AND name='" + table_name + "';";
    sqlite3_stmt* stmt;
    bool exists = false;
    
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        exists = sqlite3_step(stmt) == SQLITE_ROW;
    }
    
    sqlite3_finalize(stmt);
    return exists;
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
            last_login INTEGER,
            active INTEGER DEFAULT 1,
            failed_attempts INTEGER DEFAULT 0,
            locked_until INTEGER DEFAULT 0
        );
    )";
    
    // Access logs table
    const char* logsSQL = R"(
        CREATE TABLE IF NOT EXISTS access_logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            username TEXT,
            action TEXT NOT NULL,
            method TEXT NOT NULL,
            details TEXT,
            timestamp INTEGER NOT NULL,
            success INTEGER DEFAULT 1,
            ip_address TEXT,
            user_agent TEXT,
            FOREIGN KEY(user_id) REFERENCES users(id)
        );
    )";
    
    // Guest codes table
    const char* guestSQL = R"(
        CREATE TABLE IF NOT EXISTS guest_codes (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            code TEXT UNIQUE NOT NULL,
            created_by INTEGER NOT NULL,
            created_by_name TEXT,
            created_at INTEGER NOT NULL,
            expires_at INTEGER NOT NULL,
            max_uses INTEGER DEFAULT 1,
            uses_remaining INTEGER DEFAULT 1,
            active INTEGER DEFAULT 1,
            notes TEXT,
            FOREIGN KEY(created_by) REFERENCES users(id)
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
            last_used INTEGER,
            verification_count INTEGER DEFAULT 0,
            success_count INTEGER DEFAULT 0,
            FOREIGN KEY(user_id) REFERENCES users(id)
        );
    )";
    
    // Devices table
    const char* devicesSQL = R"(
        CREATE TABLE IF NOT EXISTS devices (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            device_id TEXT UNIQUE NOT NULL,
            name TEXT,
            ip_address TEXT,
            mac_address TEXT,
            firmware_version TEXT,
            last_seen INTEGER,
            active INTEGER DEFAULT 1,
            battery_level INTEGER DEFAULT 100
        );
    )";
    
    executeQuery(usersSQL);
    executeQuery(logsSQL);
    executeQuery(guestSQL);
    executeQuery(voiceSQL);
    executeQuery(devicesSQL);
}

void Database::createIndexes() {
    executeQuery("CREATE INDEX IF NOT EXISTS idx_logs_timestamp ON access_logs(timestamp);");
    executeQuery("CREATE INDEX IF NOT EXISTS idx_logs_user ON access_logs(user_id);");
    executeQuery("CREATE INDEX IF NOT EXISTS idx_logs_action ON access_logs(action);");
    executeQuery("CREATE INDEX IF NOT EXISTS idx_guest_code ON guest_codes(code);");
    executeQuery("CREATE INDEX IF NOT EXISTS idx_guest_expires ON guest_codes(expires_at);");
    executeQuery("CREATE INDEX IF NOT EXISTS idx_guest_active ON guest_codes(active);");
    executeQuery("CREATE INDEX IF NOT EXISTS idx_devices_last_seen ON devices(last_seen);");
}

void Database::backup() {
    // Create backup of database
    std::string backup_path = db_path + ".backup";
    std::ifstream src(db_path, std::ios::binary);
    std::ofstream dst(backup_path, std::ios::binary);
    dst << src.rdbuf();
}

bool Database::createUser(const User& user) {
    std::stringstream sql;
    sql << "INSERT INTO users (username, email, password_hash, salt, role, created_at, active) "
        << "VALUES ('" << user.username << "', '" << user.email << "', '"
        << user.password_hash << "', '" << user.salt << "', '" << user.role << "', "
        << user.created_at << ", " << user.active << ");";
    
    return executeQuery(sql.str());
}

User* Database::getUserByUsername(const std::string& username) {
    std::stringstream sql;
    sql << "SELECT id, username, email, password_hash, salt, role, created_at, last_login, active, failed_attempts, locked_until "
        << "FROM users WHERE username = '" << username << "';";
    
    sqlite3_stmt* stmt;
    User* user = nullptr;
    
    if (sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            user = new User();
            user->id = sqlite3_column_int(stmt, 0);
            user->username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            user->email = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            user->password_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            user->salt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            user->role = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            user->created_at = sqlite3_column_int64(stmt, 6);
            user->last_login = sqlite3_column_int64(stmt, 7);
            user->active = sqlite3_column_int(stmt, 8);
            user->failed_attempts = sqlite3_column_int(stmt, 9);
            user->locked_until = sqlite3_column_int64(stmt, 10);
        }
    }
    
    sqlite3_finalize(stmt);
    return user;
}

bool Database::authenticateUser(const std::string& username, const std::string& password) {
    User* user = getUserByUsername(username);
    if (!user) return false;
    
    if (user->locked_until > time(nullptr)) {
        delete user;
        return false;
    }
    
    // Verify password (would use proper hashing)
    bool authenticated = false; // Compare password hash
    
    updateLoginAttempts(user->id, authenticated);
    
    if (authenticated) {
        std::stringstream sql;
        sql << "UPDATE users SET last_login = " << time(nullptr) 
            << ", failed_attempts = 0 WHERE id = " << user->id;
        executeQuery(sql.str());
    }
    
    delete user;
    return authenticated;
}

bool Database::logAccess(const AccessLog& log) {
    std::stringstream sql;
    sql << "INSERT INTO access_logs (user_id, username, action, method, details, timestamp, success, ip_address, user_agent) "
        << "VALUES (" << log.user_id << ", '" << log.username << "', '" 
        << log.action << "', '" << log.method << "', '" << log.details << "', "
        << log.timestamp << ", " << log.success << ", '" 
        << log.ip_address << "', '" << log.user_agent << "');";
    
    return executeQuery(sql.str());
}

std::vector<AccessLog> Database::getAccessLogs(time_t from, time_t to, int limit, int offset) {
    std::vector<AccessLog> logs;
    std::stringstream sql;
    sql << "SELECT id, user_id, username, action, method, details, timestamp, success, ip_address, user_agent "
        << "FROM access_logs WHERE timestamp BETWEEN " << from << " AND " << to
        << " ORDER BY timestamp DESC LIMIT " << limit << " OFFSET " << offset << ";";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            AccessLog log;
            log.id = sqlite3_column_int(stmt, 0);
            log.user_id = sqlite3_column_int(stmt, 1);
            log.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            log.action = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            log.method = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            log.details = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            log.timestamp = sqlite3_column_int64(stmt, 6);
            log.success = sqlite3_column_int(stmt, 7);
            log.ip_address = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
            log.user_agent = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
            logs.push_back(log);
        }
    }
    
    sqlite3_finalize(stmt);
    return logs;
}

bool Database::createGuestCode(const GuestCode& code) {
    std::stringstream sql;
    sql << "INSERT INTO guest_codes (code, created_by, created_by_name, created_at, expires_at, max_uses, uses_remaining, active, notes) "
        << "VALUES ('" << code.code << "', " << code.created_by << ", '" << code.created_by_name << "', "
        << code.created_at << ", " << code.expires_at << ", " << code.max_uses << ", "
        << code.uses_remaining << ", " << code.active << ", '" << code.notes << "');";
    
    return executeQuery(sql.str());
}

GuestCode* Database::getGuestCode(const std::string& code) {
    std::stringstream sql;
    sql << "SELECT id, code, created_by, created_by_name, created_at, expires_at, max_uses, uses_remaining, active, notes "
        << "FROM guest_codes WHERE code = '" << code << "' AND active = 1;";
    
    sqlite3_stmt* stmt;
    GuestCode* guestCode = nullptr;
    
    if (sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            guestCode = new GuestCode();
            guestCode->id = sqlite3_column_int(stmt, 0);
            guestCode->code = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            guestCode->created_by = sqlite3_column_int(stmt, 2);
            guestCode->created_by_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            guestCode->created_at = sqlite3_column_int64(stmt, 4);
            guestCode->expires_at = sqlite3_column_int64(stmt, 5);
            guestCode->max_uses = sqlite3_column_int(stmt, 6);
            guestCode->uses_remaining = sqlite3_column_int(stmt, 7);
            guestCode->active = sqlite3_column_int(stmt, 8);
            guestCode->notes = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        }
    }
    
    sqlite3_finalize(stmt);
    return guestCode;
}

bool Database::useGuestCode(const std::string& code) {
    GuestCode* gc = getGuestCode(code);
    if (!gc) return false;
    
    if (gc->expires_at < time(nullptr) || gc->uses_remaining <= 0) {
        delete gc;
        return false;
    }
    
    std::stringstream sql;
    sql << "UPDATE guest_codes SET uses_remaining = " << (gc->uses_remaining - 1)
        << " WHERE code = '" << code << "';";
    
    bool result = executeQuery(sql.str());
    delete gc;
    return result;
}

Database::Statistics Database::getStatistics() {
    Statistics stats = {0};
    
    // Get user count
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM users WHERE active = 1;", -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats.total_users = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    
    // Get today's access count
    time_t now = time(nullptr);
    time_t today_start = now - (now % 86400);
    
    std::stringstream sql;
    sql << "SELECT COUNT(*) FROM access_logs WHERE timestamp >= " << today_start;
    
    if (sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats.total_accesses_today = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    
    // Get active guest codes
    if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM guest_codes WHERE active = 1 AND expires_at > strftime('%s', 'now');", -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats.active_guest_codes = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    
    return stats;
}

bool Database::vacuum() {
    return executeQuery("VACUUM;");
}

bool Database::integrityCheck() {
    sqlite3_stmt* stmt;
    bool result = false;
    
    if (sqlite3_prepare_v2(db, "PRAGMA integrity_check;", -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string check = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            result = (check == "ok");
        }
        sqlite3_finalize(stmt);
    }
    
    return result;
}
