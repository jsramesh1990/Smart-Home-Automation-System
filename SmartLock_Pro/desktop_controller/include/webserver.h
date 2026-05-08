#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <string>
#include <map>
#include <functional>
#include <vector>

enum HttpMethod {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_OPTIONS
};

struct HttpRequest {
    HttpMethod method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> queryParams;
    std::string body;
    std::string clientIp;
};

struct HttpResponse {
    int statusCode;
    std::map<std::string, std::string> headers;
    std::string body;
    
    HttpResponse() : statusCode(200) {
        headers["Content-Type"] = "application/json";
        headers["Access-Control-Allow-Origin"] = "*";
    }
};

typedef std::function<HttpResponse(const HttpRequest&)> RouteHandler;

class WebServer {
private:
    int serverSocket;
    int port;
    std::map<std::string, RouteHandler> getRoutes;
    std::map<std::string, RouteHandler> postRoutes;
    std::map<std::string, RouteHandler> putRoutes;
    std::map<std::string, RouteHandler> deleteRoutes;
    
    bool running;
    void* serverThread;
    
    void handleClient(int clientSocket);
    HttpRequest parseRequest(const std::string& rawRequest);
    void sendResponse(int clientSocket, const HttpResponse& response);
    std::string getMimeType(const std::string& path);
    
public:
    WebServer(int port = 8080);
    ~WebServer();
    
    bool start();
    void stop();
    
    void get(const std::string& path, RouteHandler handler);
    void post(const std::string& path, RouteHandler handler);
    void put(const std::string& path, RouteHandler handler);
    void del(const std::string& path, RouteHandler handler);
    
    static void* serverLoop(void* arg);
};

#endif
