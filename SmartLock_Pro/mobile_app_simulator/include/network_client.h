#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include <string>
#include <functional>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>

struct ApiResponse {
    int status_code;
    std::string body;
    std::map<std::string, std::string> headers;
    bool success;
    std::string error;
};

struct WebSocketMessage {
    std::string event;
    std::string data;
    std::string raw;
};

typedef std::function<void(const ApiResponse& response)> ApiCallback;
typedef std::function<void(const WebSocketMessage& message)> WebSocketCallback;
typedef std::function<void(bool connected)> ConnectionCallback;

class NetworkClient {
private:
    std::string api_base_url;
    std::string ws_url;
    std::string auth_token;
    
    // WebSocket
    int ws_fd;
    std::atomic<bool> ws_connected;
    std::thread* ws_thread;
    WebSocketCallback ws_callback;
    
    std::mutex request_mutex;
    int request_id_counter;
    std::map<int, ApiCallback> pending_requests;
    
    std::string buildUrl(const std::string& endpoint);
    std::string buildQueryString(const std::map<std::string, std::string>& params);
    std::map<std::string, std::string> parseQueryString(const std::string& query);
    std::string urlEncode(const std::string& str);
    std::string parseJsonValue(const std::string& json, const std::string& key);
    
    // WebSocket implementation
    bool connectWebSocket();
    void disconnectWebSocket();
    void websocketThread();
    bool sendWebSocketFrame(int fd, const std::string& data);
    std::string readWebSocketFrame(int fd);
    
public:
    NetworkClient();
    ~NetworkClient();
    
    void setApiBaseUrl(const std::string& url);
    void setAuthToken(const std::string& token);
    std::string getAuthToken() const { return auth_token; }
    
    // HTTP requests
    void get(const std::string& endpoint, ApiCallback callback,
             const std::map<std::string, std::string>& params = {});
    void post(const std::string& endpoint, const std::string& body, ApiCallback callback,
              const std::map<std::string, std::string>& params = {});
    void put(const std::string& endpoint, const std::string& body, ApiCallback callback);
    void del(const std::string& endpoint, ApiCallback callback);
    
    // Synchronous versions
    ApiResponse getSync(const std::string& endpoint);
    ApiResponse postSync(const std::string& endpoint, const std::string& body);
    
    // WebSocket
    bool connectWebSocket(const std::string& url, WebSocketCallback callback);
    void disconnectWebSocket();
    void sendWebSocketMessage(const std::string& event, const std::string& data);
    bool isWebSocketConnected() const { return ws_connected; }
    void setWebSocketCallback(WebSocketCallback callback) { ws_callback = callback; }
    
    // API specific methods
    void login(const std::string& username, const std::string& password, ApiCallback callback);
    void getLockStatus(ApiCallback callback);
    void lockDoor(ApiCallback callback);
    void unlockDoor(ApiCallback callback);
    void createGuestCode(int duration_hours, int max_uses, ApiCallback callback);
    void getAccessLogs(int limit, ApiCallback callback);
    void getVoiceProfiles(ApiCallback callback);
    void enrollVoice(const std::string& user_id, const std::string& wake_word, ApiCallback callback);
    
    // Status
    bool hasAuthToken() const { return !auth_token.empty(); }
    void clearAuthToken() { auth_token.clear(); }
};

#endif
