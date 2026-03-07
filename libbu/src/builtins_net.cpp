
#include "interpreter.hpp"

#ifdef BU_ENABLE_SOCKETS

// ============================================
// SOCKET MODULE
// ============================================
#include "platform.hpp"
#include "utils.hpp"
#include <cstring>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define SHUT_RDWR SD_BOTH
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
typedef int SOCKET;
#endif

enum class SocketType
{
    TCP_SERVER,
    TCP_CLIENT,
    UDP
};

struct SocketHandle
{
    SOCKET socket;
    SocketType type;
    bool isBlocking;
    bool isConnected;
    uint16_t port;
    std::string host;
};

// Extrair headers de um map
static std::map<std::string, std::string> extractHeaders(Interpreter *vm, Value mapValue)
{
    std::map<std::string, std::string> headers;

    if (!mapValue.isMap())
    {
        return headers;
    }

    MapInstance *map = mapValue.asMap();

    map->table.forEach([&](String *key, Value value)
                       {
                           if (value.isString())
                           {
                               headers[key->chars()] = value.asStringChars();
                           } else if (value.isInt())
                           {
                               headers[key->chars()] = std::to_string(value.asInt());
                           } else if (value.isFloat())
                           {
                               headers[key->chars()] = std::to_string(value.asFloat());
                           }else if (value.isBool())
                           {
                               headers[key->chars()] = std::to_string(value.asBool());
                           }else if (value.isDouble())
                           {
                               headers[key->chars()] = std::to_string(value.asDouble());
                           } 
                           else
                           {
                               vm->runtimeError("Invalid header format");
                           } });

    return headers;
}

// Construir query string de um map
static std::string buildQueryString(Interpreter *vm, Value mapValue)
{
    std::string query;

    if (!mapValue.isMap())
    {
        return query;
    }

    MapInstance *map = mapValue.asMap();
    bool first = true;

    map->table.forEach([&](String *key, Value value)
                       {
        if (!first) {
            query += "&";
        }
        first = false;

        query += key->chars();
        query += "=";

        if (value.isString())
        {
            query += value.asStringChars();
        }
        else if (value.isInt())
        {
            query += std::to_string(value.asInt());
        }
        else if (value.isFloat())
        {
            query += std::to_string(value.asFloat());
        } 
        else if (value.isDouble())
        {
            query += std::to_string(value.asDouble());
        }
        else if (value.isBool())
        {
            query += value.asBool() ? "true" : "false";
        } });

    return query;
}
// URL encode
static std::string urlEncode(const std::string &value)
{
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (char c : value)
    {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
        {
            escaped << c;
        }
        else
        {
            escaped << '%' << std::setw(2) << int((unsigned char)c);
        }
    }

    return escaped.str();
}

// ============================================
// HTTP UTILITIES - Similar ao Python requests
// ============================================

struct HttpResponse
{
    int statusCode;
    std::string statusText;
    std::map<std::string, std::string> headers;
    std::string body;
    bool success;
};

static HttpResponse parseHttpResponse(const std::string &rawResponse)
{
    HttpResponse response;
    response.success = false;
    response.statusCode = 0;

    size_t headerEnd = rawResponse.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
    {
        return response;
    }

    std::string headerSection = rawResponse.substr(0, headerEnd);
    response.body = rawResponse.substr(headerEnd + 4);

    size_t lineEnd = headerSection.find("\r\n");
    std::string statusLine = headerSection.substr(0, lineEnd);

    // Parse status line: "HTTP/1.1 200 OK"
    size_t firstSpace = statusLine.find(' ');
    size_t secondSpace = statusLine.find(' ', firstSpace + 1);
    if (firstSpace != std::string::npos && secondSpace != std::string::npos)
    {
        response.statusCode = std::stoi(statusLine.substr(firstSpace + 1, secondSpace - firstSpace - 1));
        response.statusText = statusLine.substr(secondSpace + 1);
    }

    // Parse headers
    size_t pos = lineEnd + 2;
    while (pos < headerSection.length())
    {
        size_t nextLine = headerSection.find("\r\n", pos);
        if (nextLine == std::string::npos)
            break;

        std::string line = headerSection.substr(pos, nextLine - pos);
        size_t colon = line.find(':');
        if (colon != std::string::npos)
        {
            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 2); // Skip ": "
            response.headers[key] = value;
        }
        pos = nextLine + 2;
    }

    response.success = (response.statusCode >= 200 && response.statusCode < 300);
    return response;
}

static std::vector<SocketHandle *> openSockets;
static int nextSocketId = 1;
static bool wsaInitialized = false;

static void SocketModuleCleanup()
{
    for (auto handle : openSockets)
    {
        if (handle && handle->socket != INVALID_SOCKET)
        {
            shutdown(handle->socket, SHUT_RDWR);
            closesocket(handle->socket);
            delete handle;
        }
    }
    openSockets.clear();

#ifdef _WIN32
    if (wsaInitialized)
    {
        WSACleanup();
        wsaInitialized = false;
    }
#endif
}

int native_socket_init(Interpreter *vm, int argCount, Value *args)
{
    bool result = false;
#ifdef _WIN32
    if (!wsaInitialized)
    {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0)
        {
            vm->runtimeError("WSAStartup failed: %d", result);
            vm->push(vm->makeBool(false));
        }
        wsaInitialized = true;
    }
#endif
    vm->push(vm->makeBool(result));
    return 1;
}

int native_socket_quit(Interpreter *vm, int argCount, Value *args)
{
    SocketModuleCleanup();
    return 0;
}

//
// REQUESTS
//

// =============================================================
// FUNCTION: HTTP GET Completo
// =============================================================
int native_socket_http_get(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isString())
    {
        vm->runtimeError("http_get expects (url, [options_map])");
        return 0;
    }

    std::string url = args[0].asStringChars();

    // --- DEFAULTS ---
    std::map<std::string, std::string> customHeaders;
    std::string queryParams;
    std::string userAgent = "SocketModule/1.0";
    int timeout = 30;

    // --- PARSE OPTIONS ---
    if (argCount >= 2 && args[1].isMap())
    {
        MapInstance *options = args[1].asMap();
        Value val;

        // 1. Headers
        if (options->table.get(vm->makeString("headers").asString(), &val))
        {
            if (val.isMap())
                customHeaders = extractHeaders(vm, val);
        }

        // 2. Params
        if (options->table.get(vm->makeString("params").asString(), &val))
        {
            if (val.isMap())
                queryParams = buildQueryString(vm, val);
        }

        // 3. Timeout
        if (options->table.get(vm->makeString("timeout").asString(), &val))
        {
            if (val.isInt())
                timeout = val.asInt();
        }

        // 4. User Agent Explícito
        if (options->table.get(vm->makeString("user_agent").asString(), &val))
        {
            if (val.isString())
                userAgent = val.asStringChars();
        }
    }

    // --- LÓGICA USER-AGENT (Evita Duplicados) ---
    // Procura se o user agent já está nos headers customizados
    auto itUA = customHeaders.find("User-Agent");
    if (itUA == customHeaders.end())
        itUA = customHeaders.find("user-agent");

    if (itUA != customHeaders.end())
    {
        userAgent = itUA->second;  // Usa o do header
        customHeaders.erase(itUA); // Remove do mapa para não enviar 2x
    }

    // --- CONSTRUÇÃO URL ---
    if (!queryParams.empty())
    {
        url += (url.find('?') != std::string::npos) ? "&" : "?";
        url += queryParams;
    }

    // --- PARSE URL ---
    size_t protoEnd = url.find("://");
    if (protoEnd == std::string::npos)
    {
        vm->runtimeError("Invalid URL");
        return 0;
    }

    std::string protocol = url.substr(0, protoEnd);
    if (protocol == "https")
    {
        vm->runtimeError("HTTPS not supported");
        return 0;
    }

    size_t hostStart = protoEnd + 3;
    size_t pathStart = url.find('/', hostStart);

    std::string host = url.substr(hostStart, pathStart - hostStart);
    std::string path = (pathStart != std::string::npos) ? url.substr(pathStart) : "/";

    int port = 80;
    size_t portPos = host.find(':');
    if (portPos != std::string::npos)
    {
        port = std::stoi(host.substr(portPos + 1));
        host = host.substr(0, portPos);
    }

    // --- SOCKET CONNECT ---
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
    {
        vm->runtimeError("Socket creation failed");
        return 0;
    }

    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv));

    struct hostent *he = gethostbyname(host.c_str());
    if (!he)
    {
        closesocket(sock);
        vm->runtimeError("Host resolution failed");
        return 0;
    }

    sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);

    if (connect(sock, (sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        closesocket(sock);
        vm->runtimeError("Connection failed");
        return 0;
    }

    // --- SEND REQUEST ---
    std::string request = "GET " + path + " HTTP/1.1\r\n";
    request += "Host: " + host + "\r\n";
    request += "User-Agent: " + userAgent + "\r\n";
    request += "Connection: close\r\n";

    for (const auto &header : customHeaders)
    {
        request += header.first + ": " + header.second + "\r\n";
    }
    request += "\r\n";

    if (send(sock, request.c_str(), request.length(), 0) == SOCKET_ERROR)
    {
        closesocket(sock);
        return 0;
    }

    // --- RECEIVE RESPONSE ---
    std::string response;
    char buffer[4096];
    int received;

    while ((received = recv(sock, buffer, sizeof(buffer), 0)) > 0)
    {
        response.append(buffer, received);
    }
    closesocket(sock);

    // --- PARSE & RETURN ---
    HttpResponse httpResp = parseHttpResponse(response);

    Value result = vm->makeMap();
    MapInstance *map = result.asMap();

    map->table.set(vm->makeString("status_code").asString(), vm->makeInt(httpResp.statusCode));
    map->table.set(vm->makeString("status_text").asString(), vm->makeString(httpResp.statusText.c_str()));
    map->table.set(vm->makeString("body").asString(), vm->makeString(httpResp.body.c_str()));
    map->table.set(vm->makeString("success").asString(), vm->makeBool(httpResp.success));
    map->table.set(vm->makeString("url").asString(), vm->makeString(url.c_str()));
    map->table.set(vm->makeString("received").asString(), vm->makeInt(response.length()));

    Value headersMap = vm->makeMap();
    MapInstance *headers = headersMap.asMap();
    for (const auto &h : httpResp.headers)
    {
        headers->table.set(vm->makeString(h.first.c_str()).asString(), vm->makeString(h.second.c_str()));
    }
    map->table.set(vm->makeString("headers").asString(), headersMap);

    vm->push(result);
    return 1;
}

// HTTP POST request -

// =============================================================
// JSON Serializer
// =============================================================
static std::string serializeJson(Interpreter *vm, Value value)
{
    if (value.isString())
    {
        return "\"" + std::string(value.asStringChars()) + "\"";
    }
    else if (value.isInt())
    {
        return std::to_string(value.asInt());
    }
    else if (value.isFloat())
    {
        return std::to_string(value.asFloat());
    }
    else if (value.isDouble())
    {
        return std::to_string(value.asDouble());
    }
    else if (value.isBool())
    {
        return value.asBool() ? "true" : "false";
    }
    else if (value.isNil())
    {
        return "null";
    }
    else if (value.isMap())
    {
        std::string json = "{";
        MapInstance *map = value.asMap();
        bool first = true;
        map->table.forEach([&](String *k, Value v)
                           {
            if (!first) json += ",";
            first = false;
            json += "\"" + std::string(k->chars()) + "\":" + serializeJson(vm, v); });
        json += "}";
        return json;
    }
    else if (value.isArray())
    {
        std::string json = "[";
        ArrayInstance *arr = value.asArray();
        for (size_t i = 0; i < arr->values.size(); i++)
        {
            if (i > 0)
                json += ",";
            json += serializeJson(vm, arr->values[i]);
        }
        json += "]";
        return json;
    }
    return "null";
}

// =============================================================
// FUNCTION: HTTP POST
// =============================================================
int native_socket_http_post(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isString())
    {
        vm->runtimeError("http_post expects (url, [options_map])");
        return 0;
    }

    std::string url = args[0].asStringChars();

    // --- DEFAULTS ---
    std::map<std::string, std::string> customHeaders;
    std::string postData;
    std::string contentType = "application/x-www-form-urlencoded"; // Default
    std::string userAgent = "SocketModule/1.0";
    int timeout = 30;

    // --- PARSE OPTIONS ---
    if (argCount >= 2 && args[1].isMap())
    {
        MapInstance *options = args[1].asMap();
        Value val;

        // 1. Headers
        if (options->table.get(vm->makeString("headers").asString(), &val))
        {
            if (val.isMap())
                customHeaders = extractHeaders(vm, val);
        }

        // 2. Data (Raw String ou Form Map)
        if (options->table.get(vm->makeString("data").asString(), &val))
        {
            if (val.isString())
            {
                postData = val.asStringChars();
            }
            else if (val.isMap())
            {
                postData = buildQueryString(vm, val);
            }
        }

        // 3. JSON (Auto-serialize) - TEM PRIORIDADE SOBRE 'data'
        if (options->table.get(vm->makeString("json").asString(), &val))
        {
            //   serializa   JSON
            postData = serializeJson(vm, val);
            contentType = "application/json";
        }

        // 4. Timeout
        if (options->table.get(vm->makeString("timeout").asString(), &val))
        {
            if (val.isInt())
                timeout = val.asInt();
        }

        // 5. User Agent Explícito
        if (options->table.get(vm->makeString("user_agent").asString(), &val))
        {
            if (val.isString())
                userAgent = val.asStringChars();
        }
    }

    // --- SMART HEADERS MANAGEMENT ---
    // User-Agent: Se existir nos headers,
    auto itUA = customHeaders.find("User-Agent");
    if (itUA == customHeaders.end())
        itUA = customHeaders.find("user-agent");
    if (itUA != customHeaders.end())
    {
        userAgent = itUA->second;
        customHeaders.erase(itUA);
    }

    // Content-Type: Se existir nos headers,
    auto itCT = customHeaders.find("Content-Type");
    if (itCT == customHeaders.end())
        itCT = customHeaders.find("content-type");
    if (itCT != customHeaders.end())
    {
        contentType = itCT->second;
        customHeaders.erase(itCT);
    }

    // --- URL PARSING ---
    size_t protoEnd = url.find("://");
    if (protoEnd == std::string::npos)
    {
        vm->runtimeError("Invalid URL");
        return 0;
    }

    std::string protocol = url.substr(0, protoEnd);
    if (protocol == "https")
    {
        vm->runtimeError("HTTPS not supported");
        return 0;
    }

    size_t hostStart = protoEnd + 3;
    size_t pathStart = url.find('/', hostStart);
    std::string host = url.substr(hostStart, pathStart - hostStart);
    std::string path = (pathStart != std::string::npos) ? url.substr(pathStart) : "/";
    int port = 80;
    size_t portPos = host.find(':');
    if (portPos != std::string::npos)
    {
        port = std::stoi(host.substr(portPos + 1));
        host = host.substr(0, portPos);
    }

    // --- SOCKET CONNECT ---
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
    {
        vm->runtimeError("Socket error");
        return 0;
    }

    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv));

    struct hostent *he = gethostbyname(host.c_str());
    if (!he)
    {
        closesocket(sock);
        vm->runtimeError("DNS error");
        return 0;
    }

    sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);

    if (connect(sock, (sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        closesocket(sock);
        vm->runtimeError("Connection failed");
        return 0;
    }

    // --- BUILD REQUEST ---
    std::string request = "POST " + path + " HTTP/1.1\r\n";
    request += "Host: " + host + "\r\n";
    request += "User-Agent: " + userAgent + "\r\n";
    request += "Content-Type: " + contentType + "\r\n";
    request += "Content-Length: " + std::to_string(postData.length()) + "\r\n";
    request += "Connection: close\r\n";

    for (const auto &header : customHeaders)
    {
        request += header.first + ": " + header.second + "\r\n";
    }

    request += "\r\n";
    request += postData;

    // --- SEND & RECEIVE ---
    if (send(sock, request.c_str(), request.length(), 0) == SOCKET_ERROR)
    {
        closesocket(sock);
        return 0;
    }

    std::string response;
    char buffer[4096];
    int received;
    while ((received = recv(sock, buffer, sizeof(buffer), 0)) > 0)
    {
        response.append(buffer, received);
    }
    closesocket(sock);

    // --- RESPONSE ---
    HttpResponse httpResp = parseHttpResponse(response);

    Value result = vm->makeMap();
    MapInstance *map = result.asMap();

    map->table.set(vm->makeString("status_code").asString(), vm->makeInt(httpResp.statusCode));
    map->table.set(vm->makeString("status_text").asString(), vm->makeString(httpResp.statusText.c_str()));
    map->table.set(vm->makeString("body").asString(), vm->makeString(httpResp.body.c_str()));
    map->table.set(vm->makeString("success").asString(), vm->makeBool(httpResp.success));
    map->table.set(vm->makeString("url").asString(), vm->makeString(url.c_str()));

    Value headersMap = vm->makeMap();
    MapInstance *headers = headersMap.asMap();
    for (const auto &h : httpResp.headers)
    {
        headers->table.set(vm->makeString(h.first.c_str()).asString(), vm->makeString(h.second.c_str()));
    }
    map->table.set(vm->makeString("headers").asString(), headersMap);

    vm->push(result);

    return 1;
}

//
// UTILS
//

int native_socket_ping(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isString())
    {
        vm->runtimeError("ping expects (host, [port], [timeout])");
        return 1;
    }

    const char *host = args[0].asStringChars();
    int port = (argCount >= 2 && args[1].isNumber()) ? args[1].asNumber() : 80;
    int timeout = (argCount >= 3 && args[2].isNumber()) ? args[2].asNumber() : 2;

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
    {
        return 1;
    }

    // Set timeout
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv));

    struct hostent *he = gethostbyname(host);
    if (!he)
    {
        closesocket(sock);
        return 1;
    }

    sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);

    bool success = (connect(sock, (sockaddr *)&addr, sizeof(addr)) != SOCKET_ERROR);
    closesocket(sock);

    vm->push(vm->makeBool(success));
    return 1;
}

// Download de file (Streamed to disk para não encher a RAM)
int native_socket_download_file(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 2 || !args[0].isString() || !args[1].isString())
    {
        vm->runtimeError("download_file expects (url, filepath)");
        return 1;
    }

    std::string url = args[0].asStringChars();
    std::string filepath = args[1].asStringChars();

    // 1. Parse URL (Host/Path/Port) - Reutiliza lógica do http_get
    size_t protoEnd = url.find("://");
    if (protoEnd == std::string::npos)
        return 1;
    size_t hostStart = protoEnd + 3;
    size_t pathStart = url.find('/', hostStart);
    std::string host = url.substr(hostStart, pathStart - hostStart);
    std::string path = (pathStart != std::string::npos) ? url.substr(pathStart) : "/";
    int port = 80;
    size_t portPos = host.find(':');
    if (portPos != std::string::npos)
    {
        port = std::stoi(host.substr(portPos + 1));
        host = host.substr(0, portPos);
    }

    // 2. Connect
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
        return 1;

    struct hostent *he = gethostbyname(host.c_str());
    if (!he)
    {
        closesocket(sock);
        return 1;
    }

    sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);

    if (connect(sock, (sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        closesocket(sock);
        return 1;
    }

    // 3. Send Request
    std::string request = "GET " + path + " HTTP/1.1\r\nHost: " + host + "\r\nConnection: close\r\n\r\n";
    send(sock, request.c_str(), request.length(), 0);

    // 4. Open File
    FILE *file = fopen(filepath.c_str(), "wb");
    if (!file)
    {
        closesocket(sock);
        return 1;
    }

    // 5. Receive & Skip Headers
    char buffer[4096];
    int received;
    bool headerFinished = false;
    std::string headerBuffer;

    while ((received = recv(sock, buffer, sizeof(buffer), 0)) > 0)
    {
        if (!headerFinished)
        {
            headerBuffer.append(buffer, received);
            size_t headerEnd = headerBuffer.find("\r\n\r\n");

            if (headerEnd != std::string::npos)
            {
                // Header found! Write the rest to file
                headerFinished = true;
                size_t bodyStart = headerEnd + 4;
                if (bodyStart < headerBuffer.length())
                {
                    fwrite(headerBuffer.data() + bodyStart, 1, headerBuffer.length() - bodyStart, file);
                }
            }
        }
        else
        {
            // Write body directly to disk
            fwrite(buffer, 1, received, file);
        }
    }

    fclose(file);
    closesocket(sock);

    vm->push(vm->makeBool(true));
    return 1;
}

// Resolver hostname para IP (Ex: "google.com" -> "142.250.184.46")
int native_socket_resolve(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isString())
    {
        vm->runtimeError("resolve expects hostname");
        return 0;
    }

#ifdef _WIN32
    if (!wsaInitialized)
        native_socket_init(vm, 0, nullptr);
#endif

    const char *hostname = args[0].asStringChars();

    struct hostent *he = gethostbyname(hostname);

    if (he == nullptr)
    {

        return 0;
    }

    if (he->h_addr_list && he->h_addr_list[0])
    {
        struct in_addr addr;

        memcpy(&addr, he->h_addr_list[0], sizeof(struct in_addr));

        vm->push(vm->makeString(inet_ntoa(addr)));
        return 1;
    }

    return 0;
}

int native_socket_get_local_ip(Interpreter *vm, int argCount, Value *args)
{
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR)
    {
        return 0;
    }

    struct hostent *he = gethostbyname(hostname);
    if (!he || !he->h_addr_list[0])
    {
        return 0;
    }

    struct in_addr addr;
    memcpy(&addr, he->h_addr_list[0], sizeof(struct in_addr));

    vm->push(vm->makeString(inet_ntoa(addr)));

    return 1;
}
//
// TCP
//

int native_socket_tcp_listen(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isInt())
    {
        vm->runtimeError("tcp_listen expects port number");
        return 0;
    }

    int port = args[0].asInt();
    int backlog = 10;

    if (argCount >= 2 && args[1].isInt())
        backlog = args[1].asInt();

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
    {
        vm->runtimeError("Failed to create socket");
        return 0;
    }

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

    sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sock, (sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        closesocket(sock);
        vm->runtimeError("Failed to bind to port %d", port);
        return 0;
    }

    if (listen(sock, backlog) == SOCKET_ERROR)
    {
        closesocket(sock);
        vm->runtimeError("Failed to listen on port %d", port);
        return 0;
    }

    SocketHandle *handle = new SocketHandle();
    handle->socket = sock;
    handle->type = SocketType::TCP_SERVER;
    handle->isBlocking = true;
    handle->isConnected = true;
    handle->port = port;

    openSockets.push_back(handle);
    vm->push(vm->makeInt(nextSocketId++));

    return 1;
}

int native_socket_tcp_accept(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isInt())
        return 0;

    int id = args[0].asInt();
    if (id <= 0 || id > openSockets.size() || !openSockets[id - 1])
        return 0;

    SocketHandle *serverHandle = openSockets[id - 1];

    if (serverHandle->type != SocketType::TCP_SERVER)
    {
        vm->runtimeError("Socket is not a TCP server");
        return 0;
    }

    sockaddr_in clientAddr = {0};
    socklen_t addrLen = sizeof(clientAddr);

    SOCKET clientSock = accept(serverHandle->socket, (sockaddr *)&clientAddr, &addrLen);

    if (clientSock == INVALID_SOCKET)
    {
#ifdef _WIN32
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK)
            return 0;
#else
        if (errno == EWOULDBLOCK || errno == EAGAIN)
            return 0;
#endif
        return 0;
    }

    SocketHandle *clientHandle = new SocketHandle();
    clientHandle->socket = clientSock;
    clientHandle->type = SocketType::TCP_CLIENT;
    clientHandle->isBlocking = true;
    clientHandle->isConnected = true;
    clientHandle->port = ntohs(clientAddr.sin_port);
    clientHandle->host = inet_ntoa(clientAddr.sin_addr);

    openSockets.push_back(clientHandle);
    vm->push(vm->makeInt(nextSocketId++));

    return 1;
}

int native_socket_tcp_connect(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 2 || !args[0].isString() || !args[1].isInt())
    {
        vm->runtimeError("tcp_connect expects (host, port)");
        return 0;
    }

    const char *host = args[0].asStringChars();
    int port = args[1].asInt();

    struct hostent *he = gethostbyname(host);
    if (!he)
    {
        vm->runtimeError("Failed to resolve hostname '%s'", host);
        return 0;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
    {
        vm->runtimeError("Failed to create socket");
        return 0;
    }

    sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);

    if (connect(sock, (sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
#ifdef _WIN32
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK)
        {
            closesocket(sock);
            vm->runtimeError("Failed to connect to %s:%d", host, port);
            return 0;
        }
#else
        if (errno != EINPROGRESS)
        {
            closesocket(sock);
            vm->runtimeError("Failed to connect to %s:%d", host, port);
            return 0;
        }
#endif
    }

    SocketHandle *handle = new SocketHandle();
    handle->socket = sock;
    handle->type = SocketType::TCP_CLIENT;
    handle->isBlocking = true;
    handle->isConnected = true;
    handle->port = port;
    handle->host = host;

    openSockets.push_back(handle);
    vm->push(vm->makeInt(nextSocketId++));

    return 1;
}

int native_socket_udp_create(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isInt())
    {
        vm->runtimeError("udp_create expects port");
        return 0;
    }

    int port = args[0].asInt();

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET)
    {
        vm->runtimeError("Failed to create UDP socket");
        return 0;
    }

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

    sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sock, (sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        closesocket(sock);
        vm->runtimeError("Failed to bind UDP socket to port %d", port);
        return 0;
    }

    SocketHandle *handle = new SocketHandle();
    handle->socket = sock;
    handle->type = SocketType::UDP;
    handle->isBlocking = true;
    handle->isConnected = false;
    handle->port = port;

    openSockets.push_back(handle);
    vm->push(vm->makeInt(nextSocketId++));

    return 1;
}

int native_socket_set_blocking(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 2 || !args[0].isInt() || !args[1].isBool())
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    int id = args[0].asInt();
    bool blocking = args[1].asBool();

    if (id <= 0 || id > openSockets.size() || !openSockets[id - 1])
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    SocketHandle *handle = openSockets[id - 1];

#ifdef _WIN32
    u_long mode = blocking ? 0 : 1;
    if (ioctlsocket(handle->socket, FIONBIO, &mode) != 0)
    {
        vm->push(vm->makeBool(false));
        return 1;
    }
#else
    int flags = fcntl(handle->socket, F_GETFL, 0);
    if (flags == -1)
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    if (fcntl(handle->socket, F_SETFL, flags) != 0)
    {
        vm->push(vm->makeBool(false));
        return 1;
    }
#endif

    handle->isBlocking = blocking;
    vm->push(vm->makeBool(true));
    return 1;
}
int native_socket_set_nodelay(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 2 || !args[0].isInt() || !args[1].isBool())
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    int id = args[0].asInt();
    bool nodelay = args[1].asBool();

    if (id <= 0 || id > openSockets.size() || !openSockets[id - 1])
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    SocketHandle *handle = openSockets[id - 1];
    if (handle->type == SocketType::UDP)
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    int flag = nodelay ? 1 : 0;
    if (setsockopt(handle->socket, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag)) != 0)
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    vm->push(vm->makeBool(true));
    return 1;
}

int native_socket_send(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 2 || !args[0].isInt() || !args[1].isString())
    {
        vm->push(vm->makeInt(-1));
        return 1;
    }

    int id = args[0].asInt();
    if (id <= 0 || id > openSockets.size() || !openSockets[id - 1])
    {
        vm->push(vm->makeInt(-1));
        return 1;
    }

    SocketHandle *handle = openSockets[id - 1];
    if (handle->type == SocketType::UDP)
    {
        vm->runtimeError("Use sendto() for UDP sockets");
        vm->push(vm->makeInt(-1));
        return 1;
    }

    if (!handle->isConnected)
    {
        vm->runtimeError("Socket not connected");
        vm->push(vm->makeInt(-1));
        return 1;
    }

    const char *data = args[1].asStringChars();
    int len = args[1].asString()->length();
    int sent = send(handle->socket, data, len, 0);

    if (sent == SOCKET_ERROR)
    {
#ifdef _WIN32
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK)
        {
            vm->push(vm->makeInt(0));
            return 1;
        }
        handle->isConnected = false;
#else
        if (errno == EWOULDBLOCK || errno == EAGAIN)
        {
            vm->push(vm->makeInt(0));
            return 1;
        }
        handle->isConnected = false;
#endif
        vm->push(vm->makeInt(-1));
        return 1;
    }

    vm->push(vm->makeInt(sent));
    return 1;
}

int native_socket_receive(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isInt())
    {
        vm->push(vm->makeNil());
        return 1;
    }

    int id = args[0].asInt();
    int maxSize = 4096;
    if (argCount >= 2 && args[1].isInt())
        maxSize = args[1].asInt();

    if (id <= 0 || id > openSockets.size() || !openSockets[id - 1])
    {
        vm->push(vm->makeNil());
        return 1;
    }

    SocketHandle *handle = openSockets[id - 1];
    if (handle->type == SocketType::UDP)
    {
        vm->runtimeError("Use recvfrom() for UDP sockets");
        vm->push(vm->makeNil());
        return 1;
    }

    std::vector<char> buffer(maxSize);
    int received = recv(handle->socket, buffer.data(), maxSize, 0);

    if (received == SOCKET_ERROR)
    {
#ifdef _WIN32
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK)
        {
            vm->push(vm->makeNil());
            return 1;
        }
        handle->isConnected = false;
#else
        if (errno == EWOULDBLOCK || errno == EAGAIN)
        {
            vm->push(vm->makeNil());
            return 1;
        }
        handle->isConnected = false;
#endif
        vm->push(vm->makeNil());
        return 1;
    }

    if (received == 0)
    {
        handle->isConnected = false;
        vm->push(vm->makeNil());
        return 1;
    }

    vm->push(vm->makeString(std::string(buffer.data(), received).c_str()));
    return 1;
}

int native_socket_sendto(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 4 || !args[0].isInt() || !args[1].isString() || !args[2].isString() || !args[3].isInt())
    {
        vm->runtimeError("sendto expects (socketId, data, host, port)");
        vm->push(vm->makeInt(-1));
        return 1;
    }

    int id = args[0].asInt();
    const char *data = args[1].asStringChars();
    const char *host = args[2].asStringChars();
    int port = args[3].asInt();

    if (id <= 0 || id > openSockets.size() || !openSockets[id - 1])
    {
        vm->push(vm->makeInt(-1));
        return 1;
    }

    SocketHandle *handle = openSockets[id - 1];
    if (handle->type != SocketType::UDP)
    {
        vm->runtimeError("sendto() is for UDP sockets only");
        vm->push(vm->makeInt(-1));
        return 1;
    }

    struct hostent *he = gethostbyname(host);
    if (!he)
    {
        vm->push(vm->makeInt(-1));
        return 1;
    }

    sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);

    int len = args[1].asString()->length();
    int sent = sendto(handle->socket, data, len, 0, (sockaddr *)&addr, sizeof(addr));

    if (sent == SOCKET_ERROR)
    {
        vm->push(vm->makeInt(-1));
        return 1;
    }

    vm->push(vm->makeInt(sent));
    return 1;
}

int native_socket_recvfrom(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isInt())
    {
        vm->push(vm->makeNil());
        return 1;
    }

    int id = args[0].asInt();
    int maxSize = 4096;
    if (argCount >= 2 && args[1].isInt())
        maxSize = args[1].asInt();

    if (id <= 0 || id > openSockets.size() || !openSockets[id - 1])
    {
        vm->push(vm->makeNil());
        return 1;
    }

    SocketHandle *handle = openSockets[id - 1];
    if (handle->type != SocketType::UDP)
    {
        vm->runtimeError("recvfrom() is for UDP sockets only");
        vm->push(vm->makeNil());
        return 1;
    }

    std::vector<char> buffer(maxSize);
    sockaddr_in fromAddr = {0};
    socklen_t fromLen = sizeof(fromAddr);

    int received = recvfrom(handle->socket, buffer.data(), maxSize, 0, (sockaddr *)&fromAddr, &fromLen);

    if (received == SOCKET_ERROR)
    {
#ifdef _WIN32
        if (WSAGetLastError() == WSAEWOULDBLOCK)
        {
            vm->push(vm->makeNil());
            return 1;
        }
#else
        if (errno == EWOULDBLOCK || errno == EAGAIN)
        {
            vm->push(vm->makeNil());
            return 1;
        }
#endif
        vm->push(vm->makeNil());
        return 1;
    }

    Value result = vm->makeMap();
    MapInstance *map = result.asMap();
    map->table.set(vm->makeString("data").asString(), vm->makeString(std::string(buffer.data(), received).c_str()));
    map->table.set(vm->makeString("host").asString(), vm->makeString(inet_ntoa(fromAddr.sin_addr)));
    map->table.set(vm->makeString("port").asString(), vm->makeInt(ntohs(fromAddr.sin_port)));

    vm->push(result);
    return 1;
}

int native_socket_info(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isInt())
    {
        vm->push(vm->makeNil());
        return 1;
    }

    int id = args[0].asInt();
    if (id <= 0 || id > openSockets.size() || !openSockets[id - 1])
    {
        vm->push(vm->makeNil());
        return 1;
    }

    SocketHandle *handle = openSockets[id - 1];
    Value result = vm->makeMap();
    MapInstance *map = result.asMap();

    const char *typeStr = "unknown";
    if (handle->type == SocketType::TCP_SERVER)
        typeStr = "tcp_server";
    else if (handle->type == SocketType::TCP_CLIENT)
        typeStr = "tcp_client";
    else if (handle->type == SocketType::UDP)
        typeStr = "udp";

    map->table.set(vm->makeString("type").asString(), vm->makeString(typeStr));
    map->table.set(vm->makeString("port").asString(), vm->makeInt(handle->port));
    map->table.set(vm->makeString("blocking").asString(), vm->makeBool(handle->isBlocking));
    map->table.set(vm->makeString("connected").asString(), vm->makeBool(handle->isConnected));

    if (!handle->host.empty())
        map->table.set(vm->makeString("host").asString(), vm->makeString(handle->host.c_str()));

    vm->push(result);
    return 1;
}

int native_socket_close(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isInt())
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    int id = args[0].asInt();
    if (id <= 0 || id > openSockets.size() || !openSockets[id - 1])
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    SocketHandle *handle = openSockets[id - 1];
    if (handle->type != SocketType::UDP)
        shutdown(handle->socket, SHUT_RDWR);

    closesocket(handle->socket);
    delete handle;
    openSockets[id - 1] = nullptr;

    vm->push(vm->makeBool(true));
    return 1;
}

int native_socket_is_connected(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isInt())
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    int id = args[0].asInt();
    if (id <= 0 || id > openSockets.size() || !openSockets[id - 1])
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    SocketHandle *handle = openSockets[id - 1];
    vm->push(vm->makeBool(handle->isConnected));
    return 1;
}

// No registerSocket():

void Interpreter::registerSocket()
{
    static bool initialized = false;
    if (!initialized)
    {
        atexit(SocketModuleCleanup);
        initialized = true;
    }

    addModule("socket")
        .addFunction("init", native_socket_init, 0)
        .addFunction("quit", native_socket_quit, 0)

        .addFunction("tcp_listen", native_socket_tcp_listen, -1)
        .addFunction("tcp_accept", native_socket_tcp_accept, 1)
        .addFunction("tcp_connect", native_socket_tcp_connect, 2)

        .addFunction("udp_create", native_socket_udp_create, 1)

        .addFunction("send", native_socket_send, 2)
        .addFunction("receive", native_socket_receive, -1)
        .addFunction("sendto", native_socket_sendto, 4)
        .addFunction("recvfrom", native_socket_recvfrom, -1)

        .addFunction("is_connected", native_socket_is_connected, 1)

        .addFunction("set_blocking", native_socket_set_blocking, 2)
        .addFunction("set_nodelay", native_socket_set_nodelay, 2)

        // HTTP Utilities (estilo requests com properties)
        .addFunction("http_get", native_socket_http_get, -1)
        .addFunction("http_post", native_socket_http_post, -1)
        .addFunction("download_file", native_socket_download_file, -1)

        // Network Utilities
        .addFunction("ping", native_socket_ping, -1)
        .addFunction("get_local_ip", native_socket_get_local_ip, 0)
        .addFunction("resolve", native_socket_resolve, 1)

        .addFunction("info", native_socket_info, 1)
        .addFunction("close", native_socket_close, 1);
}

#endif

/*

# GET simples
response = socket.http_get("http://api.example.com/users")

# GET com headers customizados
response = socket.http_get("http://api.example.com/users", {
    headers: {
        "Authorization": "Bearer token123",
        "User-Agent": "MyApp/1.0"
    },
    timeout: 10
})

# GET com query parameters
response = socket.http_get("http://api.example.com/search", {
    params: {
        q: "python",
        page: 1,
        limit: 50
    },
    headers: {
        "Accept": "application/json"
    }
})

# POST com data (form-encoded)
response = socket.http_post("http://api.example.com/login", {
    data: {
        username: "admin",
        password: "secret123"
    },
    headers: {
        "Content-Type": "application/x-www-form-urlencoded"
    }
})

# POST com JSON
response = socket.http_post("http://api.example.com/users", {
    json: {
        name: "John Doe",
        email: "john@example.com",
        age: 30
    },
    headers: {
        "Authorization": "Bearer token123"
    },
    timeout: 15
})

# POST com string raw
response = socket.http_post("http://api.example.com/webhook", {
    data: '{"event": "user.created", "id": 123}',
    headers: {
        "Content-Type": "application/json"
    }
})

# Acessar resposta
if response["success"]:
    print("Status:", response["status_code"])
    print("Body:", response["body"])
    print("Content-Type:", response["headers"]["Content-Type"])
else:
    print("Erro:", response["status_text"])

# Download com opções
socket.download_file("http://example.com/file.zip", "./file.zip", {
    headers: {
        "Authorization": "Bearer token123"
    },
    timeout: 60
})

# Ping com timeout custom
if socket.ping("google.com", 443, 5):
    print("Host alcançável!")

*/