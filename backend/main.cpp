#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "models.h"
#include "dsa.h"
#include "scheduler.h"
#include "storage.h"
#include "api.h"

#pragma comment(lib, "ws2_32.lib")
using namespace std;

Department dept;
Division divisions[MAX_DIVS]; int numDivs = 0;
Faculty faculties[MAX_FAC]; int numFac = 0;
Subject subjects[MAX_SUBS]; int numSubs = 0;
Room rooms[MAX_ROOMS]; int numRooms = 0;
Break breaks[MAX_BREAKS]; int numBreaks = 0;
Cell grid[MAX_DIVS][MAX_DAYS][MAX_SLOTS];
UnscheduledInfo unscheduled[MAX_SESS]; int numUnsched = 0;
Graph divGraph;
UndoStack undoStack;
FacultyHash facHash;

string readFile(const string& path) {
    ifstream f(path.c_str(), ios::binary);
    if (!f) return "";
    ostringstream ss; ss << f.rdbuf(); return ss.str();
}

void sendResponse(SOCKET s, const string& type, const string& body, int code = 200) {
    ostringstream oss;
    oss << "HTTP/1.1 " << code << " OK\r\n"
        << "Content-Type: " << type << "\r\n"
        << "Content-Length: " << body.size() << "\r\n"
        << "Access-Control-Allow-Origin: *\r\n"
        << "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
        << "Access-Control-Allow-Headers: Content-Type\r\n"
        << "Connection: close\r\n\r\n" << body;
    string res = oss.str();
    send(s, res.c_str(), (int)res.size(), 0);
    shutdown(s, SD_SEND);
}

int main() {
    loadData();
    buildFacultyGraph();

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;
    SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);

    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    if (bind(server, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR || listen(server, 10) == SOCKET_ERROR) {
        closesocket(server); WSACleanup(); return 1;
    }

    cout << "DeptFlow V2 server running at http://localhost:8080\n";

    while (true) {
        SOCKET client = accept(server, nullptr, nullptr);
        if (client == INVALID_SOCKET) continue;

        string req = ""; char buf[4096]; int bytes = 0, clen = -1;
        while ((bytes = recv(client, buf, sizeof(buf) - 1, 0)) > 0) {
            buf[bytes] = '\0'; req.append(buf, bytes);
            if (clen == -1) {
                size_t p = req.find("Content-Length:");
                if (p != string::npos) clen = atoi(req.substr(p + 15).c_str());
            }
            size_t bPos = req.find("\r\n\r\n");
            if (bPos != string::npos && (clen == -1 || (int)(req.size() - (bPos + 4)) >= clen)) break;
        }
        if (req.empty()) { closesocket(client); continue; }

        istringstream iss(req);
        string method, path, proto;
        iss >> method >> path >> proto;

        if (method == "OPTIONS") {
            sendResponse(client, "text/plain", "");
            closesocket(client); continue;
        }

        string body = "";
        size_t bPos = req.find("\r\n\r\n");
        if (bPos != string::npos) body = req.substr(bPos + 4);

        if (path.rfind("/api/", 0) == 0) {
            string resp = "{}";
            if (path == "/api/config") resp = handleConfig(method, body);
            else if (path == "/api/faculty") resp = handleFaculty(method, body);
            else if (path == "/api/subjects") resp = handleSubjects(method, body);
            else if (path == "/api/divisions") resp = handleDivisions(method, body);
            else if (path == "/api/rooms") resp = handleRooms(method, body);
            else if (path == "/api/breaks") resp = handleBreaks(method, body);
            else if (path == "/api/timetable/session") resp = handleSessionEdit(body);
            else if (path == "/api/timetable/reset") resp = handleResetTimetable(body);
            else if (path == "/api/reset") resp = handleReset(body);
            else if (path == "/api/generate") resp = handleGenerate(body);
            else if (path.rfind("/api/timetable", 0) == 0) {
                int divIdx = 0; size_t q = path.find("division=");
                if (q != string::npos) divIdx = atoi(path.substr(q + 9).c_str());
                resp = handleTimetable(divIdx);
            }
            else if (path == "/api/reports") resp = handleReports();
            sendResponse(client, "application/json", resp);
        } else {
            string fileContent = "", mime = "text/plain";
            if (path == "/" || path == "/index.html") { fileContent = readFile("frontend/index.html"); mime = "text/html"; }
            else if (path == "/css/style.css") { fileContent = readFile("frontend/css/style.css"); mime = "text/css"; }
            else if (path == "/js/app.js") { fileContent = readFile("frontend/js/app.js"); mime = "application/javascript"; }

            if (!fileContent.empty()) sendResponse(client, mime, fileContent);
            else sendResponse(client, "text/plain", "404 Not Found", 404);
        }
        closesocket(client);
    }
    closesocket(server); WSACleanup(); return 0;
}
