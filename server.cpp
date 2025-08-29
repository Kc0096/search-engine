#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#define PORT 8080

struct Entry {
    std::string keyword;
    std::string pdf;
    std::string gfg;
    std::string wiki;
    std::string yt;
};

// Load database from db.txt
std::vector<Entry> loadDatabase(const std::string &filename) {
    std::vector<Entry> db;
    std::ifstream file(filename);
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        Entry e;
        std::getline(ss, e.keyword, '|');
        std::getline(ss, e.pdf, '|');
        std::getline(ss, e.gfg, '|');
        std::getline(ss, e.wiki, '|');
        std::getline(ss, e.yt, '|');
        db.push_back(e);
    }
    return db;
}

// Search database for query
std::vector<Entry> search(const std::vector<Entry> &db, const std::string &query) {
    std::vector<Entry> results;
    std::string q = query;
    std::transform(q.begin(), q.end(), q.begin(), ::tolower);

    for (const auto &e : db) {
        std::string k = e.keyword;
        std::transform(k.begin(), k.end(), k.begin(), ::tolower);
        if (k.find(q) != std::string::npos) results.push_back(e);
    }
    return results;
}

// Read file content as string
std::string getFileContent(const std::string &filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return "";
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

// URL decode helper
std::string urlDecode(const std::string &SRC) {
    std::string ret;
    char ch;
    int i, ii;
    for (i = 0; i < (int)SRC.length(); i++) {
        if (int(SRC[i]) == 37) {
            sscanf(SRC.substr(i + 1, 2).c_str(), "%x", &ii);
            ch = static_cast<char>(ii);
            ret += ch;
            i += 2;
        } else if (SRC[i] == '+') ret += ' ';
        else ret += SRC[i];
    }
    return ret;
}

// Determine MIME type based on file extension
std::string getMimeType(const std::string &path) {
    if (path.find(".css") != std::string::npos) return "text/css";
    if (path.find(".js") != std::string::npos) return "application/javascript";
    if (path.find(".png") != std::string::npos) return "image/png";
    if (path.find(".jpg") != std::string::npos || path.find(".jpeg") != std::string::npos) return "image/jpeg";
    if (path.find(".ico") != std::string::npos) return "image/x-icon";
    return "text/html"; // default
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    SOCKET server_fd, client;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    auto database = loadDatabase("db.txt");

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        std::cerr << "Socket creation failed\n";
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
        std::cerr << "Bind failed\n";
        return 1;
    }

    if (listen(server_fd, 10) == SOCKET_ERROR) {
        std::cerr << "Listen failed\n";
        return 1;
    }

    std::cout << "Server running at http://localhost:" << PORT << std::endl;

    while (true) {
        client = accept(server_fd, (struct sockaddr*)&address, &addrlen);
        if (client == INVALID_SOCKET) {
            std::cerr << "Accept failed\n";
            continue;
        }

        char buffer[8192] = {0};
        int bytesRead = recv(client, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead <= 0) {
            closesocket(client);
            continue;
        }
        buffer[bytesRead] = '\0';

        std::string request(buffer);
        std::string response;

        if (request.find("GET /search?query=") != std::string::npos) {
            // Handle search API
            std::string query = request.substr(request.find("query=") + 6);
            query = query.substr(0, query.find(" "));
            query = urlDecode(query);

            auto results = search(database, query);

            std::ostringstream oss;
            oss << "[";
            for (size_t i = 0; i < results.size(); i++) {
                oss << "{";
                oss << "\"keyword\":\"" << results[i].keyword << "\",";
                oss << "\"pdf\":\"" << results[i].pdf << "\",";
                oss << "\"gfg\":\"" << results[i].gfg << "\",";
                oss << "\"wiki\":\"" << results[i].wiki << "\",";
                oss << "\"yt\":\"" << results[i].yt << "\"";
                oss << "}";
                if (i != results.size() - 1) oss << ",";
            }
            oss << "]";
            std::string jsonData = oss.str();

            response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: "
                + std::to_string(jsonData.size()) + "\r\n\r\n" + jsonData;

        } else {
            // Serve static files
            std::string path = "index.html"; // default
            size_t start = request.find("GET ") + 4;
            size_t end = request.find(" ", start);
            std::string reqPath = request.substr(start, end - start);
            if (reqPath != "/") {
                if (reqPath[0] == '/') reqPath.erase(0, 1);
                path = reqPath;
            }

            size_t qpos = path.find("?");
            if (qpos != std::string::npos) path = path.substr(0, qpos);

            std::string content = getFileContent(path);
            if (content.empty()) {
                std::string notFound = "<h1>404 Not Found</h1>";
                response = "HTTP/1.1 404 Not Found\r\nContent-Length: " + std::to_string(notFound.size())
                    + "\r\nContent-Type: text/html\r\n\r\n" + notFound;
            } else {
                response = "HTTP/1.1 200 OK\r\nContent-Type: " + getMimeType(path)
                    + "\r\nContent-Length: " + std::to_string(content.size())
                    + "\r\n\r\n" + content;
            }
        }

        send(client, response.c_str(), response.size(), 0);
        closesocket(client);
    }

    WSACleanup();
    return 0;
}
