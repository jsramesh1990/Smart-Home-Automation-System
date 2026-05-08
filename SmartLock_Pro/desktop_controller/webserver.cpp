#include "include/webserver.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cctype>

WebServer::WebServer(int port) : port(port) {
    server_fd = -1;
    running = false;
    server_thread = nullptr;
    enable_cors = true;
    enable_logging = true;
}

WebServer::~WebServer() {
    stop();
}

bool WebServer::initializeSocket() {
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
    
    if (listen(server_fd, 100) < 0) {
        close(server_fd);
        return false;
    }
    
    return true;
}

void WebServer::acceptClients() {
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
                std::thread(&WebServer::handleClient, this, client_fd).detach();
            }
        }
    }
}

void WebServer::handleClient(int client_fd) {
    char buffer[65536];
    ssize_t n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    
    if (n <= 0) {
        close(client_fd);
        return;
    }
    
    buffer[n] = '\0';
    std::string raw_request(buffer);
    
    HttpRequest request = parseRequest(raw_request);
    request.client_ip = "unknown"; // Would get from socket
    
    if (enable_logging) {
        logRequest(request, HttpResponse());
    }
    
    HttpResponse response = processRequest(request);
    
    if (enable_cors) {
        response.setCORS();
    }
    
    sendResponse(client_fd, response);
    close(client_fd);
}

HttpRequest WebServer::parseRequest(const std::string& raw_request) {
    HttpRequest request;
    std::istringstream stream(raw_request);
    std::string line;
    
    // Parse request line
    std::getline(stream, line);
    std::istringstream request_line(line);
    std::string method_str, path_str, protocol;
    request_line >> method_str >> path_str >> protocol;
    
    // Parse method
    if (method_str == "GET") request.method = HTTP_GET;
    else if (method_str == "POST") request.method = HTTP_POST;
    else if (method_str == "PUT") request.method = HTTP_PUT;
    else if (method_str == "DELETE") request.method = HTTP_DELETE;
    else if (method_str == "OPTIONS") request.method = HTTP_OPTIONS;
    else request.method = HTTP_GET;
    
    request.protocol = protocol;
    
    // Parse path and query parameters
    size_t query_pos = path_str.find('?');
    if (query_pos != std::string::npos) {
        request.path = urlDecode(path_str.substr(0, query_pos));
        std::string query_string = path_str.substr(query_pos + 1);
        
        std::istringstream query_stream(query_string);
        std::string param;
        while (std::getline(query_stream, param, '&')) {
            size_t eq_pos = param.find('=');
            if (eq_pos != std::string::npos) {
                std::string key = urlDecode(param.substr(0, eq_pos));
                std::string value = urlDecode(param.substr(eq_pos + 1));
                request.query_params[key] = value;
            }
        }
    } else {
        request.path = urlDecode(path_str);
    }
    
    // Parse headers
    while (std::getline(stream, line) && line != "\r" && !line.empty()) {
        if (line.back() == '\r') line.pop_back();
        
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 1);
            
            // Trim leading/trailing spaces
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            request.headers[key] = value;
        }
    }
    
    // Parse body
    size_t body_pos = raw_request.find("\r\n\r\n");
    if (body_pos != std::string::npos) {
        request.body = raw_request.substr(body_pos + 4);
    }
    
    return request;
}

HttpResponse WebServer::processRequest(const HttpRequest& request) {
    HttpResponse response;
    response.setCORS();
    
    // Apply middleware
    for (const auto& middleware_handler : middleware) {
        middleware_handler(request, response);
        if (response.status_code != 200) {
            return response;
        }
    }
    
    // Route handling
    RouteHandler handler = nullptr;
    
    switch (request.method) {
        case HTTP_GET:
            if (get_routes.find(request.path) != get_routes.end()) {
                handler = get_routes[request.path];
            } else if (!static_dir.empty()) {
                serveStaticFile(request.path, response);
                return response;
            }
            break;
        case HTTP_POST:
            if (post_routes.find(request.path) != post_routes.end()) {
                handler = post_routes[request.path];
            }
            break;
        case HTTP_PUT:
            if (put_routes.find(request.path) != put_routes.end()) {
                handler = put_routes[request.path];
            }
            break;
        case HTTP_DELETE:
            if (delete_routes.find(request.path) != delete_routes.end()) {
                handler = delete_routes[request.path];
            }
            break;
        case HTTP_OPTIONS:
            response.status_code = 200;
            response.setCORS();
            return response;
        default:
            break;
    }
    
    if (handler) {
        response = handler(request);
    } else {
        response.status_code = 404;
        response.setHeader("Content-Type", "application/json");
        response.body = "{\"error\":\"Not Found\",\"path\":\"" + request.path + "\"}";
    }
    
    return response;
}

void WebServer::serveStaticFile(const std::string& path, HttpResponse& response) {
    std::string full_path = static_dir + path;
    
    // Security: prevent directory traversal
    if (full_path.find("..") != std::string::npos) {
        response.status_code = 403;
        response.body = "Forbidden";
        return;
    }
    
    // Check if file exists
    if (!std::filesystem::exists(full_path)) {
        response.status_code = 404;
        response.body = "Not Found";
        return;
    }
    
    // Check if it's a directory
    if (std::filesystem::is_directory(full_path)) {
        full_path += "/index.html";
        if (!std::filesystem::exists(full_path)) {
            response.status_code = 403;
            response.body = "Directory listing disabled";
            return;
        }
    }
    
    std::ifstream file(full_path, std::ios::binary);
    if (!file.is_open()) {
        response.status_code = 500;
        response.body = "Internal Server Error";
        return;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    response.body = buffer.str();
    response.setHeader("Content-Type", getMimeType(full_path));
    response.status_code = 200;
}

std::string WebServer::getMimeType(const std::string& path) {
    std::string ext = path.substr(path.find_last_of('.') + 1);
    
    if (ext == "html") return "text/html";
    if (ext == "css") return "text/css";
    if (ext == "js") return "application/javascript";
    if (ext == "json") return "application/json";
    if (ext == "png") return "image/png";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "gif") return "image/gif";
    if (ext == "svg") return "image/svg+xml";
    if (ext == "ico") return "image/x-icon";
    
    return "application/octet-stream";
}

void WebServer::sendResponse(int client_fd, const HttpResponse& response) {
    std::string response_str;
    
    // Status line
    std::string status_text;
    switch (response.status_code) {
        case 200: status_text = "OK"; break;
        case 201: status_text = "Created"; break;
        case 204: status_text = "No Content"; break;
        case 301: status_text = "Moved Permanently"; break;
        case 302: status_text = "Found"; break;
        case 400: status_text = "Bad Request"; break;
        case 401: status_text = "Unauthorized"; break;
        case 403: status_text = "Forbidden"; break;
        case 404: status_text = "Not Found"; break;
        case 405: status_text = "Method Not Allowed"; break;
        case 429: status_text = "Too Many Requests"; break;
        case 500: status_text = "Internal Server Error"; break;
        default: status_text = "Unknown";
    }
    
    response_str = "HTTP/1.1 " + std::to_string(response.status_code) + " " + status_text + "\r\n";
    
    // Headers
    response_str += "Content-Length: " + std::to_string(response.body.length()) + "\r\n";
    for (const auto& header : response.headers) {
        response_str += header.first + ": " + header.second + "\r\n";
    }
    response_str += "Connection: " + std::string(response.close_connection ? "close" : "keep-alive") + "\r\n";
    response_str += "\r\n";
    
    // Body
    response_str += response.body;
    
    send(client_fd, response_str.c_str(), response_str.length(), 0);
}

std::string WebServer::urlDecode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '%') {
            if (i + 2 < str.length()) {
                int value;
                std::istringstream hex_stream(str.substr(i + 1, 2));
                hex_stream >> std::hex >> value;
                result += static_cast<char>(value);
                i += 2;
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

std::string WebServer::getCurrentTime() {
    time_t now = time(nullptr);
    struct tm* tm_info = localtime(&now);
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    return std::string(buffer);
}

void WebServer::logRequest(const HttpRequest& request, const HttpResponse& response) {
    std::string method_str;
    switch (request.method) {
        case HTTP_GET: method_str = "GET"; break;
        case HTTP_POST: method_str = "POST"; break;
        case HTTP_PUT: method_str = "PUT"; break;
        case HTTP_DELETE: method_str = "DELETE"; break;
        default: method_str = "UNKNOWN";
    }
    
    printf("[%s] %s %s\n", getCurrentTime().c_str(), method_str.c_str(), request.path.c_str());
}

bool WebServer::start() {
    if (!initializeSocket()) {
        return false;
    }
    
    running = true;
    server_thread = new std::thread(&WebServer::acceptClients, this);
    
    return true;
}

void WebServer::stop() {
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
}

void WebServer::setStaticDir(const std::string& dir) {
    static_dir = dir;
}

void WebServer::enableCORS(bool enable) {
    enable_cors = enable;
}

void WebServer::enableLogging(bool enable) {
    enable_logging = enable;
}

void WebServer::use(MiddlewareHandler handler) {
    middleware.push_back(handler);
}

void WebServer::get(const std::string& path, RouteHandler handler) {
    get_routes[path] = handler;
}

void WebServer::post(const std::string& path, RouteHandler handler) {
    post_routes[path] = handler;
}

void WebServer::put(const std::string& path, RouteHandler handler) {
    put_routes[path] = handler;
}

void WebServer::del(const std::string& path, RouteHandler handler) {
    delete_routes[path] = handler;
}

void WebServer::setNotFoundHandler(RouteHandler handler) {
    get_routes["*"] = handler;
    post_routes["*"] = handler;
    put_routes["*"] = handler;
    delete_routes["*"] = handler;
}

void WebServer::setErrorHandler(RouteHandler handler) {
    // Store error handler (not fully implemented)
}
