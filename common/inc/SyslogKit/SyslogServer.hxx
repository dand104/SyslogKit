#pragma once
#include "SyslogProto.hxx"
#include <functional>
#include <thread>
#include <atomic>

namespace SyslogKit {

    class Server {
    public:
        using Callback = std::function<void(SyslogMessage)>;

        Server();
        ~Server();

        bool start(uint16_t port, bool udp, bool tcp);
        void stop();
        void set_callback(Callback const& cb) { callback_ = cb; }

    private:
        void udp_loop(uint64_t fd) const;
        void tcp_loop(uint64_t fd) const;

        std::atomic<bool> running_{false};
        std::jthread udp_thread_;
        std::jthread tcp_thread_;
        Callback callback_;
    };
}