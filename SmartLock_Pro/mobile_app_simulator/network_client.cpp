#include "include/network_client.h"
#include <curl/curl.h>
#include <sstream>
#include <iomanip>
#include <random>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t total_size = size * nmemb;
    output->append((char*)contents, total_size);
    return total_size;
}

NetworkClient::NetworkClient() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    ws_fd = -1;
    ws_connected = false;
    ws_thread = nullptr;
    request_id_counter = 0;
}

NetworkClient::~NetworkClient() {
    disconnectWebSocket();
    curl_global_cleanup();
}

void NetworkClient::setApiBaseUrl(const std::string& url) {
    api_base_url = url;
}

void NetworkClient::setAuthToken(const std::string& token) {
    auth_token = token;
}

std::string NetworkClient::buildUrl(const std::string& endpoint) {
    return api_base_url + endpoint;
}

std::string NetworkClient::buildQueryString(const std::map<std::string, std::string>& params) {
    if (params.empty()) return "";
    
    std::string query = "?";
    bool first = true;
    
    for (const auto& param : params) {
        if (!first) query += "&";
        query += urlEncode(param.first) + "=" + urlEncode(param.second);
        first = false;
    }
    
    return query;
}

std::string NetworkClient::urlEncode(const std::string& str) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;
    
    for (char c : str) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::setw(2) << int((unsigned char)c);
        }
    }
    
    return escaped.str();
}

void NetworkClient::get(const std::string& endpoint, ApiCallback callback,
                         const std::map<std::string, std::string>& params) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        callback({"", "", {}, false, "Failed to initialize curl"});
        return;
    }
    
    std::string url = buildUrl(endpoint) + buildQueryString(params);
    std::string response;
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    
    struct curl_slist* headers = nullptr;
    if (!auth_token.empty()) {
        std::string auth_header = "Authorization: Bearer " + auth_token;
        headers = curl_slist_append(headers, auth_header.c_str());
    }
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    CURLcode res = curl_easy_perform(curl);
    
    ApiResponse api_response;
    api_response.body = response;
    
    if (res == CURLE_OK) {
        long http_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        api_response.status_code = http_code;
        api_response.success = http_code >= 200 && http_code < 300;
    } else {
        api_response.success = false;
        api_response.error = curl_easy_strerror(res);
    }
    
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    
    callback(api_response);
}

void NetworkClient::post(const std::string& endpoint, const std::string& body, ApiCallback callback,
                          const std::map<std::string, std::string>& params) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        callback({"", {}, {}, false, "Failed to initialize curl"});
        return;
    }
    
    std::string url = buildUrl(endpoint) + buildQueryString(params);
    std::string response;
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.size());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    
    struct curl_slist* headers = nullptr;
    if (!auth_token.empty()) {
        std::string auth_header = "Authorization: Bearer " + auth_token;
        headers = curl_slist_append(headers, auth_header.c_str());
    }
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    CURLcode res = curl_easy_perform(curl);
    
    ApiResponse api_response;
    api_response.body = response;
    
    if (res == CURLE_OK) {
        long http_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        api_response.status_code = http_code;
        api_response.success = http_code >= 200 && http_code < 300;
    } else {
        api_response.success = false;
        api_response.error = curl_easy_strerror(res);
    }
    
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    
    callback(api_response);
}

ApiResponse NetworkClient::getSync(const std::string& endpoint) {
    ApiResponse response;
    bool completed = false;
    
    get(endpoint, [&](const ApiResponse& resp) {
        response = resp;
        completed = true;
    });
    
    while (!completed) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    return response;
}

ApiResponse NetworkClient::postSync(const std::string& endpoint, const std::string& body) {
    ApiResponse response;
    bool completed = false;
    
    post(endpoint, body, [&](const ApiResponse& resp) {
        response = resp;
        completed = true;
    });
    
    while (!completed) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    return response;
}

void NetworkClient::login(const std::string& username, const std::string& password, ApiCallback callback) {
    std::string body = "{\"username\":\"" + username + "\",\"password\":\"" + password + "\"}";
    post("/api/login", body, [this, callback](const ApiResponse& response) {
        if (response.success) {
            // Extract token from response
            std::string token = parseJsonValue(response.body, "token");
            if (!token.empty()) {
                setAuthToken(token);
            }
        }
        callback(response);
    });
}

void NetworkClient::getLockStatus(ApiCallback callback) {
    get("/api/status", callback);
}

void NetworkClient::lockDoor(ApiCallback callback) {
    post("/api/lock", "{}", callback);
}

void NetworkClient::unlockDoor(ApiCallback callback) {
    post("/api/unlock", "{}", callback);
}

void NetworkClient::createGuestCode(int duration_hours, int max_uses, ApiCallback callback) {
    std::string body = "{\"duration_hours\":" + std::to_string(duration_hours) + 
                       ",\"max_uses\":" + std::to_string(max_uses) + "}";
    post("/api/guest-codes", body, callback);
}

void NetworkClient::getAccessLogs(int limit, ApiCallback callback) {
    std::map<std::string, std::string> params;
    params["limit"] = std::to_string(limit);
    get("/api/logs", callback, params);
}

void NetworkClient::getVoiceProfiles(ApiCallback callback) {
    get("/api/voice/profiles", callback);
}

void NetworkClient::enrollVoice(const std::string& user_id, const std::string& wake_word, ApiCallback callback) {
    std::string body = "{\"user_id\":\"" + user_id + "\",\"wake_word\":\"" + wake_word + "\"}";
    post("/api/voice/enroll", body, callback);
}

std::string NetworkClient::parseJsonValue(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\":\"";
    size_t start = json.find(search);
    if (start == std::string::npos) return "";
    
    start += search.length();
    size_t end = json.find("\"", start);
    if (end == std::string::npos) return "";
    
    return json.substr(start, end - start);
}

bool NetworkClient::connectWebSocket(const std::string& url, WebSocketCallback callback) {
    ws_url = url;
    ws_callback = callback;
    
    ws_thread = new std::thread(&NetworkClient::websocketThread, this);
    
    // Wait for connection
    for (int i = 0; i < 50 && !ws_connected; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    return ws_connected;
}

void NetworkClient::disconnectWebSocket() {
    ws_connected = false;
    if (ws_fd >= 0) {
        close(ws_fd);
        ws_fd = -1;
    }
    if (ws_thread && ws_thread->joinable()) {
        ws_thread->join();
        delete ws_thread;
        ws_thread = nullptr;
    }
}

void NetworkClient::sendWebSocketMessage(const std::string& event, const std::string& data) {
    if (!ws_connected || ws_fd < 0) return;
    
    std::string message = "{\"event\":\"" + event + "\",\"data\":" + data + "}";
    sendWebSocketFrame(ws_fd, message);
}

void NetworkClient::websocketThread() {
    struct sockaddr_in server_addr;
    
    // Parse URL (simplified)
    std::string host = "localhost";
    int port = 8081;
    
    ws_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (ws_fd < 0) return;
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    
    if (connect(ws_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(ws_fd);
        ws_fd = -1;
        return;
    }
    
    // Perform WebSocket handshake
    std::string key = "dGhlIHNhbXBsZSBub25jZQ==";
    std::string handshake = 
        "GET /ws HTTP/1.1\r\n"
        "Host: localhost:" + std::to_string(port) + "\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: " + key + "\r\n"
        "Sec-WebSocket-Version: 13\r\n\r\n";
    
    send(ws_fd, handshake.c_str(), handshake.length(), 0);
    
    char buffer[4096];
    recv(ws_fd, buffer, sizeof(buffer) - 1, 0);
    
    ws_connected = true;
    
    while (ws_connected) {
        std::string message = readWebSocketFrame(ws_fd);
        if (message.empty()) {
            break;
        }
        
        WebSocketMessage ws_msg;
        ws_msg.raw = message;
        
        // Parse event from JSON
        size_t event_start = message.find("\"event\":\"");
        if (event_start != std::string::npos) {
            event_start += 9;
            size_t event_end = message.find("\"", event_start);
            ws_msg.event = message.substr(event_start, event_end - event_start);
        }
        
        size_t data_start = message.find("\"data\":");
        if (data_start != std::string::npos) {
            data_start += 7;
            ws_msg.data = message.substr(data_start);
            // Remove trailing }
            if (!ws_msg.data.empty() && ws_msg.data.back() == '}') {
                ws_msg.data.pop_back();
            }
        }
        
        if (ws_callback) {
            ws_callback(ws_msg);
        }
    }
    
    ws_connected = false;
    close(ws_fd);
    ws_fd = -1;
}

bool NetworkClient::sendWebSocketFrame(int fd, const std::string& data) {
    std::vector<uint8_t> frame;
    frame.push_back(0x81);  // FIN + Text frame
    
    size_t len = data.length();
    if (len <= 125) {
        frame.push_back(len);
    } else if (len <= 65535) {
        frame.push_back(126);
        frame.push_back((len >> 8) & 0xFF);
        frame.push_back(len & 0xFF);
    }
    
    frame.insert(frame.end(), data.begin(), data.end());
    
    return send(fd, frame.data(), frame.size(), 0) == (ssize_t)frame.size();
}

std::string NetworkClient::readWebSocketFrame(int fd) {
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
    
    if (opcode == 0x08) {  // Close frame
        return "";
    }
    
    if (opcode == 0x01) {  // Text frame
        return std::string(payload.begin(), payload.end());
    }
    
    return "";
}

void NetworkClient::put(const std::string& endpoint, const std::string& body, ApiCallback callback) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        callback({"", {}, {}, false, "Failed to initialize curl"});
        return;
    }
    
    std::string url = buildUrl(endpoint);
    std::string response;
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
    struct curl_slist* headers = nullptr;
    if (!auth_token.empty()) {
        std::string auth_header = "Authorization: Bearer " + auth_token;
        headers = curl_slist_append(headers, auth_header.c_str());
    }
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    CURLcode res = curl_easy_perform(curl);
    
    ApiResponse api_response;
    api_response.body = response;
    
    if (res == CURLE_OK) {
        long http_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        api_response.status_code = http_code;
        api_response.success = http_code >= 200 && http_code < 300;
    } else {
        api_response.success = false;
        api_response.error = curl_easy_strerror(res);
    }
    
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    
    callback(api_response);
}

void NetworkClient::del(const std::string& endpoint, ApiCallback callback) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        callback({"", {}, {}, false, "Failed to initialize curl"});
        return;
    }
    
    std::string url = buildUrl(endpoint);
    std::string response;
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
    struct curl_slist* headers = nullptr;
    if (!auth_token.empty()) {
        std::string auth_header = "Authorization: Bearer " + auth_token;
        headers = curl_slist_append(headers, auth_header.c_str());
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    CURLcode res = curl_easy_perform(curl);
    
    ApiResponse api_response;
    api_response.body = response;
    
    if (res == CURLE_OK) {
        long http_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        api_response.status_code = http_code;
        api_response.success = http_code >= 200 && http_code < 300;
    } else {
        api_response.success = false;
        api_response.error = curl_easy_strerror(res);
    }
    
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    
    callback(api_response);
}
