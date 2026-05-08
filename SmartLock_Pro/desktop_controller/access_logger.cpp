#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>

struct WebSocketClient {
    int fd;
    std::string session_id;
    int user_id;
    std::string username;
    std::string role;
    std::string ip_address;
    time_t connected_at;
    bool authenticated;
};

struct WebSocketMessage {
    std::string type;
    std::string event;
    std::string data;
    std::string target_client;
    std::vector<std::string> target_roles;
};

typedef std::function<void(int client_fd, const std::string& message)> WebSocketHandler;
typedef std::function<void(int client_fd, bool connected)> ConnectionHandler;

class WebSocketServer {
private:
    int server_fd;
    int port;
    std::atomic<bool> running;
    std::thread* server_thread;
    std::thread* broadcast_thread;
    
    std::map<int, WebSocketClient> clients;
    std::mutex clients_mutex;
    
    std::map<std::string, WebSocketHandler> event_handlers;
    std::vector<WebSocketMessage> message_queue;
    std::mutex queue_mutex;
    
    ConnectionHandler on_connect_callback;
    ConnectionHandler on_disconnect_callback;
    
    bool initializeSocket();
    void acceptClients();
    void handleClient(int client_fd);
    void processMessages();
    void broadcastLoop();
    
    std::string generateSessionId();
    std::string readWebSocketFrame(int fd);
    bool writeWebSocketFrame(int fd, const std::string& data);
    int performHandshake(int client_fd, const std::string& key);
    
public:
    WebSocketServer(int port = 8081);
    ~WebSocketServer();
    
    bool start();
    void stop();
    void setConnectionHandler(ConnectionHandler on_connect, ConnectionHandler on_disconnect);
    
    // Event handling
    void on(const std::string& event, WebSocketHandler handler);
    void emit(int client_fd, const std::string& event, const std::string& data);
    void broadcast(const std::string& event, const std::string& data);
    void broadcastToRole(const std::string& role, const std::string& event, const std::string& data);
    void broadcastToUser(int user_id, const std::string& event, const std::string& data);
    
    // Client management
    bool authenticateClient(int client_fd, int user_id, const std::string& username, const std::string& role);
    void disconnectClient(int client_fd);
    std::vector<WebSocketClient> getConnectedClients();
    int getClientCount();
    
    // Status
    bool isRunning() const { return running; }
    int getPort() const { return port; }
    bool isClientAuthenticated(int client_fd);
};

#endif
