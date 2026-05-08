#include "include/webserver.h"
#include "include/database.h"
#include "include/auth_manager.h"
#include "include/access_logger.h"
#include "include/websocket_server.h"
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

Database db("smartlock.db");
AuthManager auth(db);
AccessLogger logger(db);
WebServer webServer(8080);
WebSocketServer wsServer(8081);

// Global state
struct LockState {
    bool locked;
    std::string lastUser;
    time_t lastAction;
} currentState = {true, "", 0};

// API Handlers
json handleLogin(const json& request) {
    std::string username = request["username"];
    std::string password = request["password"];
    
    User* user = auth.authenticate(username, password);
    if (user) {
        std::string token = auth.generateToken(user->id);
        delete user;
        
        return json{
            {"success", true},
            {"token", token},
            {"message", "Login successful"}
        };
    }
    
    return json{
        {"success", false},
        {"message", "Invalid credentials"}
    };
}

json handleLockControl(const std::string& action, int userId) {
    if (action == "lock") {
        currentState.locked = true;
        currentState.lastUser = std::to_string(userId);
        currentState.lastAction = time(nullptr);
        
        logger.log(userId, action, "app", "Lock command executed", true);
        
        // Broadcast via WebSocket
        wsServer.broadcast(json{{"event", "state_change"}, {"locked", true}});
        
        return json{{"success", true}, {"state", "locked"}};
    } else if (action == "unlock") {
        currentState.locked = false;
        currentState.lastUser = std::to_string(userId);
        currentState.lastAction = time(nullptr);
        
        logger.log(userId, action, "app", "Unlock command executed", true);
        
        wsServer.broadcast(json{{"event", "state_change"}, {"locked", false}});
        
        return json{{"success", true}, {"state", "unlocked"}};
    }
    
    return json{{"success", false}, {"message", "Invalid action"}};
}

json handleCreateGuestCode(const json& request, int userId) {
    GuestCode code;
    code.userId = std::to_string(userId);
    code.maxUses = request.value("max_uses", 1);
    code.usesRemaining = code.maxUses;
    code.active = true;
    
    // Generate random 6-digit code
    code.code = std::to_string(rand() % 900000 + 100000);
    code.expiresAt = time(nullptr) + request.value("duration_hours", 24) * 3600;
    
    if (db.createGuestCode(code)) {
        logger.log(userId, "create_guest_code", "app", 
                  "Created guest code: " + code.code, true);
        
        return json{
            {"success", true},
            {"code", code.code},
            {"expires_at", code.expiresAt}
        };
    }
    
    return json{{"success", false}, {"message", "Failed to create code"}};
}

json handleGetAccessLogs(int userId, int limit) {
    std::vector<AccessLog> logs = db.getAccessLogs(0, time(nullptr), limit);
    json logsArray = json::array();
    
    for (const auto& log : logs) {
        logsArray.push_back({
            {"id", log.id},
            {"user_id", log.userId},
            {"action", log.action},
            {"method", log.method},
            {"details", log.details},
            {"timestamp", log.timestamp},
            {"success", log.success}
        });
    }
    
    return json{
        {"success", true},
        {"logs", logsArray},
        {"count", logsArray.size()}
    };
}

// Route handlers
HttpResponse handleApiRoute(const HttpRequest& req) {
    HttpResponse res;
    json response;
    
    if (req.path == "/api/login" && req.method == HTTP_POST) {
        json request = json::parse(req.body);
        response = handleLogin(request);
        
    } else if (req.path == "/api/lock" && req.method == HTTP_POST) {
        // Extract user ID from token
        std::string token = req.headers.at("Authorization");
        int userId = auth.verifyToken(token);
        
        if (userId > 0) {
            response = handleLockControl("lock", userId);
        } else {
            response = {{"success", false}, {"message", "Unauthorized"}};
        }
        
    } else if (req.path == "/api/unlock" && req.method == HTTP_POST) {
        std::string token = req.headers.at("Authorization");
        int userId = auth.verifyToken(token);
        
        if (userId > 0) {
            response = handleLockControl("unlock", userId);
        } else {
            response = {{"success", false}, {"message", "Unauthorized"}};
        }
        
    } else if (req.path == "/api/status" && req.method == HTTP_GET) {
        response = {
            {"success", true},
            {"locked", currentState.locked},
            {"last_user", currentState.lastUser},
            {"last_action", currentState.lastAction}
        };
        
    } else if (req.path == "/api/guest-codes" && req.method == HTTP_POST) {
        std::string token = req.headers.at("Authorization");
        int userId = auth.verifyToken(token);
        
        if (userId > 0) {
            json request = json::parse(req.body);
            response = handleCreateGuestCode(request, userId);
        } else {
            response = {{"success", false}, {"message", "Unauthorized"}};
        }
        
    } else if (req.path == "/api/logs" && req.method == HTTP_GET) {
        std::string token = req.headers.at("Authorization");
        int userId = auth.verifyToken(token);
        
        if (userId > 0) {
            int limit = req.queryParams.count("limit") ? 
                       std::stoi(req.queryParams["limit"]) : 100;
            response = handleGetAccessLogs(userId, limit);
        } else {
            response = {{"success", false}, {"message", "Unauthorized"}};
        }
        
    } else {
        res.statusCode = 404;
        response = {{"success", false}, {"message", "Not found"}};
    }
    
    res.body = response.dump();
    return res;
}

HttpResponse handleStaticFiles(const HttpRequest& req) {
    HttpResponse res;
    
    if (req.path == "/") {
        res.headers["Content-Type"] = "text/html";
        res.body = R"(
            <!DOCTYPE html>
            <html>
            <head>
                <title>SmartLock Pro Controller</title>
                <style>
                    body { font-family: Arial; text-align: center; padding: 50px; }
                    button { padding: 15px 30px; font-size: 18px; margin: 10px; }
                    .lock { background: #ff4444; color: white; }
                    .unlock { background: #44ff44; color: black; }
                    #status { font-size: 24px; margin: 20px; }
                </style>
            </head>
            <body>
                <h1>SmartLock Pro Controller</h1>
                <div id="status">Status: Unknown</div>
                <button class="lock" onclick="control('lock')">🔒 LOCK</button>
                <button class="unlock" onclick="control('unlock')">🔓 UNLOCK</button>
                <script>
                    async function control(action) {
                        const res = await fetch('/api/' + action, {
                            method: 'POST',
                            headers: {'Authorization': localStorage.getItem('token')}
                        });
                        const data = await res.json();
                        alert(data.message);
                        updateStatus();
                    }
                    async function updateStatus() {
                        const res = await fetch('/api/status');
                        const data = await res.json();
                        document.getElementById('status').innerHTML = 
                            'Status: ' + (data.locked ? '🔒 LOCKED' : '🔓 UNLOCKED');
                    }
                    setInterval(updateStatus, 5000);
                    updateStatus();
                </script>
            </body>
            </html>
        )";
    } else {
        res.statusCode = 404;
        res.body = "Not found";
    }
    
    return res;
}

int main() {
    std::cout << "SmartLock Pro Desktop Controller v1.0" << std::endl;
    std::cout << "======================================" << std::endl;
    
    // Initialize database
    if (!db.initialize()) {
        std::cerr << "Failed to initialize database" << std::endl;
        return 1;
    }
    std::cout << "Database initialized" << std::endl;
    
    // Setup web server routes
    webServer.get("/", handleStaticFiles);
    webServer.post("/api/login", handleApiRoute);
    webServer.post("/api/lock", handleApiRoute);
    webServer.post("/api/unlock", handleApiRoute);
    webServer.get("/api/status", handleApiRoute);
    webServer.post("/api/guest-codes", handleApiRoute);
    webServer.get("/api/logs", handleApiRoute);
    
    // Start web server
    if (!webServer.start()) {
        std::cerr << "Failed to start web server" << std::endl;
        return 1;
    }
    std::cout << "Web server started on port 8080" << std::endl;
    
    // Start WebSocket server
    if (!wsServer.start()) {
        std::cerr << "Failed to start WebSocket server" << std::endl;
    } else {
        std::cout << "WebSocket server started on port 8081" << std::endl;
    }
    
    std::cout << "\nSmartLock Pro is running!" << std::endl;
    std::cout << "Access the control panel at: http://localhost:8080" << std::endl;
    std::cout << "\nPress Ctrl+C to stop..." << std::endl;
    
    // Main loop
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}
