#include "include/ui_controller.h"
#include "include/network_client.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cmath>

static NetworkClient network;

UIController::UIController() {
    running = false;
    update_thread = nullptr;
    
    current_status.locked = true;
    current_status.battery_percentage = 85;
    current_status.wifi_rssi = -45;
    current_status.tamper_detected = false;
    current_status.door_open = false;
}

UIController::~UIController() {
    stop();
}

void UIController::setStatusCallback(StatusUpdateCallback callback) {
    status_callback = callback;
}

void UIController::setLogCallback(LogUpdateCallback callback) {
    log_callback = callback;
}

void UIController::setNotificationCallback(NotificationCallback callback) {
    notification_callback = callback;
}

void UIController::setAuthToken(const std::string& token) {
    auth_token = token;
    network.setAuthToken(token);
}

void UIController::start() {
    running = true;
    update_thread = new std::thread(&UIController::updateLoop, this);
}

void UIController::stop() {
    running = false;
    if (update_thread && update_thread->joinable()) {
        update_thread->join();
        delete update_thread;
        update_thread = nullptr;
    }
}

void UIController::updateLoop() {
    while (running) {
        refreshStatus();
        refreshLogs();
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void UIController::refreshStatus() {
    network.getLockStatus([this](const ApiResponse& response) {
        if (response.success) {
            formatLockStatus(response.body);
        } else {
            showNotification("Failed to get status: " + response.error, true);
        }
    });
}

void UIController::refreshLogs() {
    network.getAccessLogs(50, [this](const ApiResponse& response) {
        if (response.success) {
            formatAccessLogs(response.body);
        }
    });
}

void UIController::lockDoor() {
    showNotification("Locking door...", false);
    
    network.lockDoor([this](const ApiResponse& response) {
        if (response.success) {
            showNotification("Door locked successfully", false);
            refreshStatus();
        } else {
            showNotification("Failed to lock door", true);
        }
    });
}

void UIController::unlockDoor() {
    showNotification("Unlocking door...", false);
    
    network.unlockDoor([this](const ApiResponse& response) {
        if (response.success) {
            showNotification("Door unlocked successfully", false);
            refreshStatus();
        } else {
            showNotification("Failed to unlock door", true);
        }
    });
}

void UIController::createGuestCode(int hours, int uses) {
    network.createGuestCode(hours, uses, [this](const ApiResponse& response) {
        if (response.success) {
            showNotification("Guest code created", false);
            refreshLogs();
        } else {
            showNotification("Failed to create guest code", true);
        }
    });
}

void UIController::revokeGuestCode(const std::string& code) {
    network.del("/api/guest-codes/" + code, [this](const ApiResponse& response) {
        if (response.success) {
            showNotification("Guest code revoked", false);
            refreshLogs();
        } else {
            showNotification("Failed to revoke guest code", true);
        }
    });
}

void UIController::deleteVoiceProfile(const std::string& user_id) {
    network.del("/api/voice/profiles/" + user_id, [this](const ApiResponse& response) {
        if (response.success) {
            showNotification("Voice profile deleted", false);
        } else {
            showNotification("Failed to delete voice profile", true);
        }
    });
}

void UIController::formatLockStatus(const std::string& json) {
    // Parse JSON and update status
    size_t state_pos = json.find("\"state\":\"");
    if (state_pos != std::string::npos) {
        state_pos += 9;
        size_t end_pos = json.find("\"", state_pos);
        std::string state = json.substr(state_pos, end_pos - state_pos);
        current_status.locked = (state == "locked");
    }
    
    size_t battery_pos = json.find("\"battery\":");
    if (battery_pos != std::string::npos) {
        battery_pos += 10;
        size_t end_pos = json.find(",", battery_pos);
        if (end_pos == std::string::npos) end_pos = json.find("}", battery_pos);
        std::string battery_str = json.substr(battery_pos, end_pos - battery_pos);
        current_status.battery_percentage = std::stoi(battery_str);
    }
    
    size_t rssi_pos = json.find("\"wifi_rssi\":");
    if (rssi_pos != std::string::npos) {
        rssi_pos += 12;
        size_t end_pos = json.find(",", rssi_pos);
        if (end_pos == std::string::npos) end_pos = json.find("}", rssi_pos);
        std::string rssi_str = json.substr(rssi_pos, end_pos - rssi_pos);
        current_status.wifi_rssi = std::stoi(rssi_str);
    }
    
    if (status_callback) {
        status_callback(current_status);
    }
}

void UIController::formatAccessLogs(const std::string& json) {
    // Parse JSON and update logs
    access_logs.clear();
    
    // Simple parsing for demonstration
    size_t logs_start = json.find("\"logs\":[");
    if (logs_start == std::string::npos) return;
    
    size_t pos = logs_start + 8;
    int brace_count = 0;
    std::string log_json;
    
    while (pos < json.length()) {
        if (json[pos] == '{') {
            brace_count++;
            log_json += json[pos];
        } else if (json[pos] == '}') {
            brace_count--;
            log_json += json[pos];
            
            if (brace_count == 0) {
                // Parse individual log
                AccessEvent event;
                
                size_t user_pos = log_json.find("\"username\":\"");
                if (user_pos != std::string::npos) {
                    user_pos += 11;
                    size_t end = log_json.find("\"", user_pos);
                    event.username = log_json.substr(user_pos, end - user_pos);
                }
                
                size_t action_pos = log_json.find("\"action\":\"");
                if (action_pos != std::string::npos) {
                    action_pos += 9;
                    size_t end = log_json.find("\"", action_pos);
                    event.action = log_json.substr(action_pos, end - action_pos);
                }
                
                size_t method_pos = log_json.find("\"method\":\"");
                if (method_pos != std::string::npos) {
                    method_pos += 9;
                    size_t end = log_json.find("\"", method_pos);
                    event.method = log_json.substr(method_pos, end - method_pos);
                }
                
                size_t time_pos = log_json.find("\"timestamp\":");
                if (time_pos != std::string::npos) {
                    time_pos += 11;
                    size_t end = log_json.find(",", time_pos);
                    if (end == std::string::npos) end = log_json.find("}", time_pos);
                    std::string time_str = log_json.substr(time_pos, end - time_pos);
                    event.timestamp = std::stoll(time_str);
                }
                
                size_t success_pos = log_json.find("\"success\":");
                if (success_pos != std::string::npos) {
                    success_pos += 10;
                    event.success = (log_json[success_pos] == 't');
                }
                
                access_logs.push_back(event);
                log_json.clear();
            }
        } else if (brace_count > 0) {
            log_json += json[pos];
        }
        pos++;
    }
    
    if (log_callback) {
        log_callback(access_logs);
    }
}

void UIController::showNotification(const std::string& message, bool is_error) {
    if (notification_callback) {
        notification_callback(message, is_error);
    }
    
    // Also print to console
    std::cout << (is_error ? "[ERROR] " : "[INFO] ") << message << std::endl;
}

std::string UIController::getStatusText() const {
    if (current_status.tamper_detected) {
        return "TAMPER DETECTED";
    }
    return current_status.locked ? "LOCKED" : "UNLOCKED";
}

std::string UIController::getBatteryIcon() const {
    if (current_status.battery_percentage >= 80) return "🔋";
    if (current_status.battery_percentage >= 50) return "🔋";
    if (current_status.battery_percentage >= 20) return "🪫";
    return "⚠️";
}

std::string UIController::getWifiIcon() const {
    if (current_status.wifi_rssi > -50) return "📶";
    if (current_status.wifi_rssi > -70) return "📶";
    return "📶";
}

std::string UIController::formatTimestamp(time_t timestamp) const {
    struct tm* tm_info = localtime(&timestamp);
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    return std::string(buffer);
}
