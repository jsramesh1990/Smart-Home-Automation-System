#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <string>
#include <map>
#include <functional>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

enum HttpMethod {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_OPTIONS,
    HTTP_HEAD
};

struct HttpRequest {
    HttpMethod method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> query_params;
    std::string body;
    std::string client_ip;
    std::string client_port;
    std::string protocol;
};

struct HttpResponse {
    int status_code;
    std::map<std::string, std::string> headers;
    std::string body;
    bool close_connection;
    
    HttpResponse() : status_code(200), close_connection(false) {
        headers["Content-Type"] = "text/html";
        headers["Server"] = "SmartLockPro/1.0";
    }
    
    void setHeader(const std::string& key, const std::string& value) {
        headers[key] = value;
    }
    
    void setJson() {
        headers["Content-Type"] = "application/json";
    }
    
    void setCORS() {
        headers["Access-Control-Allow-Origin"] = "*";
        headers["Access-Control-Allow-Methods"] = "GET, POST, PUT, DELETE, OPTIONS";
        headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization";
    }
};

typedef std::function<HttpResponse(const HttpRequest&)> RouteHandler;
typedef std::function<void(const HttpRequest&, HttpResponse&)> MiddlewareHandler;

class WebServer {
private:
    int server_fd;
    int port;
    std::atomic<bool> running;
    std::thread* server_thread;
    
    std::map<std::string, RouteHandler> get_routes;
    std::map<std::string, RouteHandler> post_routes;
    std::map<std::string, RouteHandler> put_routes;
    std::map<std::string, RouteHandler> delete_routes;
    std::vector<MiddlewareHandler> middleware;
    
    std::string static_dir;
    bool enable_cors;
    bool enable_logging;
    
    bool initializeSocket();
    void acceptClients();
    void handleClient(int client_fd);
    HttpResponse processRequest(const HttpRequest& request);
    HttpRequest parseRequest(const std::string& raw_request);
    void sendResponse(int client_fd, const HttpResponse& response);
    void serveStaticFile(const std::string& path, HttpResponse& response);
    std::string getMimeType(const std::string& path);
    std::string urlDecode(const std::string& str);
    std::string getCurrentTime();
    void logRequest(const HttpRequest& request, const HttpResponse& response);
    
public:
    WebServer(int port = 8080);
    ~WebServer();
    
    bool start();
    void stop();
    void setStaticDir(const std::string& dir);
    void enableCORS(bool enable);
    void enableLogging(bool enable);
    void use(MiddlewareHandler handler);
    
    // Route handlers
    void get(const std::string& path, RouteHandler handler);
    void post(const std::string& path, RouteHandler handler);
    void put(const std::string& path, RouteHandler handler);
    void del(const std::string& path, RouteHandler handler);
    
    // Default handlers
    void setNotFoundHandler(RouteHandler handler);
    void setErrorHandler(RouteHandler handler);
    
    // Status
    bool isRunning() const { return running; }
    int getPort() const { return port; }
};

#endif
