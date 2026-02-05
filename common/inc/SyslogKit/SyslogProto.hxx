#pragma once

#include <string>
#include <string_view>
#include <cstdint>

namespace SyslogKit {

    enum class Severity {
        Emergency = 0, // System is unusable
        Alert = 1,     // Action must be taken immediately
        Critical = 2,  // Critical conditions
        Error = 3,     // Error conditions
        Warning = 4,   // Warning conditions
        Notice = 5,    // Normal but significant condition
        Info = 6,      // Informational messages
        Debug = 7      // Debug-level messages
    };

    enum class Facility {
        Kern = 0, User = 1, Mail = 2, Daemon = 3, Auth = 4,
        Syslog = 5, Lpr = 6, News = 7, Uucp = 8, Cron = 9,
        Authpriv = 10, Ftp = 11, Local0 = 16, Local1 = 17,
        Local2 = 18, Local3 = 19, Local4 = 20, Local5 = 21,
        Local6 = 22, Local7 = 23
    };

    struct SyslogMessage {
        Facility facility = Facility::User;
        Severity severity = Severity::Info;
        std::string timestamp;
        std::string hostname;
        std::string app_name;
        std::string message;

        [[nodiscard]] int get_priority() const {
            return (static_cast<int>(facility) * 8) + static_cast<int>(severity);
        }
    };

    class SyslogBuilder {
    public:
        static std::string build(const SyslogMessage& msg);

        static SyslogMessage parse(std::string_view raw_msg);
    };

} // namespace syslog