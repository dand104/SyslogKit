#pragma once
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    using socket_t = SOCKET;
    #define INVALID_SOCKET_VAL INVALID_SOCKET
    #define SOCKET_ERROR_VAL SOCKET_ERROR
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    using socket_t = int;
    #define INVALID_SOCKET_VAL -1
    #define SOCKET_ERROR_VAL -1
#endif

#include <iostream>

class SocketGuard {
public:
    static bool init() {
        #ifdef _WIN32
                WSADATA wsa;
                return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
        #endif
        return true;
    }
    static void cleanup() {
        #ifdef _WIN32
                WSACleanup();
        #endif
    }
    static void close_socket(socket_t s) {
        #ifdef _WIN32
                closesocket(s);
        #else
                close(s);
        #endif
    }
};