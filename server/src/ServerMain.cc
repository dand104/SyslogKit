#include <iostream>
#include <vector>
#include <thread>
#include "SocketWrapper.H"
#include "SyslogKit/SyslogProto"

void run_udp_server(const uint16_t port) {
    const socket_t server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_fd == INVALID_SOCKET_VAL) {
        std::cerr << "Failed to create UDP socket" << std::endl;
        return;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR_VAL) {
        std::cerr << "UDP Bind failed" << std::endl;
        return;
    }

    std::cout << "UDP Syslog Server listening on port " << port << "..." << std::endl;

    std::vector<char> buffer(2048);
    while (true) {
        sockaddr_in client_addr{};
        int client_len = sizeof(client_addr);
        int bytes_received = recvfrom(server_fd, buffer.data(), buffer.size() - 1, 0, (sockaddr*)&client_addr, (socklen_t*)&client_len);

        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            const std::string_view raw(buffer.data());

            auto parsed = SyslogKit::SyslogBuilder::parse(raw);

            std::cout << "[UDP] Received: " << raw << std::endl;
            std::cout << "      Parsed Msg: " << parsed.message << std::endl;
        }
    }
    SocketGuard::close_socket(server_fd);
}

int main() {
    if (!SocketGuard::init()) return 1;

    std::jthread udp_thread([]() { run_udp_server(5140); });

    std::cout << "Server handles running. Press Enter to exit..." << std::endl;
    std::cin.get();

    SocketGuard::cleanup();
    return 0;
}