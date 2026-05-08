#include "include/websocket_server.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <sstream>
#include <random>
#include <sha256.h>
#include <base64.h>

WebSocketServer::WebSocketServer(int port) : port(port) {
    server_fd = -1;
    running = false;
    server_thread = nullptr;
    broadcast_thread = nullptr;
    on_connect_callback = nullptr;
    on_disconnect_callback = nullptr;
}

WebSocketServer::~WebSocketServer() {
    stop();
}

bool WebSocketServer::initializeSocket() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        return false;
    }
    
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(server_fd);
        return false;
    }
    
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        close(server_fd);
        return false;
    }
    
    if (listen(server_fd, 10) < 0) {
        close(server_fd);
        return false;
    }
    
    return true;
}

std::string WebSocketServer::generateSessionId() {
    const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, chars.size() - 1);
    
    std::string session_id;
    for (int i = 0; i < 32; i++) {
        session_id += chars[dis(gen)];
    }
    return session_id;
}

int WebSocketServer::performHandshake(int client_fd, const std::string& key) {
    std::string accept_key = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    
    unsigned char hash[SHA256_DIGEST_SIZE];
    sha256((const uint8_t*)accept_key.c_str(), accept_key.length(), hash);
    
    std::string encoded = base64_encode(hash, SHA256_DIGEST_SIZE);
    
    std::string response = 
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: " + encoded + "\r\n\r\n";
    
    if (send(client_fd, response.c_str(), response.length(), 0) != (ssize_t)response.length()) {
        return -1;
    }
    
    return 0;
}

std::string WebSocketServer::readWebSocketFrame(int fd) {
    uint8_t header[2];
    if (recv(fd, header, 2, 0) != 2) {
        return "";
    }
    
    bool fin = (header[0] & 0x80) != 0;
    uint8_t opcode = header[0] & 0x0F;
    bool masked = (header[1] & 0x80) != 0;
    uint64_t payload_len = header[1] & 0x7F;
    
    if (payload_len == 126) {
        uint8_t ext_len[2];
        if (recv(fd, ext_len, 2, 0) != 2) return "";
        payload_len = (ext_len[0] << 8) | ext_len[1];
    } else if (payload_len == 127) {
        uint8_t ext_len[8];
        if (recv(fd, ext_len, 8, 0) != 8) return "";
        payload_len = 0;
        for (int i = 0; i < 8; i++) {
            payload_len = (payload_len << 8) | ext_len[i];
        }
    }
    
    uint8_t mask[4] = {0};
    if (masked) {
        if (recv(fd, mask, 4, 0) != 4) return "";
    }
    
    std::vector<uint8_t> payload(payload_len);
    size_t received = 0;
    while (received < payload_len) {
        ssize_t n = recv(fd, payload.data() + received, payload_len - received, 0);
        if (n <= 0) return "";
        received += n;
    }
    
    if (masked) {
        for (uint64_t i = 0; i < payload_len; i++) {
            payload[i] ^= mask[i % 4];
        }
    }
    
    if (opcode == 0x08) { // Close frame
        return "";
    }
    
    if (opcode == 0x01) { // Text frame
        return std::string(payload.begin(), payload.end());
    }
    
    return "";
}

bool WebSocketServer::writeWebSocketFrame(int fd, const std::string& data) {
    std::vector<uint8_t> frame;
    
    frame.push_back(0x81); // FIN + Text frame
    
    size_t len = data.length();
    if (len <= 125) {
        frame.push_back(len);
    } else if (len <= 65535) {
        frame.push_back(126);
        frame.push_back((len >> 8) & 0xFF);
        frame.push_back(len & 0xFF);
    } else {
        frame.push_back(127);
        for (int i = 7; i >= 0; i--) {
            frame.push_back((len >> (i * 8)) & 0xFF);
        }
    }
    
    frame.insert(frame.end(), data.begin(), data.end());
    
    if (send(fd, frame.data(), frame.size(), 0) != (ssize_t)frame.size()) {
        return false;
    }
    
    return true;
}

void WebSocketServer::acceptClients() {
    while (running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        
        if (select(server_fd + 1, &readfds, nullptr, nullptr, &tv) > 0) {
            int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            
            if (client_fd >= 0) {
                // Read handshake
                char buffer[4096];
                ssize_t n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
                if (n > 0) {
                    buffer[n] = '\0';
                    std::string request(buffer);
                    
                    size_t key_pos = request.find("Sec-WebSocket-Key: ");
                    if (key_pos != std::string::npos) {
                        size_t end_pos = request.find("\r\n", key_pos);
                        std::string key = request.substr(key_pos + 19, end_pos - (key_pos + 19));
                        
                        if (performHandshake(client_fd, key) == 0) {
                            WebSocketClient client;
                            client.fd = client_fd;
                            client.session_id = generateSessionId();
                            client.user_id = -1;
                            client.username = "";
                            client.role = "";
                            client.ip_address = inet_ntoa(client_addr.sin_addr);
                            client.connected_at = time(nullptr);
                            client.authenticated = false;
                            
                            {
                                std::lock_guard<std::mutex> lock(clients_mutex);
                                clients[client_fd] = client;
                            }
                            
                            if (on_connect_callback) {
                                on_connect_callback(client_fd, true);
                            }
                            
                            // Start client handler thread
                            std::thread(&WebSocketServer::handleClient, this, client_fd).detach();
                        } else {
                            close(client_fd);
                        }
                    } else {
                        close(client_fd);
                    }
                } else {
                    close(client_fd);
                }
            }
        }
    }
}

void WebSocketServer::handleClient(int client_fd) {
    while (running) {
        std::string message = readWebSocketFrame(client_fd);
        
        if (message.empty()) {
            break;
        }
        
        // Parse JSON message
        // Simple parsing for now
        if (message.find("\"event\"") != std::string::npos) {
            // Extract event type
            size_t start = message.find("\"event\":\"") + 9;
            size_t end = message.find("\"", start);
            std::string event = message.substr(start, end - start);
            
            auto it = event_handlers.find(event);
            if (it != event_handlers.end()) {
                it->second(client_fd, message);
            }
        }
    }
    
    disconnectClient(client_fd);
    if (on_disconnect_callback) {
        on_disconnect_callback(client_fd, false);
    }
}

void WebSocketServer::processMessages() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        std::vector<WebSocketMessage> messages;
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            messages.swap(message_queue);
        }
        
        for (const auto& msg : messages) {
            std::string json = "{\"event\":\"" + msg.event + "\",\"data\":" + msg.data + "}";
            
            if (!msg.target_client.empty()) {
                // Direct to specific client
                int fd = std::stoi(msg.target_client);
                writeWebSocketFrame(fd, json);
            } else if (!msg.target_roles.empty()) {
                // Broadcast to roles
                std::lock_guard<std::mutex> lock(clients_mutex);
                for (const auto& pair : clients) {
                    for (const auto& role : msg.target_roles) {
                        if (pair.second.role == role && pair.second.authenticated) {
                            writeWebSocketFrame(pair.first, json);
                        }
                    }
                }
            } else {
                // Broadcast to all
                broadcast(msg.event, msg.data);
            }
        }
    }
}

void WebSocketServer::broadcastLoop() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Send heartbeat ping
        std::lock_guard<std::mutex> lock(clients_mutex);
        for (const auto& pair : clients) {
            std::string ping = "{\"event\":\"ping\",\"data\":{}}";
            writeWebSocketFrame(pair.first, ping);
        }
    }
}

bool WebSocketServer::start() {
    if (!initializeSocket()) {
        return false;
    }
    
    running = true;
    
    server_thread = new std::thread(&WebSocketServer::acceptClients, this);
    broadcast_thread = new std::thread(&WebSocketServer::broadcastLoop, this);
    
    return true;
}

void WebSocketServer::stop() {
    running = false;
    
    if (server_fd >= 0) {
        close(server_fd);
        server_fd = -1;
    }
    
    if (server_thread && server_thread->joinable()) {
        server_thread->join();
        delete server_thread;
        server_thread = nullptr;
    }
    
    if (broadcast_thread && broadcast_thread->joinable()) {
        broadcast_thread->join();
        delete broadcast_thread;
        broadcast_thread = nullptr;
    }
    
    // Disconnect all clients
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (const auto& pair : clients) {
        close(pair.first);
    }
    clients.clear();
}

void WebSocketServer::setConnectionHandler(ConnectionHandler on_connect, ConnectionHandler on_disconnect) {
    on_connect_callback = on_connect;
    on_disconnect_callback = on_disconnect;
}

void WebSocketServer::on(const std::string& event, WebSocketHandler handler) {
    event_handlers[event] = handler;
}

void WebSocketServer::emit(int client_fd, const std::string& event, const std::string& data) {
    std::string json = "{\"event\":\"" + event + "\",\"data\":" + data + "}";
    writeWebSocketFrame(client_fd, json);
}

void WebSocketServer::broadcast(const std::string& event, const std::string& data) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    std::string json = "{\"event\":\"" + event + "\",\"data\":" + data + "}";
    
    for (const auto& pair : clients) {
        if (pair.second.authenticated) {
            writeWebSocketFrame(pair.first, json);
        }
    }
}

void WebSocketServer::broadcastToRole(const std::string& role, const std::string& event, const std::string& data) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    std::string json = "{\"event\":\"" + event + "\",\"data\":" + data + "}";
    
    for (const auto& pair : clients) {
        if (pair.second.role == role && pair.second.authenticated) {
            writeWebSocketFrame(pair.first, json);
        }
    }
}

void WebSocketServer::broadcastToUser(int user_id, const std::string& event, const std::string& data) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    std::string json = "{\"event\":\"" + event + "\",\"data\":" + data + "}";
    
    for (const auto& pair : clients) {
        if (pair.second.user_id == user_id && pair.second.authenticated) {
            writeWebSocketFrame(pair.first, json);
        }
    }
}

bool WebSocketServer::authenticateClient(int client_fd, int user_id, const std::string& username, const std::string& role) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    
    auto it = clients.find(client_fd);
    if (it == clients.end()) {
        return false;
    }
    
    it->second.user_id = user_id;
    it->second.username = username;
    it->second.role = role;
    it->second.authenticated = true;
    
    return true;
}

void WebSocketServer::disconnectClient(int client_fd) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    
    auto it = clients.find(client_fd);
    if (it != clients.end()) {
        close(it->first);
        clients.erase(it);
    }
}

std::vector<WebSocketClient> WebSocketServer::getConnectedClients() {
    std::lock_guard<std::mutex> lock(clients_mutex);
    std::vector<WebSocketClient> result;
    
    for (const auto& pair : clients) {
        result.push_back(pair.second);
    }
    
    return result;
}

int WebSocketServer::getClientCount() {
    std::lock_guard<std::mutex> lock(clients_mutex);
    return clients.size();
}

bool WebSocketServer::isClientAuthenticated(int client_fd) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    
    auto it = clients.find(client_fd);
    if (it == clients.end()) {
        return false;
    }
    
    return it->second.authenticated;
}
