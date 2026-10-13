#include <iostream>
#include <string>
#include <cstring>
#include <vector>

#include "SyslogKit/SyslogProto.hxx"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    using socket_t = SOCKET;
    #define CLOSE_SOCKET closesocket
    #define IS_INVALID(s) ((s) == INVALID_SOCKET)
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
    using socket_t = int;
    #define CLOSE_SOCKET close
    #define IS_INVALID(s) ((s) < 0)
#endif

namespace {
    struct SocketInitializer {
        SocketInitializer() {
#ifdef _WIN32
            WSADATA wsa;
            WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
        }
        ~SocketInitializer() {
#ifdef _WIN32
            WSACleanup();
#endif
        }
    };
}

void print_help(const char* prog) {
    std::cout << "Usage: " << prog << " [options] [message]\n"
              << "Options:\n"
              << "  -h, --host <ip/host>   Target host (default: 127.0.0.1)\n"
              << "  -p, --port <port>      Target port (default: 5140)\n"
              << "  -t, --tcp              Use TCP protocol (default: UDP)\n"
              << "  -u, --udp              Use UDP protocol\n"
              << "  -a, --app <name>       Application/Tag name (default: SyslogSend)\n"
              << "  -s, --sev <0-7>        Severity (0=Emerg .. 7=Debug, default: 6/Info)\n"
              << "  -f, --fac <0-23>       Facility (default: 1/User)\n"
              << "  -r, --raw              Send raw message as is (without RFC3164 formatting)\n"
              << "  --help                 Show this help message\n\n"
              << "Examples:\n"
              << "  " << prog << " \"Hello from Syslog\"\n"
              << "  " << prog << " -p 514 -t -s 3 -m \"Critical error occurred\"\n";
}

int main(int argc, char* argv[]) {
    SocketInitializer sock_init;

    std::string host = "127.0.0.1";
    int port = 5140;
    bool use_tcp = false;
    bool raw_mode = false;
    std::string app_name = "SyslogSend";
    int severity = static_cast<int>(SyslogKit::Severity::Info);
    int facility = static_cast<int>(SyslogKit::Facility::User);
    std::string message;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help") {
            print_help(argv[0]);
            return 0;
        } else if ((arg == "-h" || arg == "--host") && i + 1 < argc) {
            host = argv[++i];
        } else if ((arg == "-p" || arg == "--port") && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "-t" || arg == "--tcp") {
            use_tcp = true;
        } else if (arg == "-u" || arg == "--udp") {
            use_tcp = false;
        } else if ((arg == "-a" || arg == "--app") && i + 1 < argc) {
            app_name = argv[++i];
        } else if ((arg == "-s" || arg == "--sev") && i + 1 < argc) {
            severity = std::stoi(argv[++i]);
        } else if ((arg == "-f" || arg == "--fac") && i + 1 < argc) {
            facility = std::stoi(argv[++i]);
        } else if (arg == "-r" || arg == "--raw") {
            raw_mode = true;
        } else if ((arg == "-m" || arg == "--msg") && i + 1 < argc) {
            message = argv[++i];
        } else if (arg[0] != '-') {
            if (!message.empty()) message += " ";
            message += arg;
        }
    }

    if (message.empty()) {
        message = "Test syslog message";
    }

    std::string packet_data;
    if (raw_mode) {
        packet_data = message;
    } else {
        SyslogKit::SyslogMessage msg;
        msg.facility = static_cast<SyslogKit::Facility>(facility);
        msg.severity = static_cast<SyslogKit::Severity>(severity);
        msg.app_name = app_name;
        msg.message = message;
        packet_data = SyslogKit::SyslogBuilder::build(msg);
    }

    addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = use_tcp ? SOCK_STREAM : SOCK_DGRAM;

    std::string port_str = std::to_string(port);
    if (getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res) != 0 || !res) {
        std::cerr << "[Error] Failed to resolve host: " << host << "\n";
        return 1;
    }

    socket_t sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (IS_INVALID(sock)) {
        std::cerr << "[Error] Failed to create socket\n";
        freeaddrinfo(res);
        return 1;
    }

    bool success = false;
    if (use_tcp) {
        if (connect(sock, res->ai_addr, static_cast<int>(res->ai_addrlen)) == 0) {
            if (packet_data.back() != '\n') {
                packet_data += '\n';
            }
            int sent = send(sock, packet_data.data(), static_cast<int>(packet_data.size()), 0);
            success = (sent > 0);
        } else {
            std::cerr << "[Error] Failed to connect via TCP to " << host << ":" << port << "\n";
        }
    } else {
        int sent = sendto(sock, packet_data.data(), static_cast<int>(packet_data.size()), 0,
                          res->ai_addr, static_cast<int>(res->ai_addrlen));
        success = (sent > 0);
    }

    CLOSE_SOCKET(sock);
    freeaddrinfo(res);

    if (success) {
        std::cout << "[OK] Sent (" << (use_tcp ? "TCP" : "UDP") << " -> " << host << ":" << port << "): "
                  << packet_data;
        if (packet_data.back() != '\n') std::cout << "\n";
        return 0;
    } else {
        std::cerr << "[Error] Failed to send packet\n";
        return 1;
    }
}