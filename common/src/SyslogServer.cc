#include "SyslogKit/SyslogServer"
#include <vector>
#include <iostream>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib")
    using sock_t = SOCKET;
    #define CLOSE_SOCK closesocket
    #define INVALID_SOCK INVALID_SOCKET
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <arpa/inet.h>
    using sock_t = int;
    #define CLOSE_SOCK close
    #define INVALID_SOCK -1
#endif

namespace SyslogKit {

    struct WSAInit {
        WSAInit() {
        #ifdef _WIN32
                    WSADATA w; WSAStartup(MAKEWORD(2,2), &w);
        #endif
                }
                ~WSAInit() {
        #ifdef _WIN32
                    WSACleanup();
        #endif
        }
    };
    static WSAInit wsa_init;

    Server::Server() = default;
    Server::~Server() { stop(); }

    void Server::start(uint16_t port, const bool udp, const bool tcp) {
        if (running_) stop();
        running_ = true;
        if (udp) udp_thread_ = std::jthread(&Server::udp_loop, this, port);
        if (tcp) tcp_thread_ = std::jthread(&Server::tcp_loop, this, port);
    }

    void Server::stop() { running_ = false; }

    void Server::udp_loop(const uint16_t port) {
        const sock_t fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd == INVALID_SOCK) return;

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;

        int opt = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
        if (bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
            CLOSE_SOCK(fd); return;
        }

        // For production code, use select() or non-blocking recv

        char buf[2048];
        while (running_) {
            fd_set fds; FD_ZERO(&fds); FD_SET(fd, &fds);
            timeval tv{0, 50000};
            if (select(static_cast<int>(fd) + 1, &fds, nullptr, nullptr, &tv) > 0) {
                sockaddr_in cli{};
                socklen_t len = sizeof(cli);
                const int n = recvfrom(fd, buf, sizeof(buf), 0, reinterpret_cast<sockaddr *>(&cli), &len);

                if (n > 0) {
                    auto msg = SyslogBuilder::parse(std::string_view(buf, static_cast<size_t>(n)));
                    if(msg.hostname.empty()) {
                        char ip[64];
                        inet_ntop(AF_INET, &cli.sin_addr, ip, 64);
                        msg.hostname = ip;
                    }
                    if (callback_) callback_(msg);
                }
            }
        }
        CLOSE_SOCK(fd);
    }

    void Server::tcp_loop(const uint16_t port) {
        const sock_t fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd == INVALID_SOCK) return;

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) { CLOSE_SOCK(fd); return; }
        listen(fd, 5);

        while (running_) {
            fd_set fds; FD_ZERO(&fds); FD_SET(fd, &fds);
            timeval tv{0, 50000};
            if (select(static_cast<int>(fd) + 1, &fds, 0, 0, &tv) > 0) {
                sockaddr_in cli{};
                socklen_t len = sizeof(cli);
                const sock_t client = accept(fd, reinterpret_cast<sockaddr *>(&cli), &len);
                if (client != INVALID_SOCK) {
                    char buf[4096];
                    if (const int n = recv(client, buf, sizeof(buf), 0); n > 0) {
                        auto msg = SyslogBuilder::parse(std::string_view(buf, static_cast<size_t>(n)));
                        if(msg.hostname.empty()) {
                            char ip[64];
                            inet_ntop(AF_INET, &cli.sin_addr, ip, 64);
                            msg.hostname = ip;
                        }
                        if (callback_) callback_(msg);
                    }
                    CLOSE_SOCK(client);
                }
            }
        }
        CLOSE_SOCK(fd);
    }
}