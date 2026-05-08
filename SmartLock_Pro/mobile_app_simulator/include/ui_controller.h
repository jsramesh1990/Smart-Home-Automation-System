#ifndef UI_CONTROLLER_H
#define UI_CONTROLLER_H

#include <string>
#include <functional>
#include <vector>
#include <atomic>

struct LockStatus {
    bool locked;
    int battery_percentage;
    int wifi_rssi;
    std::string last_action;
    time_t last_action_time;
    bool tamper_detected;
    bool door_open;
};

struct GuestCode {
    std::string code;
    std::string created_by;
    time_t created_at;
    time_t expires_at;
    int max_uses;
    int uses_remaining;
    bool active;
};

struct AccessEvent {
    std::string username;
    std::string action;
    std::string method;
    time_t timestamp;
    bool success;
    std::string details;
};

typedef std::function<void(const LockStatus&)> StatusUpdateCallback;
typedef std::function<void(const std::vector<AccessEvent>&)> LogUpdateCallback;
typedef std::function<void(const std::string& message, bool is_error)> NotificationCallback;

class UIController {
private:
    LockStatus current_status;
    std::vector<GuestCode> guest_codes;
    std::vector<AccessEvent> access_logs;
    
    StatusUpdateCallback status_callback;
    LogUpdateCallback log_callback;
    NotificationCallback notification_callback;
    
    std::atomic<bool> running;
    std::thread* update_thread;
    
    std::string auth_token;
    
    void updateLoop();
    void formatLockStatus(const std::string& json);
    void formatAccessLogs(const std::string& json);
    void showNotification(const std::string& message, bool is_error);
    
public:
    UIController();
    ~UIController();
    
    void setStatusCallback(StatusUpdateCallback callback);
    void setLogCallback(LogUpdateCallback callback);
    void setNotificationCallback(NotificationCallback callback);
    void setAuthToken(const std::string& token);
    
    void start();
    void stop();
    
    // UI Actions
    void lockDoor();
    void unlockDoor();
    void refreshStatus();
    void refreshLogs();
    void createGuestCode(int hours, int uses);
    void revokeGuestCode(const std::string& code);
    void deleteVoiceProfile(const std::string& user_id);
    
    // Getters
    LockStatus getCurrentStatus() const { return current_status; }
    std::vector<GuestCode> getGuestCodes() const { return guest_codes; }
    std::vector<AccessEvent> getAccessLogs() const { return access_logs; }
    
    // Screen helpers
    std::string getStatusText() const;
    std::string getBatteryIcon() const;
    std::string getWifiIcon() const;
    std::string formatTimestamp(time_t timestamp) const;
};

#endif
